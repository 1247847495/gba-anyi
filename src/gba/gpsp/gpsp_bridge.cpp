/* AURORA_GPSP_V1
 * gpSP libretro bridge for PS2/GBAticle Aurora.
 * GBA V1: path-based cart load, BGR555->RGBA32 video, stereo PCM,
 * single pad, SRAM/state, dynarec-friendly environment.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include "gpsp_bridge.h"
#include "types.h"
#include "emuinput.h"
#include "rendersurface.h"
#include "pixelformat.h"
#include "mixbuffer.h"
#include "audmixbuffer.h"
extern AudMixBuffer *_AudMix;
#include "snio.h"
#include "platform/ps2/system/mainloop_ui.h"
#include "platform/ps2/system/mainloop_state.h"

extern "C" {
#include "../../third_party/gpsp/libretro/libretro-common/include/libretro.h"
}

/* Namespaced gpSP exports (tools/namespace_gpsp_archive.py, GPSP_ prefix). */
extern "C" {
unsigned GPSP_retro_api_version(void);
void GPSP_retro_set_environment(retro_environment_t);
void GPSP_retro_set_video_refresh(retro_video_refresh_t);
void GPSP_retro_set_audio_sample(retro_audio_sample_t);
void GPSP_retro_set_audio_sample_batch(retro_audio_sample_batch_t);
void GPSP_retro_set_input_poll(retro_input_poll_t);
void GPSP_retro_set_input_state(retro_input_state_t);
void GPSP_retro_set_controller_port_device(unsigned, unsigned);
void GPSP_retro_init(void);
void GPSP_retro_deinit(void);
bool GPSP_retro_load_game(const struct retro_game_info *);
void GPSP_retro_unload_game(void);
void GPSP_retro_reset(void);
void GPSP_retro_run(void);
size_t GPSP_retro_serialize_size(void);
bool GPSP_retro_serialize(void *, size_t);
bool GPSP_retro_unserialize(const void *, size_t);
void *GPSP_retro_get_memory_data(unsigned);
size_t GPSP_retro_get_memory_size(unsigned);
void GPSP_retro_get_system_av_info(struct retro_system_av_info *);
}

/* AURORA_GPSP_STATE_V1: identity globals exported by gba_memory.c and
 * renamed to the GPSP_ namespace by tools/namespace_gpsp_archive.py. */
extern "C" {
extern unsigned GPSP_gamepak_identity_crc;
extern unsigned GPSP_gamepak_identity_size;
}

/* AURORA_GBA_ZIP_MEM_V1_20260911: cache-resident cart load API exported
 * by gba_memory.c and renamed to the GPSP_ namespace by
 * tools/namespace_gpsp_archive.py (non-static globals are auto-prefixed). */
extern "C" {
extern unsigned GPSP_gamepak_cache_window_bytes(void);
extern void GPSP_gamepak_arm_memory_load(unsigned size);
extern unsigned GPSP_gamepak_memory_sink(unsigned ofs,
                                         const unsigned char *src,
                                         unsigned n);
}

static bool s_Initialized = false;
static bool s_CartLoaded = false;
static Emu::SysInputT *s_pInput = NULL;
static CMixBuffer *s_pMix = NULL;
static const void *s_VideoData = NULL;
static unsigned s_VideoW = 0, s_VideoH = 0;
/* AURORA_GBA_SCALE_V2: gskit_backend.c 导出的逻辑像素物理宽高比
 * (256x240 逻辑画布 -> 当前模式的物理显示窗口)。 */
extern "C" float g_GskLogicalPixelAspect;
static Int32 s_ScaleMode = 0; /* AURORA_GBA_SCALE_V4: 0=原始 1=等比 2=拉伸 3=4:3 4=16:9 */
static size_t s_VideoPitch = 0;
static bool s_HaveVideo = false;
static Uint8 *s_pSramData = NULL;
static Int32 s_SramBytes = 0;
static unsigned s_SampleRate = 65536;
static char s_SystemPath[1024] = ".";
static char s_ContentPath[1024] = "game.gba";

/* AURORA_GPSP_V1
 * PS2 gpSP emits GBA-native BGR555 (USE_XBGR1555_FORMAT keeps the
 * 15-bit colour untouched). Expand each 5-bit channel once through
 * small tables instead of per-pixel arithmetic. */
static Uint32 s_GbaR[32], s_GbaG[32], s_GbaB[32];
static bool s_GbaColorTablesReady = false;

static void gbaInitColorTables(void)
{
    if (s_GbaColorTablesReady) return;
    for (unsigned i = 0; i < 32; ++i)
    {
        Uint32 v = (i << 3) | (i >> 2);
        s_GbaR[i] = v;
        s_GbaG[i] = v << 8;
        s_GbaB[i] = v << 16;
    }
    s_GbaColorTablesReady = true;
}

static void gbaLog(enum retro_log_level level, const char *fmt, ...)
{
    (void)level;
    if (!fmt) return;
    va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap);
}

/* AURORA_GBA_FREEZE_DIAG_V1: minimal on-screen sub-step markers for
 * gpSP load/run. Uses MainLoopModalPrintf so the last painted frame
 * stays on screen if real hardware dies. Keep the count tiny - thirty
 * 4-frame markers are known to freeze PS2 hardware by themselves. */
/* AURORA_MARK_OFF_V1_20260910: off by default -- the markers themselves
 * stacked up to the known ~30-render freeze threshold. Set 1 to re-arm
 * on-screen tracing. */
/* AURORA_GBA_LAUNCH_TIMING_V1 (diagnostic build 20260911): armed for the
   GBA slow-launch investigation - every mark is one-shot (init/load/reset/
   first frame), so the ~10 marks per launch match the proven-safe V10-V12
   diagnostic density. Each mark shows seconds since LoadCart entry. */
enum { AURORA_MARK_ENABLE = 1 };
static time_t s_BridgeMarkT0 = 0;
static void GbaBridgeMark(int nSub, const char *pFormat, ...)
{
    char str[128];
    va_list argptr;
    long el;
    if (!pFormat || !AURORA_MARK_ENABLE) return;
    if (!s_BridgeMarkT0) s_BridgeMarkT0 = time(NULL);
    el = (long)(time(NULL) - s_BridgeMarkT0);
    va_start(argptr, pFormat);
    vsnprintf(str, sizeof(str), pFormat, argptr);
    va_end(argptr);
    MainLoopModalPrintf(8, "DBG[G%d] T+%lds %s", 10 + nSub, el, str);
}

/* gpSP core options; defaults mirror RetroArch's option defaults so the
 * dynarec stays on and the RTC follows the frontend clock. */
static const char *gbaVariable(const char *key)
{
    if (!key) return NULL;
    if (!strcmp(key, "gpsp_drc")) return "enabled";
    /* AURORA_GBA_AUDIO_V2: GBA 硬件原生 32768 Hz。默认的 65536 会被
     * 前端线性降采样到 48 kHz, 高频混叠折叠成可闻破音; 32768 改为
     * 上采样, 无混叠, 且重采样工作量减半。 */
    if (!strcmp(key, "gpsp_sound_rate")) return "32768";
    if (!strcmp(key, "gpsp_rtc_time_source")) return "realtime";
    return NULL;
}

static bool gbaEnvironment(unsigned cmd, void *data)
{
    switch (cmd)
    {
        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
            if (!data) return false;
            ((struct retro_log_callback *)data)->log = gbaLog; return true;
        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
            /* gpSP still negotiates RGB565, but with USE_XBGR1555_FORMAT
             * (its PS2 platform) the delivered surface is GBA-native
             * BGR555. Accept and reinterpret, exactly like the PS2
             * RetroArch port. */
            return data && *(enum retro_pixel_format *)data == RETRO_PIXEL_FORMAT_RGB565;
        case RETRO_ENVIRONMENT_GET_VARIABLE:
        {
            struct retro_variable *v = (struct retro_variable *)data;
            if (!v || !v->key) return false;
            v->value = gbaVariable(v->key); return v->value != NULL;
        }
        case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
            if (!data) return false; *(bool *)data = false; return true;
        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
            /* gba_bios.bin lives under GBAticle/SYSTEM; the core falls
             * back to its built-in open-source BIOS automatically. */
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
        case RETRO_ENVIRONMENT_SET_MESSAGE:
        {
            const struct retro_message *m = (const struct retro_message *)data;
            if (m && m->msg) GbaBridgeMark(1, "msg: %s", m->msg);
            return true;
        }
        case RETRO_ENVIRONMENT_SET_MESSAGE_EXT:
        {
            const struct retro_message_ext *m = (const struct retro_message_ext *)data;
            if (m && m->msg) GbaBridgeMark(2, "msg: %s", m->msg);
            return true;
        }
        case RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER:
        case RETRO_ENVIRONMENT_GET_VFS_INTERFACE:
        case RETRO_ENVIRONMENT_GET_PERF_INTERFACE:
        case RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE:
        case RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION:
            return false;
        case RETRO_ENVIRONMENT_SET_VARIABLES:
        case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
        case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
        case RETRO_ENVIRONMENT_SET_MEMORY_MAPS:
        case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
        case RETRO_ENVIRONMENT_SET_GEOMETRY:
        case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO:
        case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:
        case RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS:
        case RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE:
        case RETRO_ENVIRONMENT_SET_MINIMUM_AUDIO_LATENCY:
        case RETRO_ENVIRONMENT_SET_AUDIO_BUFFER_STATUS_CALLBACK:
        case RETRO_ENVIRONMENT_SET_FASTFORWARDING_OVERRIDE:
        case RETRO_ENVIRONMENT_SET_NETPACKET_INTERFACE:
            return true;
        default: return false;
    }
}

static void gbaVideoRefresh(const void *data, unsigned w, unsigned h, size_t pitch)
{
    if (!data || !w || !h || pitch < w * 2) { s_HaveVideo = false; return; }
    s_VideoData = data; s_VideoW = w; s_VideoH = h; s_VideoPitch = pitch; s_HaveVideo = true;
}

static void gbaAudioSample(int16_t l, int16_t r)
{
    if (!s_pMix) return;
    Int16 L = (Int16)l, R = (Int16)r; s_pMix->OutputSamplesStereo(&L, &R, 1);
}

enum { GBA_AUDIO_CHUNK = 1024 };
static Int16 s_AudioL[GBA_AUDIO_CHUNK], s_AudioR[GBA_AUDIO_CHUNK];

static size_t gbaAudioBatch(const int16_t *data, size_t frames)
{
    if (!data || !frames || !s_pMix) return frames;
    if (_AudMix && s_pMix == _AudMix && frames <= 0x7fffffffU)
    {
        _AudMix->OutputLibretroInterleaved(
            (const Int16 *)data, (Int32)frames);
        return frames;
    }
    size_t pos = 0;
    while (pos < frames)
    {
        size_t n = frames - pos; if (n > GBA_AUDIO_CHUNK) n = GBA_AUDIO_CHUNK;
        for (size_t i = 0; i < n; ++i)
        {
            s_AudioL[i] = (Int16)data[(pos + i) * 2 + 0];
            s_AudioR[i] = (Int16)data[(pos + i) * 2 + 1];
        }
        s_pMix->OutputSamplesStereo(s_AudioL, s_AudioR, (Int32)n); pos += n;
    }
    return frames;
}

static void gbaInputPoll(void) {}
static bool gbaPadHas(Uint16 p, Uint16 bit) { return p != EMUSYS_DEVICE_DISCONNECTED && (p & bit); }

static int16_t gbaJoyMask(unsigned port)
{
    if (!s_pInput || port != 0) return 0;
    Uint16 p = s_pInput->uPad[0]; if (p == EMUSYS_DEVICE_DISCONNECTED) return 0;
    unsigned m = 0;
    /* Retropad A/B are physically the GBA A/B positions on a
     * SNES-layout pad (A right, B bottom), so the identity mapping is
     * also the ergonomically correct one. L/R shoulder pass through. */
    if (gbaPadHas(p, SNESIO_JOY_UP))     m |= 1u << RETRO_DEVICE_ID_JOYPAD_UP;
    if (gbaPadHas(p, SNESIO_JOY_DOWN))   m |= 1u << RETRO_DEVICE_ID_JOYPAD_DOWN;
    if (gbaPadHas(p, SNESIO_JOY_LEFT))   m |= 1u << RETRO_DEVICE_ID_JOYPAD_LEFT;
    if (gbaPadHas(p, SNESIO_JOY_RIGHT))  m |= 1u << RETRO_DEVICE_ID_JOYPAD_RIGHT;
    if (gbaPadHas(p, SNESIO_JOY_A))      m |= 1u << RETRO_DEVICE_ID_JOYPAD_A;
    if (gbaPadHas(p, SNESIO_JOY_B))      m |= 1u << RETRO_DEVICE_ID_JOYPAD_B;
    if (gbaPadHas(p, SNESIO_JOY_L))      m |= 1u << RETRO_DEVICE_ID_JOYPAD_L;
    if (gbaPadHas(p, SNESIO_JOY_R))      m |= 1u << RETRO_DEVICE_ID_JOYPAD_R;
    if (gbaPadHas(p, SNESIO_JOY_SELECT)) m |= 1u << RETRO_DEVICE_ID_JOYPAD_SELECT;
    if (gbaPadHas(p, SNESIO_JOY_START))  m |= 1u << RETRO_DEVICE_ID_JOYPAD_START;
    return (int16_t)m;
}

static int16_t gbaInputState(unsigned port, unsigned device, unsigned index, unsigned id)
{
    (void)index;
    if (device != RETRO_DEVICE_JOYPAD || port != 0) return 0;
    int16_t mask = gbaJoyMask(port);
    if (id == RETRO_DEVICE_ID_JOYPAD_MASK) return mask;
    if (id > RETRO_DEVICE_ID_JOYPAD_R3) return 0;
    return (mask & (1u << id)) ? 1 : 0;
}

static inline Uint32 gba555(Uint16 c)
{
    /* GBA BGR555: rrrrrgggggbbbbb (LSB-aligned channels).
     * Aurora RGBA word: AABBGGRR. */
    return 0xff000000u |
           s_GbaR[c & 31u] |
           s_GbaG[(c >> 5) & 31u] |
           s_GbaB[(c >> 10) & 31u];
}

static void gbaRender(CRenderSurface *target)
{
    if (!target || !s_HaveVideo || !s_VideoData) return;

    PixelFormatT *fmt = target->GetFormat();
    if (!fmt || fmt->uBitDepth != 32) return;

    gbaInitColorTables();

    /* AURORA_GBA_SCALE_V2: 可视逻辑区域是 256x240; 表面本身 256x256,
     * 240 以下的内容会被屏幕裁掉, 必须 clamp。 */
    int tw = (int)target->GetWidth();
    int th = (int)target->GetHeight();
    if (tw > 256) tw = 256;
    if (th > 240) th = 240;
    if (tw <= 0 || th <= 0) return;

    /* GBA is 240x160 (3:2). All rectangles are computed in logical
     * coordinates but constrained to the desired *physical* aspect:
     * physical ratio of a w x h logical rect is (w/h) * pixelAspect. */
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
        /* want = 期望的逻辑宽高比 (物理比例 / 像素比) */
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
            /* 等比放大 / 4:3 / 16:9: 充满目标区域内的最大正确矩形 */
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
                dst[dstX + x] = gba555(src[fx >> 16]);
        }
        for (int x = dstX + dstW; x < tw; ++x) dst[x] = black;
    }
}

Int32 GbaBridge_GetScalingMode(void) { return s_ScaleMode; }
void GbaBridge_SetScalingMode(Int32 mode)
{
    if (mode < 0 || mode > 4) mode = 0; /* AURORA_GBA_SCALE_V4: 0..4 */
    s_ScaleMode = mode;
}

bool GbaBridge_Init(void)
{
    if (s_Initialized) return true;
    GbaBridgeMark(3, "Init start");
    GPSP_retro_set_environment(gbaEnvironment); GPSP_retro_set_video_refresh(gbaVideoRefresh);
    GPSP_retro_set_audio_sample(gbaAudioSample); GPSP_retro_set_audio_sample_batch(gbaAudioBatch);
    GPSP_retro_set_input_poll(gbaInputPoll); GPSP_retro_set_input_state(gbaInputState);
    GPSP_retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
    if (GPSP_retro_api_version()!=RETRO_API_VERSION) { printf("[GBA] libretro API mismatch\n"); return false; }
    GPSP_retro_init();
    GbaBridgeMark(4, "Init done");
    s_Initialized=true; return true;
}

void GbaBridge_Shutdown(void)
{
    if (!s_Initialized) return; GbaBridge_UnloadGame(); GPSP_retro_deinit(); s_Initialized=false;
}

static bool gbaLoadCartInternal(const char *path, const char *systemPath,
                                unsigned memRomBytes)
{
    struct retro_game_info info;
    struct retro_system_av_info av;

    if (!path || !*path ||
        strlen(path) >= sizeof(s_ContentPath) ||
        (systemPath && strlen(systemPath) >= sizeof(s_SystemPath)))
        return false;

    /* AURORA_GBA_LAUNCH_TIMING_V1: restart the mark clock each launch. */
    s_BridgeMarkT0 = time(NULL);

    if (s_CartLoaded)
        GbaBridge_UnloadGame();

    /* gpSP caches nothing from retro_init that depends on the system
     * directory, but keep the copy ahead of init for first boot. */
    if (systemPath && *systemPath)
        strcpy(s_SystemPath, systemPath);
    else
        strcpy(s_SystemPath, ".");

    if (!GbaBridge_Init())
        return false;

    /* AURORA_GBA_STATE_STATIC_V1_20260910: the state scratch lives in BSS
     * now (see _MainLoop_GbaStateData), so this pin allocates nothing.
     * It only drops the previous system's heap scratch -- keep it AFTER
     * retro_init() above, which has already carved the 1 MB-block ROM
     * cache (up to 16 blocks) and the frame buffer. */
    MainLoopStatePinGbaScratch();
    GbaBridgeMark(5, "LoadCart: %s", path);
    strcpy(s_ContentPath, path);
    memset(&info, 0, sizeof(info));
    info.path = s_ContentPath;
    info.data = NULL;
    info.size = 0;

    /* AURORA_GBA_ZIP_MEM_V1: memRomBytes > 0 arms the cache-resident load
     * (the frontend filled gpSP's ROM cache through GbaBridge_RomCacheSink).
     * The core consumes the flag one-shot; disarm again on this failure
     * path so a later path-based load never sees a stale arm. */
    if (memRomBytes)
        GPSP_gamepak_arm_memory_load(memRomBytes);

    if (!GPSP_retro_load_game(&info))
    {
        if (memRomBytes)
            GPSP_gamepak_arm_memory_load(0);
        printf("[GBA] retro_load_game failed: %s\n", s_ContentPath);
        return false;
    }
    GbaBridgeMark(6, "LoadCart game loaded");

    s_CartLoaded = true;
    {
        size_t sb = GPSP_retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
        void *sp = GPSP_retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
        s_pSramData = (sp && sb && sb <= 0x7fffffffU) ? (Uint8 *)sp : NULL;
        s_SramBytes = s_pSramData ? (Int32)sb : 0;
    }
    memset(&av, 0, sizeof(av));
    GPSP_retro_get_system_av_info(&av);
    if (av.timing.sample_rate >= 8000.0 && av.timing.sample_rate <= 96000.0)
        s_SampleRate = (unsigned)(av.timing.sample_rate + 0.5);

    GbaBridgeMark(7, "LoadCart done SRAM=%d", (int)s_SramBytes);
    printf("[GBA] loaded %s; SRAM=%d; audio=%u Hz\n",
           s_ContentPath, (int)s_SramBytes, s_SampleRate);
    return true;
}

bool GbaBridge_LoadCart(const char *path, const char *systemPath)
{
    return gbaLoadCartInternal(path, systemPath, 0);
}

/* AURORA_GBA_ZIP_MEM_V1_20260911 */
int GbaBridge_RomCacheWindowBytes(void)
{
    if (!GbaBridge_Init())
        return 0;
    return (int)GPSP_gamepak_cache_window_bytes();
}

size_t GbaBridge_RomCacheSink(void *opaque, unsigned long long file_ofs,
                              const void *buf, size_t n)
{
    (void)opaque;
    if (!buf || !n || file_ofs > 0xFFFFFFFFull)
        return 0;
    return (size_t)GPSP_gamepak_memory_sink(
        (unsigned)file_ofs, (const unsigned char *)buf, (unsigned)n);
}

bool GbaBridge_LoadCartMemory(const char *path, const char *systemPath,
                              unsigned romBytes)
{
    if (!romBytes)
        return false;
    return gbaLoadCartInternal(path, systemPath, romBytes);
}

void GbaBridge_UnloadGame(void)
{
    if (s_Initialized && s_CartLoaded) GPSP_retro_unload_game();
    s_CartLoaded=false; s_pInput=NULL; s_pMix=NULL; s_pSramData=NULL; s_SramBytes=0;
    s_VideoData=NULL; s_VideoW=s_VideoH=0; s_VideoPitch=0; s_HaveVideo=false;
}

bool GbaBridge_IsCartLoaded(void)
{
    return s_CartLoaded;
}

void GbaBridge_SyncSramRegion(void)
{
    /* Nothing to sync: gpSP keeps gamepak_backup resident and the
     * frontend reads it directly through GbaBridge_GetSRAMData(). */
}

void GbaBridge_Reset(void)
{
    if (s_CartLoaded) { GbaBridgeMark(8, "Reset"); GPSP_retro_reset(); }
}

void GbaBridge_RunFrame(Emu::SysInputT *input, CRenderSurface *target, CMixBuffer *mix)
{
    if (!s_CartLoaded)
        return;

    static bool sFirstFrameMarked = false;
    if (!sFirstFrameMarked) { sFirstFrameMarked = true; GbaBridgeMark(9, "RunFrame first"); }
    s_pInput = input;
    s_pMix = mix;
    s_HaveVideo = false;

    GPSP_retro_run();

    if (s_HaveVideo)
        gbaRender(target);

    if (s_pMix)
        s_pMix->Flush();

    s_pMix = NULL;
    s_pInput = NULL;
}

int GbaBridge_GetStateSize(void)
{
    if (!s_CartLoaded) return 0;
    size_t n = GPSP_retro_serialize_size();
    return n && n <= 0x7fffffffU ? (int)n : 0;
}

int GbaBridge_SaveState(void *data, int bytes)
{
    int n = GbaBridge_GetStateSize();
    return data && n > 0 && bytes >= n && GPSP_retro_serialize(data, (size_t)n) ? n : 0;
}

bool GbaBridge_LoadState(const void *data, int bytes)
{
    return data && bytes > 0 && GPSP_retro_unserialize(data, (size_t)bytes);
}

int GbaBridge_GetSRAMBytes(void) { return s_CartLoaded ? s_SramBytes : 0; }
Uint8 *GbaBridge_GetSRAMData(void) { return s_CartLoaded ? s_pSramData : NULL; }
unsigned GbaBridge_GetSampleRate(void) { return s_SampleRate; }

/* AURORA_GPSP_STATE_V1 */
bool GbaBridge_GetCartIdentity(Uint32 *pCRC, Uint32 *pBytes)
{
    if (!s_CartLoaded || !GPSP_gamepak_identity_size)
        return false;
    if (pCRC)
        *pCRC = (Uint32)GPSP_gamepak_identity_crc;
    if (pBytes)
        *pBytes = (Uint32)GPSP_gamepak_identity_size;
    return true;
}
