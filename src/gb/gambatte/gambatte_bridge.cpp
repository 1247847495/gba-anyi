/* AURORA_GAMBATTE_V1
 * Gambatte libretro bridge for PS2/SNESticle Aurora.
 * GB/GBC V1: in-memory load, RGB565 video, stereo PCM, single pad,
 * SRAM/state, blipper audio (~32 kHz).
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <kernel.h>
#include "gambatte_bridge.h"
#include "types.h"
#include "emuinput.h"
#include "rendersurface.h"
#include "pixelformat.h"
#include "mixbuffer.h"
#include "audmixbuffer.h"
extern AudMixBuffer *_AudMix;
#include "snio.h"
#include "platform/ps2/system/mainloop_ui.h"

extern "C" {
#include "../../third_party/gambatte/libgambatte/libretro-common/include/libretro.h"
}

/* Namespaced Gambatte exports (tools/namespace_gambatte_archive.py, GMBT_ prefix). */
extern "C" {
unsigned GMBT_retro_api_version(void);
void GMBT_retro_set_environment(retro_environment_t);
void GMBT_retro_set_video_refresh(retro_video_refresh_t);
void GMBT_retro_set_audio_sample(retro_audio_sample_t);
void GMBT_retro_set_audio_sample_batch(retro_audio_sample_batch_t);
void GMBT_retro_set_input_poll(retro_input_poll_t);
void GMBT_retro_set_input_state(retro_input_state_t);
void GMBT_retro_set_controller_port_device(unsigned, unsigned);
void GMBT_retro_init(void);
void GMBT_retro_deinit(void);
bool GMBT_retro_load_game(const struct retro_game_info *);
void GMBT_retro_unload_game(void);
void GMBT_retro_reset(void);
void GMBT_retro_run(void);
size_t GMBT_retro_serialize_size(void);
bool GMBT_retro_serialize(void *, size_t);
bool GMBT_retro_unserialize(const void *, size_t);
void *GMBT_retro_get_memory_data(unsigned);
size_t GMBT_retro_get_memory_size(unsigned);
void GMBT_retro_get_system_av_info(struct retro_system_av_info *);
}

static bool s_Initialized = false;
static bool s_GameLoaded = false;
static Emu::SysInputT *s_pInput = NULL;
static CMixBuffer *s_pMix = NULL;
static const void *s_VideoData = NULL;
static unsigned s_VideoW = 0, s_VideoH = 0;
/* AURORA_GBA_SCALE_V2: gskit_backend.c 导出的逻辑像素物理宽高比。 */
extern "C" float g_GskLogicalPixelAspect;
static Int32 s_ScaleMode = 0; /* AURORA_GBA_SCALE_V4: 0=原始 1=等比 2=拉伸 3=4:3 4=16:9 */
static Int32 s_PaletteMode = 0;     /* 0=自动 1=DMG 2=Pocket 3=Light 4=GBC蓝 5=SGB1A 6=Special1 */
static Int32 s_MixFrames = 0;       /* 0=关 1=简单混合 2=LCD残影 3=快速残影 */
static Int32 s_ColorCorrection = 1; /* 0=关 1=仅GBC 2=始终 */
static size_t s_VideoPitch = 0;
static bool s_HaveVideo = false;
static Uint8 *s_pSramData = NULL;
static Int32 s_SramBytes = 0;
static unsigned s_SampleRate = 32768;
static char s_SystemPath[1024] = ".";
static char s_ContentName[1024] = "game.gb";

/* Gambatte emits conventional RGB565 (R high, B low); reuse the
 * PicoDrive/Beetle expansion-table scheme. */
static Uint32 s_GbR[32], s_GbG[64], s_GbB[32];
static bool s_GbColorTablesReady = false;

static void gbInitColorTables(void)
{
    if (s_GbColorTablesReady) return;
    for (unsigned i = 0; i < 32; ++i)
    {
        Uint32 v = (i << 3) | (i >> 2);
        s_GbR[i] = v;
        s_GbB[i] = v << 16;
    }
    for (unsigned i = 0; i < 64; ++i)
    {
        Uint32 v = (i << 2) | (i >> 4);
        s_GbG[i] = v << 8;
    }
    s_GbColorTablesReady = true;
}

static void gbLog(enum retro_log_level level, const char *fmt, ...)
{
    (void)level;
    if (!fmt) return;
    va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap);
}

/* AURORA_GBA_UI_V1: core options driven by the frontend settings menu.
 * Values are read by the core during retro_load_game(), so palette /
 * ghosting / colour-correction changes apply on the next game start. */
static const char *gbVariable(const char *key)
{
    if (!key) return NULL;
    if (!strcmp(key, "gambatte_gb_colorization"))
        return s_PaletteMode == 0 ? "auto" : "internal";
    if (!strcmp(key, "gambatte_gb_internal_palette"))
    {
        switch (s_PaletteMode)
        {
            case 1:  return "GB - DMG";
            case 2:  return "GB - Pocket";
            case 3:  return "GB - Light";
            case 4:  return "GBC - Blue";
            case 5:  return "SGB - 1A";
            case 6:  return "Special 1";
            default: return "GB - DMG";
        }
    }
    if (!strcmp(key, "gambatte_mix_frames"))
    {
        switch (s_MixFrames)
        {
            case 1:  return "mix";
            case 2:  return "lcd_ghosting";
            case 3:  return "lcd_ghosting_fast";
            default: return "disabled";
        }
    }
    if (!strcmp(key, "gambatte_gbc_color_correction"))
    {
        switch (s_ColorCorrection)
        {
            case 0:  return "disabled";
            case 2:  return "always";
            default: return "GBC only";
        }
    }
    /* 其余选项沿用核心默认值 (blipper 重采样器等) */
    return NULL;
}

static bool gbEnvironment(unsigned cmd, void *data)
{
    switch (cmd)
    {
        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
            if (!data) return false;
            ((struct retro_log_callback *)data)->log = gbLog; return true;
        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
            return data && *(enum retro_pixel_format *)data == RETRO_PIXEL_FORMAT_RGB565;
        case RETRO_ENVIRONMENT_GET_VARIABLE:
        {
            struct retro_variable *v = (struct retro_variable *)data;
            if (!v || !v->key) return false;
            v->value = gbVariable(v->key); return v->value != NULL;
        }
        case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
            if (!data) return false; *(bool *)data = false; return true;
        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
            if (!data) return false; *(const char **)data = s_SystemPath; return true;
        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        case RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY:
            if (!data) return false; *(const char **)data = s_SystemPath; return true;
        case RETRO_ENVIRONMENT_GET_CAN_DUPE:
            if (data) *(bool *)data = true; return true;
        case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
            if (data) *(unsigned *)data = 0; return true;
        case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:
            return true;
        case RETRO_ENVIRONMENT_GET_GAME_INFO_EXT:
            return false;
        case RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER:
        case RETRO_ENVIRONMENT_GET_VFS_INTERFACE:
        case RETRO_ENVIRONMENT_GET_PERF_INTERFACE:
        case RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION:
        case RETRO_ENVIRONMENT_GET_LANGUAGE:
            return false;
        case RETRO_ENVIRONMENT_SET_VARIABLES:
        case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
        case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
        case RETRO_ENVIRONMENT_SET_MEMORY_MAPS:
        case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
        case RETRO_ENVIRONMENT_SET_GEOMETRY:
        case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO:
        case RETRO_ENVIRONMENT_SET_MESSAGE:
        case RETRO_ENVIRONMENT_SET_MESSAGE_EXT:
        case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:
        case RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS:
        case RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE:
        case RETRO_ENVIRONMENT_SET_MINIMUM_AUDIO_LATENCY:
        case RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK:
        case RETRO_ENVIRONMENT_SET_FASTFORWARDING_OVERRIDE:
        case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY:
        case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK:
        case RETRO_ENVIRONMENT_SET_VARIABLE:
            return true;
        default: return false;
    }
}

static void gbVideoRefresh(const void *data, unsigned w, unsigned h, size_t pitch)
{
    if (!data || !w || !h || pitch < w * 2) { s_HaveVideo = false; return; }
    s_VideoData = data; s_VideoW = w; s_VideoH = h; s_VideoPitch = pitch; s_HaveVideo = true;
}

static void gbAudioSample(int16_t l, int16_t r)
{
    if (!s_pMix) return;
    Int16 L = (Int16)l, R = (Int16)r; s_pMix->OutputSamplesStereo(&L, &R, 1);
}

static size_t gbAudioBatch(const int16_t *data, size_t frames)
{
    if (!data || !frames || !s_pMix) return frames;
    if (_AudMix && s_pMix == _AudMix && frames <= 0x7fffffffU)
    {
        _AudMix->OutputLibretroInterleaved(
            (const Int16 *)data, (Int32)frames);
        return frames;
    }
    /* Gambatte caps chunks at 64 Ki frames, but the fallback path below
     * is only used by non-AudMixBuffer sinks; keep a local chunk cap. */
    enum { GB_AUDIO_CHUNK = 1024 };
    static Int16 s_AudioL[GB_AUDIO_CHUNK], s_AudioR[GB_AUDIO_CHUNK];
    size_t pos = 0;
    while (pos < frames)
    {
        size_t n = frames - pos; if (n > GB_AUDIO_CHUNK) n = GB_AUDIO_CHUNK;
        for (size_t i = 0; i < n; ++i)
        {
            s_AudioL[i] = (Int16)data[(pos + i) * 2 + 0];
            s_AudioR[i] = (Int16)data[(pos + i) * 2 + 1];
        }
        s_pMix->OutputSamplesStereo(s_AudioL, s_AudioR, (Int32)n); pos += n;
    }
    return frames;
}

static void gbInputPoll(void) {}
static bool gbPadHas(Uint16 p, Uint16 bit) { return p != EMUSYS_DEVICE_DISCONNECTED && (p & bit); }

static int16_t gbJoyMask(unsigned port)
{
    if (!s_pInput || port != 0) return 0;
    Uint16 p = s_pInput->uPad[0]; if (p == EMUSYS_DEVICE_DISCONNECTED) return 0;
    unsigned m = 0;
    if (gbPadHas(p, SNESIO_JOY_UP))     m |= 1u << RETRO_DEVICE_ID_JOYPAD_UP;
    if (gbPadHas(p, SNESIO_JOY_DOWN))   m |= 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
    if (gbPadHas(p, SNESIO_JOY_LEFT))   m |= 1u << RETRO_DEVICE_ID_JOYPAD_LEFT;
    if (gbPadHas(p, SNESIO_JOY_RIGHT))  m |= 1u << RETRO_DEVICE_ID_JOYPAD_RIGHT;
    if (gbPadHas(p, SNESIO_JOY_A))      m |= 1u << RETRO_DEVICE_ID_JOYPAD_A;
    if (gbPadHas(p, SNESIO_JOY_B))      m |= 1u << RETRO_DEVICE_ID_JOYPAD_B;
    if (gbPadHas(p, SNESIO_JOY_SELECT)) m |= 1u << RETRO_DEVICE_ID_JOYPAD_SELECT;
    if (gbPadHas(p, SNESIO_JOY_START))  m |= 1u << RETRO_DEVICE_ID_JOYPAD_START;
    /* Gambatte uses L/R for internal palette cycling; harmless when the
     * feature is inactive, so pass the shoulders through. */
    if (gbPadHas(p, SNESIO_JOY_L))      m |= 1u << RETRO_DEVICE_ID_JOYPAD_L;
    if (gbPadHas(p, SNESIO_JOY_R))      m |= 1u << RETRO_DEVICE_ID_JOYPAD_R;
    return (int16_t)m;
}

static int16_t gbInputState(unsigned port, unsigned device, unsigned index, unsigned id)
{
    (void)index;
    if (device != RETRO_DEVICE_JOYPAD || port != 0) return 0;
    int16_t mask = gbJoyMask(port);
    if (id == RETRO_DEVICE_ID_JOYPAD_MASK) return mask;
    if (id > RETRO_DEVICE_ID_JOYPAD_R3) return 0;
    return (mask & (1u << id)) ? 1 : 0;
}

static inline Uint32 gb565(Uint16 c)
{
    /* RGB565: RRRRRGGGGGGBBBBB. Aurora RGBA word: AABBGGRR. */
    return 0xff000000u |
           s_GbR[(c >> 11) & 31u] |
           s_GbG[(c >> 5) & 63u] |
           s_GbB[c & 31u];
}

static void gbRender(CRenderSurface *target)
{
    if (!target || !s_HaveVideo || !s_VideoData) return;

    PixelFormatT *fmt = target->GetFormat();
    if (!fmt || fmt->uBitDepth != 32) return;

    gbInitColorTables();

    /* AURORA_GBA_SCALE_V2: 可视逻辑区域 256x240 (表面 256x256)。 */
    int tw = (int)target->GetWidth();
    int th = (int)target->GetHeight();
    if (tw > 256) tw = 256;
    if (th > 240) th = 240;
    if (tw <= 0 || th <= 0) return;

    /* GB is 160x144 (10:9; SGB borders may be wider). Rectangles are
     * computed in logical coordinates but constrained to the desired
     * *physical* aspect: (w/h) * pixelAspect = physical ratio. */
    const int srcW = (int)s_VideoW;
    const int srcH = (int)s_VideoH;
    const float srcAspect = (float)srcW / (float)srcH;
    const float par = g_GskLogicalPixelAspect > 0.05f
                    ? g_GskLogicalPixelAspect : 1.25f;
    int dstW, dstH;
    if (s_ScaleMode == 2)
    {
        dstW = tw; dstH = th; /* 全屏拉伸 (忽略比例) */
    }
    else
    {
        float want;
        if (s_ScaleMode == 3)      want = (4.0f / 3.0f) / par;   /* 4:3  */
        else if (s_ScaleMode == 4) want = (16.0f / 9.0f) / par;  /* 16:9 */
        else                       want = srcAspect / par;       /* 原比例 */
        if (want < 0.05f) want = 0.05f;

        if (s_ScaleMode == 0)
        {
            /* 原始大小: 保持源宽, 高度按物理比例补偿 */
            dstW = srcW < tw ? srcW : tw;
            dstH = (int)((float)dstW / want + 0.5f);
            if (dstH > th) { dstH = th; dstW = (int)((float)th * want + 0.5f); }
        }
        else
        {
            dstW = tw; dstH = (int)((float)tw / want + 0.5f);
            if (dstH > th) { dstH = th; dstW = (int)((float)th * want + 0.5f); }
        }
        if (dstW < 1) dstW = 1;
        if (dstH < 1) dstH = 1;
        if (dstW > tw) dstW = tw;
        if (dstH > th) dstH = th;
    }
    const int dstX = (tw - dstW) / 2;
    const int dstY = (th - dstH) / 2;
    const Uint32 black = 0xff000000u;

    for (int y = 0; y < th; ++y)
    {
        Uint32 *dst = (Uint32 *)target->GetLinePtr(y);
        if (!dst) continue;

        if (y < dstY || y >= dstY + dstH)
        {
            for (int x = 0; x < tw; ++x) dst[x] = black;
            continue;
        }

        const int sy = (y - dstY) * srcH / dstH;
        const Uint16 *src = (const Uint16 *)(
            (const Uint8 *)s_VideoData + (size_t)sy * s_VideoPitch);

        for (int x = 0; x < dstX; ++x) dst[x] = black;
        {
            /* 定点步进采样, 支持任意缩放比例 */
            const unsigned stepX = ((unsigned)srcW << 16) / (unsigned)dstW;
            unsigned fx = stepX >> 1;
            for (int x = 0; x < dstW; ++x, fx += stepX)
                dst[dstX + x] = gb565(src[fx >> 16]);
        }
        for (int x = dstX + dstW; x < tw; ++x) dst[x] = black;
    }
}

Int32 GbBridge_GetScalingMode(void) { return s_ScaleMode; }
void GbBridge_SetScalingMode(Int32 mode)
{
    if (mode < 0 || mode > 4) mode = 0; /* AURORA_GBA_SCALE_V4: 0..4 */
    s_ScaleMode = mode;
}
Int32 GbBridge_GetPaletteMode(void) { return s_PaletteMode; }
void GbBridge_SetPaletteMode(Int32 mode)
{
    if (mode < 0 || mode > 6) mode = 0;
    s_PaletteMode = mode;
}
Int32 GbBridge_GetMixFrames(void) { return s_MixFrames; }
void GbBridge_SetMixFrames(Int32 mode)
{
    if (mode < 0 || mode > 3) mode = 0;
    s_MixFrames = mode;
}
Int32 GbBridge_GetColorCorrection(void) { return s_ColorCorrection; }
void GbBridge_SetColorCorrection(Int32 mode)
{
    if (mode < 0 || mode > 2) mode = 1;
    s_ColorCorrection = mode;
}

/* AURORA_GB_WATCHDOG_REMOVED_V32_20260913: the entire watchdog thread and
 * all its heartbeat/loadStep globals have been removed. They were the
 * source of the GB/GBC regressions (the watchdog's frame-pumping marks
 * corrupted GS state mid-frame on real hardware, and the hard caps on
 * the event loop broke interrupt-heavy scenes like level transitions).
 * The original SNESticleAurora Gambatte port ran perfectly without any
 * of this instrumentation; we restore that clean state. The variables
 * below are kept as stubs only to satisfy any remaining extern
 * references during the transition. */
extern "C" {
volatile unsigned g_GbBeat = 0;
volatile unsigned g_GbPhase = 0;
volatile long     g_GbOuterIter = 0;
volatile unsigned g_GbStuckCount = 0;
volatile unsigned g_GbFramesCount = 0;
volatile int g_GbRunActive = 0;
volatile int g_GbSessionStarted = 0;
volatile unsigned g_MlRenderBeat = 0;
volatile unsigned g_GbLoadStep = 0;
}

bool GbBridge_Init(void)
{
    if (s_Initialized) return true;
    GMBT_retro_set_environment(gbEnvironment); GMBT_retro_set_video_refresh(gbVideoRefresh);
    GMBT_retro_set_audio_sample(gbAudioSample); GMBT_retro_set_audio_sample_batch(gbAudioBatch);
    GMBT_retro_set_input_poll(gbInputPoll); GMBT_retro_set_input_state(gbInputState);
    GMBT_retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
    if (GMBT_retro_api_version()!=RETRO_API_VERSION) { printf("[GB] libretro API mismatch\n"); return false; }
    GMBT_retro_init(); s_Initialized=true; return true;
}

void GbBridge_Shutdown(void)
{
    if (!s_Initialized) return; GbBridge_UnloadGame(); GMBT_retro_deinit(); s_Initialized=false;
}

/* AURORA_GB_ZEROCOPY_REMOVED_V33_20260913: ZEROCOPY (externalRom)
 * removed. It altered the ELF memory layout vs the original SNESticle
 * port, which tripped a PS2 layout-sensitive indirect-call fault inside
 * StateSaver::loadState's fn-pointer dispatch (`(*it->load)(...)`),
 * freezing the machine on quick-load. GB/GBC ROMs are <=8MB and the PS2
 * has 32MB EE RAM, so the core's own allocation is safe. */

bool GbBridge_LoadGame(const void *data, size_t bytes, size_t capacity, const char *name)
{
    if (!data || bytes < 0x150 || bytes > (size_t)(8U*1024U*1024U) || !GbBridge_Init()) return false;
    if (s_GameLoaded)
        GbBridge_UnloadGame();

    if (name && *name)
    {
        strncpy(s_ContentName, name, sizeof(s_ContentName) - 1);
        s_ContentName[sizeof(s_ContentName) - 1] = 0;
    }
    else
        strcpy(s_ContentName, "game.gb");

    struct retro_game_info info; memset(&info,0,sizeof(info));
    info.path=s_ContentName; info.data=data; info.size=bytes;
    if (!GMBT_retro_load_game(&info))
    {
        printf("[GB] retro_load_game failed: %s (%u bytes)\n",s_ContentName,(unsigned)bytes);
        return false;
    }
    s_GameLoaded=true;
    {
        size_t sb=GMBT_retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
        void *sp=GMBT_retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
        s_pSramData=(sp&&sb&&sb<=0x7fffffffU)?(Uint8*)sp:NULL;
        s_SramBytes=s_pSramData?(Int32)sb:0;
    }
    struct retro_system_av_info av; memset(&av,0,sizeof(av)); GMBT_retro_get_system_av_info(&av);
    if (av.timing.sample_rate>=8000.0 && av.timing.sample_rate<=96000.0)
        s_SampleRate=(unsigned)(av.timing.sample_rate+0.5);
    printf("[GB] loaded %s; SRAM=%d; audio=%u Hz\n",s_ContentName,(int)s_SramBytes,s_SampleRate);
    return true;
}

void GbBridge_UnloadGame(void)
{
    if (s_Initialized && s_GameLoaded) GMBT_retro_unload_game();
    s_GameLoaded=false; s_pInput=NULL; s_pMix=NULL; s_pSramData=NULL; s_SramBytes=0;
    s_VideoData=NULL; s_VideoW=s_VideoH=0; s_VideoPitch=0; s_HaveVideo=false;
}

bool GbBridge_IsGameLoaded(void) { return s_GameLoaded; }

void GbBridge_Reset(void) { if (s_GameLoaded) GMBT_retro_reset(); }

void GbBridge_RunFrame(Emu::SysInputT *input, CRenderSurface *target, CMixBuffer *mix)
{
    if (!s_GameLoaded)
        return;

    s_pInput = input;
    s_pMix = mix;
    s_HaveVideo = false;

    GMBT_retro_run();

    if (s_HaveVideo)
        gbRender(target);

    if (s_pMix)
        s_pMix->Flush();

    s_pMix = NULL;
    s_pInput = NULL;
}

int GbBridge_GetStateSize(void)
{
    if (!s_GameLoaded) return 0;
    size_t n = GMBT_retro_serialize_size();
    return n && n <= 0x7fffffffU ? (int)n : 0;
}

int GbBridge_SaveState(void *data, int bytes)
{
    int n = GbBridge_GetStateSize();
    int r = data && n > 0 && bytes >= n && GMBT_retro_serialize(data, (size_t)n) ? n : 0;
    return r;
}

bool GbBridge_LoadState(const void *data, int bytes)
{
    bool ok = data && bytes > 0 && GMBT_retro_unserialize(data, (size_t)bytes);
    return ok;
}

int GbBridge_GetSRAMBytes(void) { return s_GameLoaded ? s_SramBytes : 0; }
Uint8 *GbBridge_GetSRAMData(void) { return s_GameLoaded ? s_pSramData : NULL; }
unsigned GbBridge_GetSampleRate(void) { return s_SampleRate; }
