
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <kernel.h>
#include <libpad.h>
#include "types.h"
#include "font.h"
#include "poly.h"
#include "uiVideo.h"
#include "snrom.h"
#include "snes.h"
extern "C" {
#include "gskit_backend.h"
#include "gpfifo.h"
#include "ps2dma.h"
#include "audio.h"
}
#include "memcard.h"
#include "uiCover.h"
#include "mainloop_bgm.h"
#include "mainloop_state.h"
#include "mainloop_input.h"
#include "input.h"
#include "mainloop_smb.h"
#include "audmixbuffer.h"
#include "embedded_irx.h"   /* HddSupportIsEnabled / HddSupportSetEnabled */
#include "snppucolor.h"
#include "snppurender.h"
#include "nes/quicknes/quicknes_bridge.h" /* QUICKNES_FAMICLONE_HOOK */
#include "sega/picodrive/picodrive_bridge.h"
#include "pce/beetle/pce_bridge.h" /* AURORA_CD_MUSIC_REDBOOK_V3_20260830 */
#include "mainloop_safe_frameskip.h" /* AURORA_SAFE_FRAMESKIP_GG_ZOOM_V2_2 */
#include "gba/gpsp/gpsp_bridge.h"       /* AURORA_GBA_UI_V1 */
#include "gb/gambatte/gambatte_bridge.h"

/* mc0:/GBAticle (defined in mainloop_globals.cpp). */
extern Char _SramPath[256];
extern SnesSystem *_pSnes;
extern Emu::System *_pSystem;
extern AudMixBuffer *_AudMix;

void MainResetEmulator(void);
Bool MainLoopReinitVideoMode(Int32 mode);
/* AURORA_SNES9X2010_V1 */
const Char *MainLoopSnesCoreGetName();
void MainLoopSnesCoreCycleDir(Int32 dir);
/* AURORA_SNES9X2010_V3_MENU_BRIDGE_20260824 */
Int32 MainLoopSnesCoreGetPersisted();
void MainLoopSnesCoreSetPersisted(Int32 value);

/* ------------------------------------------------------------------ */
/* Persistence                                                         */
/* ------------------------------------------------------------------ */

#define VIDEOCFG_MAGIC   0x53564944u   /* 'SVID' */
#define VIDEOCFG_VERSION 47
/* AURORA_CD_MUSIC_REDBOOK_V3_20260830: v43 appends shared SCD/PCE CD Red Book toggle; old configs default On. */
/* AURORA_PCE_SCALING_LIGHTGUN_TOGGLE_V2_20260830: v42 appends Light Gun; old configs default On. */
/* AURORA_FAMICOM_MIC_CFG41_20260828: v41 keeps the layout and migrates every older Safe Frameskip to 1 once. */
/* AURORA_PCE_VOLUME_V37_20260823
 * v37 appends pcevol only; every v36 field keeps the same offset.
 * Internal 200 == UI 100. */
/* AURORA_AUDIO_SPLIT_VOLUMES_V36_20260823
 * v36 changes NO struct size or field offsets. The former legacy/reserved
 * snesaudiorate slot is now segavol; native SNES synthesis stays fixed 32k. */
/* v35 appends SMS color-border + SMS FM preferences. */
/* AURORA_AUDIO_SPLIT_REVIEW_V1_20260821 */
/* AURORA_PD_MEGA_FIX_20260820 */
/* AURORA_PICODRIVE_CURRENT_UI_V4: v32 appends md6button only. */
/* v31 establishes Menu Volume UI 100 (internal 200) as the migration
 * default for every older config. Once saved as v31+, the user's selected
 * Menu Volume persists normally. */
/* v30 fixes the temporary v29 Menu Audio defaults:
 * Menu Volume defaults to internal 200 (UI 100) and Menu Music to Off.
 * Once saved as v30, both settings persist independently. */
/* v29 changes Menu Volume semantics to match Game Volume:
 * internal 0..400, UI displays /2 (0..200). v28 is byte-compatible. */
/* v27 changes only Game Volume semantics: gamevol is now internal 0..400
 * and the UI displays gamevol/2. v26 remains byte-compatible and migrates. */
/* AURORA_AUG19_BUNDLE_V3: v26 appends Turbo Speed + CPU Overclock. */
/* AURORA_CONFIG_V24_STORAGE_OBJ_LIMIT_MODE
 * v24 appends OBJ limiter mode. v23 imports its limiter/storage fields and
 * defaults the new mode to Per Scanline; v21/v22 retain exact import. */
/* AURORA_V85_SOFTWARE_HACKS_CONFIG
 * v20 only appends renderer-hack preferences. Old configs migrate with all
 * hacks disabled and all layers enabled. */

typedef struct
{
	Uint32 magic;
	Int32  version;
	Int32  mode;
	Int32  offx;
	Int32  offy;
	Int32  overscan;
	Int32  widescreen;
	Int32  covers;
	Int32  bgmvol;     /* ganho interno da trilha: 0..400; UI /2 */
	Int32  bgmrate;    /* frequencia de sintese da trilha (Hz)     */
	Int32  gamevol;    /* SNES/QuickNES gain: 0..400; UI /2 */
	Int32  hddenable;  /* suporte ao HD interno (hdd0:): 0=off, 1=on  */
	Int32  mmceenable; /* suporte a MMCE (mmce0/1): 0=off, 1=on       */
	Int32  massenable; /* mass/USB (mass0/1): 0=off, 1=on             */
	Int32  smbenable;  /* historical host slot; now smb: 0=off, 1=on */
	Int32  mx4sioenable; /* MX4SIO (SD via SIO2): 0=off, 1=on         */
	Int32  colorprofile; /* SNPPU_COLOR_PROFILE_*                     */
	Int32  famicloneaudio;
	Int32  fakesramsize;
	Int32  forceregion;
	Int32  sneshacklayersoff;
	Int32  sneshackflags;
	Int32  compatflags;
	Int32  objlimit;
	Int32  sramdevice;
	Int32  objlimitmode;
	Int32  snesmousemode;
	Int32  turbospeed;
	Int32  cpuoverclock;
	Int32  bgmenable;   /* Menu Music: 0=off, 1=on */
	Int32  md6button;   /* bit0=6-button, bit1=BCA */
	Int32  bgmtrack;    /* 1..64 */
	Int32  mdrendering; /* 0=Fast, 1=Good, 2=Accurate */
	Int32  segavol;       /* v36: PicoDrive final gain 0..400; UI /2 */
	Int32  segaaudiorate; /* independent PicoDrive PCM rate */
	Int32  smscolorborder; /* 0=black, 1=SMS VDP backdrop */
	Int32  smsfm;          /* 0=off, 1=Master System YM2413/OPLL */
	Int32  pcevol;         /* v37: Beetle PCE Fast gain 0..400; UI /2 */
	Int32  snescore;       /* v38: persisted SNES core selector */
	Int32  safeframeskip;  /* v41: 0=Off, 1..9=max auto skips; default 1 */
	Int32  ggzoom;         /* v39: GG 160x144 -> 240x216, uniform 3:2 */
	Int32  lightgun;       /* v42: CRC-known NES/Famicom gun; 1=On */
	Int32  cdmusic;        /* v43: SCD/PCE CD Red Book CDDA; 1=On */
	Int32  ataexfat;       /* v44: HD interno exFAT/FAT (ata0:): 0=off, 1=on */
	Int32  gbascale;      /* v45: GBA scaling 0=原始 1=等比 2=拉伸 3=4:3 4=16:9 */
	Int32  gbscale;       /* v45: GB scaling 0=原始 1=等比 2=拉伸 3=4:3 4=16:9 */
	Int32  gbpalette;     /* v45: Gambatte palette 0..6 */
	Int32  gbmix;         /* v45: Gambatte LCD ghosting 0..3 */
	Int32  gbcolorcorr;   /* v45: Gambatte GBC colour correction 0..2 */
	Int32  padface;       /* v47: AURORA_PAD_FACE_REMAP face-button map byte */
} VideoCfgT;
#define VIDEOCFG_V46_BYTES (sizeof(VideoCfgT) - sizeof(Int32))
#define VIDEOCFG_V44_BYTES (sizeof(VideoCfgT) - 6 * sizeof(Int32))
#define VIDEOCFG_V43_BYTES (VIDEOCFG_V44_BYTES - sizeof(Int32))
#define VIDEOCFG_V42_BYTES (VIDEOCFG_V43_BYTES - sizeof(Int32))
#define VIDEOCFG_V38_BYTES (sizeof(VideoCfgT) - 9 * sizeof(Int32))
#define VIDEOCFG_V37_BYTES (VIDEOCFG_V38_BYTES - sizeof(Int32))

/* v16 is the exact prefix written by v1.0.4 and by the first video-fix
   test build. Keep it readable so installing this build never resets the
   user's mode, offsets, audio volumes or storage choices. */
typedef struct
{
	Uint32 magic;
	Int32  version;
	Int32  mode;
	Int32  offx;
	Int32  offy;
	Int32  overscan;
	Int32  widescreen;
	Int32  covers;
	Int32  bgmvol;
	Int32  bgmrate;
	Int32  gamevol;
	Int32  hddenable;
	Int32  mmceenable;
	Int32  massenable;
	Int32  hostenable;
	Int32  mx4sioenable;
} VideoCfgV16T;

typedef struct
{
	Uint32 magic;
	Int32  version;
	Int32  mode;
	Int32  offx;
	Int32  offy;
	Int32  overscan;
	Int32  widescreen;
	Int32  covers;
	Int32  bgmvol;
	Int32  bgmrate;
	Int32  gamevol;
	Int32  hddenable;
	Int32  mmceenable;
	Int32  massenable;
	Int32  smbenable;
	Int32  mx4sioenable;
	Int32  colorprofile;
} VideoCfgV17T;

typedef struct
{
	Uint32 magic;
	Int32  version;
	Int32  mode;
	Int32  offx;
	Int32  offy;
	Int32  overscan;
	Int32  widescreen;
	Int32  covers;
	Int32  bgmvol;
	Int32  bgmrate;
	Int32  gamevol;
	Int32  hddenable;
	Int32  mmceenable;
	Int32  massenable;
	Int32  smbenable;
	Int32  mx4sioenable;
	Int32  colorprofile;
	Int32  famicloneaudio;
} VideoCfgV18T;

/* Exact on-card layout written by v19. */
typedef struct
{
	Uint32 magic;
	Int32  version;
	Int32  mode;
	Int32  offx;
	Int32  offy;
	Int32  overscan;
	Int32  widescreen;
	Int32  covers;
	Int32  bgmvol;
	Int32  bgmrate;
	Int32  gamevol;
	Int32  hddenable;
	Int32  mmceenable;
	Int32  massenable;
	Int32  smbenable;
	Int32  mx4sioenable;
	Int32  colorprofile;
	Int32  famicloneaudio;
	Int32  fakesramsize;
	Int32  forceregion;
} VideoCfgV19T;

/* Exact on-card layout written by v20 (V8.5). */
typedef struct
{
	Uint32 magic;
	Int32  version;
	Int32  mode;
	Int32  offx;
	Int32  offy;
	Int32  overscan;
	Int32  widescreen;
	Int32  covers;
	Int32  bgmvol;
	Int32  bgmrate;
	Int32  gamevol;
	Int32  hddenable;
	Int32  mmceenable;
	Int32  massenable;
	Int32  smbenable;
	Int32  mx4sioenable;
	Int32  colorprofile;
	Int32  famicloneaudio;
	Int32  fakesramsize;
	Int32  forceregion;
	Int32  sneshacklayersoff;
	Int32  sneshackflags;
} VideoCfgV20T;

/* Exact byte layout written by v21 and v22. */
typedef struct
{
	Uint32 magic;
	Int32  version;
	Int32  mode;
	Int32  offx;
	Int32  offy;
	Int32  overscan;
	Int32  widescreen;
	Int32  covers;
	Int32  bgmvol;
	Int32  bgmrate;
	Int32  gamevol;
	Int32  hddenable;
	Int32  mmceenable;
	Int32  massenable;
	Int32  smbenable;
	Int32  mx4sioenable;
	Int32  colorprofile;
	Int32  famicloneaudio;
	Int32  fakesramsize;
	Int32  forceregion;
	Int32  sneshacklayersoff;
	Int32  sneshackflags;
	Int32  compatflags;
} VideoCfgV22T;


/* Exact byte layout written by Aurora video.cfg v24. */
typedef struct
{
	Uint32 magic;
	Int32  version;
	Int32  mode;
	Int32  offx;
	Int32  offy;
	Int32  overscan;
	Int32  widescreen;
	Int32  covers;
	Int32  bgmvol;
	Int32  bgmrate;
	Int32  gamevol;
	Int32  hddenable;
	Int32  mmceenable;
	Int32  massenable;
	Int32  smbenable;
	Int32  mx4sioenable;
	Int32  colorprofile;
	Int32  famicloneaudio;
	Int32  fakesramsize;
	Int32  forceregion;
	Int32  sneshacklayersoff;
	Int32  sneshackflags;
	Int32  compatflags;
	Int32  objlimit;
	Int32  sramdevice;
	Int32  objlimitmode;
} VideoCfgV24T;

/* Exact byte layout written by Aurora video.cfg v25. */
typedef struct
{
	VideoCfgV24T v24;
	Int32 snesmousemode;
} VideoCfgV25T;

/* Exact byte layout written by Aurora V1/V1.1 (video.cfg v23). */
typedef struct
{
	Uint32 magic;
	Int32  version;
	Int32  mode;
	Int32  offx;
	Int32  offy;
	Int32  overscan;
	Int32  widescreen;
	Int32  covers;
	Int32  bgmvol;
	Int32  bgmrate;
	Int32  gamevol;
	Int32  hddenable;
	Int32  mmceenable;
	Int32  massenable;
	Int32  smbenable;
	Int32  mx4sioenable;
	Int32  colorprofile;
	Int32  famicloneaudio;
	Int32  fakesramsize;
	Int32  forceregion;
	Int32  sneshacklayersoff;
	Int32  sneshackflags;
	Int32  compatflags;
	Int32  objlimit;
	Int32  sramdevice;
} VideoCfgV23T;

typedef struct
{
	Uint32 magic;
	Int32  version;
} VideoCfgHeaderT;

static void _VideoCfgPath(char *pOut)
{
	strcpy(pOut, _SramPath);
	strcat(pOut, "/video.cfg");
}

static Bool g_FamicloneAudio = FALSE;

/* AURORA_CD_MUSIC_REDBOOK_V3_20260830
 * Shared by Sega CD and PCE CD; Red Book/CDDA only. */
static Bool g_CdMusicEnabled = TRUE;

static void _VideoSetCdMusicEnabled(Bool enabled)
{
	g_CdMusicEnabled = enabled ? TRUE : FALSE;
	PicoDriveBridge_SetCdMusicEnabled(g_CdMusicEnabled ? true : false);
	PceBridge_SetCdMusicEnabled(g_CdMusicEnabled ? true : false);
	if (!g_CdMusicEnabled)
		MainLoopSafeFrameskipCancelCdAudioWindow();
}

/* AURORA_COMPAT_PAGE_V21
 * Zero flags = exact V8.5 host behavior. */
#define VIDEO_COMPAT_GS_FULL_CACHE   (1 << 0)
#define VIDEO_COMPAT_GIF_LONG_WAIT   (1 << 1)
#define VIDEO_COMPAT_AUDIO_SMALL_RPC (1 << 2)
#define VIDEO_COMPAT_AUDIO_DEEP_Q    (1 << 3)
#define VIDEO_COMPAT_ALL             0x0F

static Int32 g_VideoCompatFlags = 0;

static void _VideoApplyCompatFlags(Int32 flags)
{
	g_VideoCompatFlags = flags & VIDEO_COMPAT_ALL;
	GPFifoSetCompatFullCache(
		(g_VideoCompatFlags & VIDEO_COMPAT_GS_FULL_CACHE) ? TRUE : FALSE);
	DmaSetGifCompatLongWait(
		(g_VideoCompatFlags & VIDEO_COMPAT_GIF_LONG_WAIT) ? TRUE : FALSE);
	Aud_SetCompatSmallChunks(
		(g_VideoCompatFlags & VIDEO_COMPAT_AUDIO_SMALL_RPC) ? 1 : 0);
	Aud_SetCompatDeepQueue(
		(g_VideoCompatFlags & VIDEO_COMPAT_AUDIO_DEEP_Q) ? 1 : 0);
}

void VideoSettingsSave(void)
{
	VideoCfgT cfg;
	char      path[300];

	cfg.magic   = VIDEOCFG_MAGIC;
	cfg.version = VIDEOCFG_VERSION;
	cfg.mode    = g_GskVideoMode;
	cfg.offx    = g_GskDispOffX;
	cfg.offy    = g_GskDispOffY;
	cfg.overscan   = g_GskOverscan;
	cfg.widescreen = g_GskWidescreen;
	cfg.covers     = CoverIsEnabled() ? 1 : 0;
	cfg.bgmvol     = BgmGetVolume();
	cfg.bgmenable  = BgmIsEnabled() ? 1 : 0;
	cfg.bgmrate    = BgmGetRate();
	cfg.gamevol    = AudMixGameGetVolume();
	cfg.hddenable  = HddSupportIsEnabled() ? 1 : 0;
	cfg.mmceenable = MmceSupportIsEnabled() ? 1 : 0;
	cfg.massenable = MassStorageIsEnabled() ? 1 : 0;
	cfg.smbenable  = SmbSupportIsEnabled() ? 1 : 0;
	cfg.mx4sioenable = Mx4sioIsEnabled() ? 1 : 0;
		/* AURORA_SNES9X2010_V2_PS2LEAN_20260824: retain the v37 field/layout, but always persist default. */
		cfg.colorprofile = SNPPU_COLOR_PROFILE_ORIGINAL;
	cfg.famicloneaudio = g_FamicloneAudio ? 1 : 0;
	cfg.fakesramsize = g_FakeSRAMSize;
	cfg.forceregion = g_SnesForceRegion;
	cfg.sneshacklayersoff =
		(~SNPPURenderGetSoftwareLayerMask()) &
		(SNESPPU_MASK_BG1 | SNESPPU_MASK_BG2 | SNESPPU_MASK_BG3 |
		 SNESPPU_MASK_BG4 | SNESPPU_MASK_OBJ);
	/* Retire the old fixed SNES frameskip bit; index 36 no longer owns it. */
	cfg.sneshackflags =
		SNPPURenderGetSoftwareHackFlags() & ~SNPPU_HACK_FRAME_SKIP;
	cfg.compatflags = g_VideoCompatFlags & VIDEO_COMPAT_ALL;
	cfg.objlimit = SNPPURenderGetObjLimitLevel();
	cfg.sramdevice = MainLoopSramGetDevice();
	cfg.objlimitmode = SNPPURenderGetObjLimitMode();
	cfg.snesmousemode = (Int32)InputSnesMouseGetMode();
	cfg.turbospeed = (Int32)MainLoopTurboGetSpeed();
	cfg.cpuoverclock = SNCPU_OVERCLOCK_OFF;
	/* AURORA_MD_PAD_LAYOUT_V1: keep video.cfg v32 byte-identical. */
	cfg.md6button =
		(PicoDriveBridge_Get6Button() ? 1 : 0) |
		(MainLoopMdPadGetLayout() == MAINLOOP_MD_PAD_BCA ? 2 : 0);
	cfg.bgmtrack = BgmGetTrackIndex();
	cfg.mdrendering = PicoDriveBridge_GetRenderingMode();
	cfg.segavol = AudMixSegaGetVolume();
	cfg.pcevol = AudMixPceGetVolume();
	cfg.segaaudiorate = (Int32)PicoDriveBridge_GetAudioRate();
	cfg.smscolorborder = PicoDriveBridge_GetSmsColorBorder() ? 1 : 0;
	cfg.smsfm = PicoDriveBridge_GetSmsFm() ? 1 : 0;
	/* AURORA_SNES9X2010_V3_MENU_BRIDGE_20260824 */
	cfg.snescore = MainLoopSnesCoreGetPersisted();
	/* AURORA_SAFE_FRAMESKIP_GG_ZOOM_V2_2: v39 append-only fields. */
	cfg.safeframeskip = MainLoopSafeFrameskipGetLevel();
	cfg.ggzoom = PicoDriveBridge_GetGgZoom() ? 1 : 0;
	cfg.lightgun = QuicknesBridge_GetLightGunEnabled() ? 1 : 0;
	cfg.cdmusic = g_CdMusicEnabled ? 1 : 0; /* AURORA_CD_MUSIC_REDBOOK_V3_20260830 */
	cfg.ataexfat = AtaBdSupportIsEnabled() ? 1 : 0; /* v44: HD interno exFAT */
	/* AURORA_GBA_UI_V1: v45 append-only fields. */
	cfg.gbascale = GbaBridge_GetScalingMode();
	cfg.gbscale = GbBridge_GetScalingMode();
	cfg.gbpalette = GbBridge_GetPaletteMode();
	cfg.gbmix = GbBridge_GetMixFrames();
	cfg.gbcolorcorr = GbBridge_GetColorCorrection();
	/* AURORA_PAD_FACE_REMAP_V1: v47 append-only field. */
	cfg.padface = (Int32)MainLoopPadFaceGetMap();
	_VideoCfgPath(path);
	BgmIOBegin();
	MemCardWriteFile(path, (Uint8 *)&cfg, sizeof(cfg));
	BgmIOEnd();
}

void VideoSettingsLoad(void)
{
	VideoCfgT cfg;
	VideoCfgV16T oldcfg;
	VideoCfgHeaderT header;
	char      path[300];
	Bool      loaded = FALSE;


	/* AURORA_SNES9X2010_V3_MENU_BRIDGE_20260824 */
	MainLoopSnesCoreSetPersisted(0);
	/* AURORA_FAMICOM_MIC_CFG41_20260828: conservative Aurora default. */
	MainLoopSafeFrameskipSetLevel(1);
	PicoDriveBridge_SetGgZoom(false);
	/* AURORA_GBA_UI_V1: v45 runtime defaults before any config import. */
	GbaBridge_SetScalingMode(0);
	GbBridge_SetScalingMode(0);
	GbBridge_SetPaletteMode(0);
	GbBridge_SetMixFrames(0);
	GbBridge_SetColorCorrection(1);
	/* AURORA_PAD_FACE_REMAP_V1: v47 default = historical layout
	   (Cross=B, Circle=A, Triangle=X, Square=Y). */
	MainLoopPadFaceSetMap(0xE1);
	cfg.gbascale = 0;
	cfg.gbscale = 0;
	cfg.gbpalette = 0;
	cfg.gbmix = 0;
	cfg.gbcolorcorr = 1;
	memset(&cfg, 0, sizeof(cfg));
	cfg.lightgun = 1; /* v42 default and all pre-v42 migrations: On */
	cfg.cdmusic = 1;  /* AURORA_CD_MUSIC_REDBOOK_V3_20260830: pre-v43 default On */
	QuicknesBridge_SetLightGunEnabled(true);
	_VideoSetCdMusicEnabled(TRUE);
	SNPPURenderSetSoftwareLayerMask(
		SNESPPU_MASK_BG1 | SNESPPU_MASK_BG2 | SNESPPU_MASK_BG3 |
		SNESPPU_MASK_BG4 | SNESPPU_MASK_OBJ);
	SNPPURenderSetSoftwareHackFlags(SNPPU_HACK_MODE7_HALF);
	SNPPURenderSetObjLimitLevel(SNPPU_OBJ_LIMIT_OFF);
	SNPPURenderSetObjLimitMode(SNPPU_OBJ_LIMIT_MODE_SCANLINE);
	MainLoopSramSetDevice(MAINLOOP_SRAMDEVICE_AUTO);
	InputSnesMouseSetMode(INPUT_SNES_MOUSE_OFF);
	MainLoopTurboSetSpeed(MAINLOOP_TURBO_SPEED_NORMAL);
	SNCPUSetOverclockLevel(_pSnes ? _pSnes->GetCpu() : NULL, SNCPU_OVERCLOCK_OFF);
	_VideoApplyCompatFlags(0);
	_VideoCfgPath(path);

	memset(&header, 0, sizeof(header));
	if (MemCardReadFile(path, (Uint8 *)&header, sizeof(header)) &&
	    header.magic == VIDEOCFG_MAGIC)
	{
		if (header.version == VIDEOCFG_VERSION)
		{
			loaded = MemCardReadFile(path, (Uint8 *)&cfg, sizeof(cfg));
		}
		else if (header.version == 46)
		{
			/* AURORA_PAD_FACE_REMAP_V1: v46 is the exact v47 prefix
			   before padface; default map stays 0xE1. */
			loaded = MemCardReadFile(path, (Uint8 *)&cfg, VIDEOCFG_V46_BYTES);
			if (loaded) cfg.version = VIDEOCFG_VERSION;
		}
		else if (header.version == 44)
		{
			/* v44 is the exact v45 prefix before the five GBA/GB fields. */
			loaded = MemCardReadFile(path, (Uint8 *)&cfg, VIDEOCFG_V44_BYTES);
			if (loaded) cfg.version = VIDEOCFG_VERSION;
		}
		else if (header.version == 43)
		{
			/* v43 is the exact v44 prefix before ataexfat; default stays Off. */
			loaded = MemCardReadFile(path, (Uint8 *)&cfg, VIDEOCFG_V43_BYTES);
			if (loaded) cfg.version = VIDEOCFG_VERSION;
		}
		else if (header.version == 42)
		{
			/* AURORA_CD_MUSIC_REDBOOK_V3_20260830
			 * v42 is the exact v43 prefix before cdmusic; default stays On. */
			loaded = MemCardReadFile(path, (Uint8 *)&cfg, VIDEOCFG_V42_BYTES);
			if (loaded) cfg.version = VIDEOCFG_VERSION;
		}
		else if (header.version == 41 || header.version == 40 || header.version == 39)
		{
			/* v39/v40/v41 are the exact v42 prefix before lightgun.
			 * lightgun and cdmusic were preinitialised On. */
			loaded = MemCardReadFile(path, (Uint8 *)&cfg,
			                         VIDEOCFG_V42_BYTES - sizeof(cfg.lightgun));
			if (loaded) cfg.version = VIDEOCFG_VERSION;
		}
		else if (header.version == 38)
		{
			/* v38 is the exact prefix before the two v39 fields. */
			loaded = MemCardReadFile(path, (Uint8 *)&cfg, VIDEOCFG_V38_BYTES);
			if (loaded) cfg.version = VIDEOCFG_VERSION;
		}
		else if (header.version == 37)
		{
			/* v37 is the exact prefix before snescore. */
			loaded = MemCardReadFile(path, (Uint8 *)&cfg, VIDEOCFG_V37_BYTES);
			if (loaded) cfg.version = VIDEOCFG_VERSION;
		}
		else if (header.version == 36)
		{
			/* v36 is exactly the v37 prefix without the appended PCE volume. */
			loaded = MemCardReadFile(path, (Uint8 *)&cfg,
			        VIDEOCFG_V37_BYTES - sizeof(cfg.pcevol));
			if (loaded) cfg.version = VIDEOCFG_VERSION;
		}
		else if (header.version == 35)
		{
			/* v35 is byte-identical in size; this slot used to be the
			 * legacy SNES rate value and is migrated below. */
			loaded = MemCardReadFile(path, (Uint8 *)&cfg, (VIDEOCFG_V37_BYTES - sizeof(cfg.pcevol)));
			if (loaded) cfg.version = VIDEOCFG_VERSION;
		}
		else if (header.version == 34)
		{
			loaded = MemCardReadFile(path, (Uint8 *)&cfg,
			        (VIDEOCFG_V37_BYTES - sizeof(cfg.pcevol)) - sizeof(cfg.smscolorborder) - sizeof(cfg.smsfm));
			if (loaded) cfg.version = VIDEOCFG_VERSION;
		}
		else if (header.version == 33)
		{
			loaded = MemCardReadFile(path, (Uint8 *)&cfg,
			        ((VIDEOCFG_V37_BYTES - sizeof(cfg.pcevol)) - sizeof(cfg.smscolorborder) - sizeof(cfg.smsfm)) - sizeof(cfg.segavol) - sizeof(cfg.segaaudiorate));
			if (loaded) cfg.version = VIDEOCFG_VERSION;
		}
		else if (header.version == 32)
		{
			loaded = MemCardReadFile(path, (Uint8 *)&cfg,
			        ((VIDEOCFG_V37_BYTES - sizeof(cfg.pcevol)) - sizeof(cfg.smscolorborder) - sizeof(cfg.smsfm)) - sizeof(cfg.segavol) - sizeof(cfg.segaaudiorate) - sizeof(cfg.bgmtrack) - sizeof(cfg.mdrendering));
			if (loaded) cfg.version = VIDEOCFG_VERSION;
		}
		else if (header.version == 31)
		{
			/* AURORA_PICODRIVE_CURRENT_CFG_V32 */
			memset(&cfg, 0, ((VIDEOCFG_V37_BYTES - sizeof(cfg.pcevol)) - sizeof(cfg.smscolorborder) - sizeof(cfg.smsfm)));
			loaded = MemCardReadFile(path, (Uint8 *)&cfg,
			        ((VIDEOCFG_V37_BYTES - sizeof(cfg.pcevol)) - sizeof(cfg.smscolorborder) - sizeof(cfg.smsfm)) - sizeof(cfg.segavol) - sizeof(cfg.segaaudiorate) - sizeof(cfg.md6button) - sizeof(cfg.bgmtrack) - sizeof(cfg.mdrendering));
			if (loaded) cfg.version = VIDEOCFG_VERSION;
		}
		else if (header.version == 30)
		{
			/* v30 has the same byte layout as v31. */
			loaded = MemCardReadFile(path, (Uint8 *)&cfg, ((VIDEOCFG_V37_BYTES - sizeof(cfg.pcevol)) - sizeof(cfg.smscolorborder) - sizeof(cfg.smsfm)) - sizeof(cfg.segavol) - sizeof(cfg.segaaudiorate) - sizeof(cfg.md6button) - sizeof(cfg.bgmtrack) - sizeof(cfg.mdrendering));
			if (loaded) cfg.version = VIDEOCFG_VERSION;
		}
		else if (header.version == 29)
		{
			/* v29 was a temporary test config with broken menu-audio defaults.
			 * Import its layout, then reset only those two settings once. */
			loaded = MemCardReadFile(path, (Uint8 *)&cfg, ((VIDEOCFG_V37_BYTES - sizeof(cfg.pcevol)) - sizeof(cfg.smscolorborder) - sizeof(cfg.smsfm)) - sizeof(cfg.segavol) - sizeof(cfg.segaaudiorate) - sizeof(cfg.md6button) - sizeof(cfg.bgmtrack) - sizeof(cfg.mdrendering));
			if (loaded)
			{
				cfg.version = VIDEOCFG_VERSION;
				cfg.bgmvol = 200;   /* UI 100 */
				cfg.bgmenable = 0;  /* Off */
			}
		}
		else if (header.version == 28)
		{
			/* v28 has the same byte layout; only Menu Volume semantics changed. */
			loaded = MemCardReadFile(path, (Uint8 *)&cfg, ((VIDEOCFG_V37_BYTES - sizeof(cfg.pcevol)) - sizeof(cfg.smscolorborder) - sizeof(cfg.smsfm)) - sizeof(cfg.segavol) - sizeof(cfg.segaaudiorate) - sizeof(cfg.md6button) - sizeof(cfg.bgmtrack) - sizeof(cfg.mdrendering));
			if (loaded) cfg.version = VIDEOCFG_VERSION;
		}
		else if (header.version == 27)
		{
			/* v27 is the v28/v29 prefix without bgmenable. */
			loaded = MemCardReadFile(path, (Uint8 *)&cfg, ((VIDEOCFG_V37_BYTES - sizeof(cfg.pcevol)) - sizeof(cfg.smscolorborder) - sizeof(cfg.smsfm)) - sizeof(cfg.segavol) - sizeof(cfg.segaaudiorate) - sizeof(cfg.md6button) - sizeof(cfg.bgmtrack) - sizeof(cfg.mdrendering) - sizeof(Int32));
			if (loaded) cfg.version = VIDEOCFG_VERSION;
		}
		else if (header.version == 26)
		{
			/* v26 has the same byte layout; only Game Volume semantics changed. */
			loaded = MemCardReadFile(path, (Uint8 *)&cfg, ((VIDEOCFG_V37_BYTES - sizeof(cfg.pcevol)) - sizeof(cfg.smscolorborder) - sizeof(cfg.smsfm)) - sizeof(cfg.segavol) - sizeof(cfg.segaaudiorate) - sizeof(cfg.md6button) - sizeof(cfg.bgmtrack) - sizeof(cfg.mdrendering) - sizeof(Int32));
			if (loaded) cfg.version = VIDEOCFG_VERSION;
		}
		else if (header.version == 25)
		{
			VideoCfgV25T oldcfg25;
			memset(&oldcfg25, 0, sizeof(oldcfg25));
			if (MemCardReadFile(path, (Uint8 *)&oldcfg25, sizeof(oldcfg25)))
			{
				memcpy(&cfg, &oldcfg25, sizeof(oldcfg25));
				cfg.version = VIDEOCFG_VERSION;
				switch (oldcfg25.snesmousemode)
				{
					case 1: cfg.snesmousemode = INPUT_SNES_MOUSE_CONTROLLER; break;
					case 3: cfg.snesmousemode = INPUT_SNES_MOUSE_USB; break;
					default: cfg.snesmousemode = INPUT_SNES_MOUSE_OFF; break;
				}
				cfg.turbospeed = MAINLOOP_TURBO_SPEED_NORMAL;
				cfg.cpuoverclock = SNCPU_OVERCLOCK_OFF;
				loaded = TRUE;
			}
		}
		else if (header.version == 24)
		{
			VideoCfgV24T oldcfg24;
			memset(&oldcfg24, 0, sizeof(oldcfg24));
			if (MemCardReadFile(path, (Uint8 *)&oldcfg24, sizeof(oldcfg24)))
			{
				memcpy(&cfg, &oldcfg24, sizeof(oldcfg24));
				cfg.version = VIDEOCFG_VERSION;
				cfg.snesmousemode = INPUT_SNES_MOUSE_OFF;
				loaded = TRUE;
			}
		}
		else if (header.version == 23)
		{
			VideoCfgV23T oldcfg23;
			memset(&oldcfg23, 0, sizeof(oldcfg23));
			if (MemCardReadFile(path, (Uint8 *)&oldcfg23, sizeof(oldcfg23)))
			{
				memcpy(&cfg, &oldcfg23, sizeof(oldcfg23));
				cfg.version = VIDEOCFG_VERSION;
				cfg.objlimitmode = SNPPU_OBJ_LIMIT_MODE_SCANLINE;
				loaded = TRUE;
			}
		}
		else if (header.version == 22 || header.version == 21)
		{
			VideoCfgV22T oldcfg22;
			memset(&oldcfg22, 0, sizeof(oldcfg22));
			if (MemCardReadFile(path, (Uint8 *)&oldcfg22, sizeof(oldcfg22)))
			{
				memcpy(&cfg, &oldcfg22, sizeof(oldcfg22));
				cfg.version = VIDEOCFG_VERSION;
				cfg.objlimit = SNPPU_OBJ_LIMIT_OFF;
				cfg.sramdevice = MAINLOOP_SRAMDEVICE_AUTO;
				cfg.objlimitmode = SNPPU_OBJ_LIMIT_MODE_SCANLINE;
				loaded = TRUE;
			}
		}
		else if (header.version == 20)
		{
			VideoCfgV20T oldcfg20;
			memset(&oldcfg20, 0, sizeof(oldcfg20));
			if (MemCardReadFile(path, (Uint8 *)&oldcfg20, sizeof(oldcfg20)))
			{
				memcpy(&cfg, &oldcfg20, sizeof(oldcfg20));
				cfg.version = VIDEOCFG_VERSION;
				cfg.compatflags = 0;
				loaded = TRUE;
			}
		}
		else if (header.version == 19)
		{
			VideoCfgV19T oldcfg19;

			memset(&oldcfg19, 0, sizeof(oldcfg19));
			if (MemCardReadFile(path, (Uint8 *)&oldcfg19, sizeof(oldcfg19)))
			{
				memcpy(&cfg, &oldcfg19, sizeof(oldcfg19));
				cfg.version = VIDEOCFG_VERSION;
				cfg.sneshacklayersoff = 0;
				cfg.sneshackflags = 0;
				loaded = TRUE;
			}
		}
		else if (header.version == 18)
		{
			VideoCfgV18T oldcfg18;

			memset(&oldcfg18, 0, sizeof(oldcfg18));
			if (MemCardReadFile(path, (Uint8 *)&oldcfg18, sizeof(oldcfg18)))
			{
				memcpy(&cfg, &oldcfg18, sizeof(oldcfg18));
				cfg.version = VIDEOCFG_VERSION;
				cfg.fakesramsize = 0;
				cfg.forceregion = SNES_FORCE_REGION_OFF;
				loaded = TRUE;
			}
		}
		else if (header.version == 17)
		{
	VideoCfgV17T oldcfg17;

	memset(&oldcfg17, 0, sizeof(oldcfg17));
	if (MemCardReadFile(path, (Uint8 *)&oldcfg17, sizeof(oldcfg17)))
	{
		memcpy(&cfg, &oldcfg17, sizeof(oldcfg17));
		cfg.version = VIDEOCFG_VERSION;
		cfg.famicloneaudio = 0;
		loaded = TRUE;
	}
}
		else if (header.version == 16)
		{
			memset(&oldcfg, 0, sizeof(oldcfg));
			if (MemCardReadFile(path, (Uint8 *)&oldcfg, sizeof(oldcfg)))
			{
				/* VideoCfgT only appends colorprofile to the v16 prefix. */
				memcpy(&cfg, &oldcfg, sizeof(oldcfg));
				cfg.version = VIDEOCFG_VERSION;
				cfg.colorprofile = SNPPU_COLOR_PROFILE_ORIGINAL;
				loaded = TRUE;
			}
		}
	}

	/* v27 and older had no independent Menu Music switch.
	 * New default policy is Off; Menu Volume remains independent. */
	if (loaded && header.version <= 27)
		cfg.bgmenable = 0;

	/* v26 and older stored Game Volume as the old 0..100 UI value.
	 * Preserve the audible setting (old 100 -> internal 200 -> UI 100). */
	if (loaded && header.version <= 26 &&
	    cfg.gamevol >= 0 && cfg.gamevol <= 100)
		cfg.gamevol *= 2;

	/* AURORA_GBA_AUDIO_V2: v46 起游戏音量默认 100 (原样输出)。v45 及
	 * 更早的默认 200 会让响亮的 GBA/GB 音频持续硬削波 (破音)。
	 * 一次性钳位; 用户之后仍可自行调高。 */
	if (loaded && header.version <= 45 && cfg.gamevol > 100)
		cfg.gamevol = 100;

	/* v28 and older stored Menu Volume directly as 0..100.
	 * v29 uses the same convention as Game Volume:
	 * old 100 -> internal 200 -> UI still shows 100. */
	if (loaded && header.version <= 28 &&
	    cfg.bgmvol >= 0 && cfg.bgmvol <= 100)
		cfg.bgmvol *= 2;

	/* v31 establishes a clean Menu Volume default for every older config.
	 * UI 100 == internal 200. From v31 onward, preserve the saved value. */
	if (loaded && header.version <= 30)
		cfg.bgmvol = 200;

	if (loaded && header.version <= 32)
	{
		cfg.bgmtrack = 1;
		cfg.mdrendering = 1;
	}

	/* AURORA_AUDIO_SPLIT_VOLUMES_V36_20260823
	 * v35 and older had one final Game Volume shared by every core.
	 * Copy the already-migrated gamevol into SEGA volume so old configs keep
	 * exactly the same audible balance after upgrading. */
	if (loaded && header.version <= 35)
		cfg.segavol = cfg.gamevol;

	/* PCE used Game Volume through v36. Preserve exactly that audible
	 * level on upgrade: old UI 100/internal 200 -> PCE UI 100. */
	if (loaded && header.version <= 36)
		cfg.pcevol = cfg.gamevol;
	if (loaded && header.version <= 33)
		cfg.segaaudiorate = 16000;

	/* Preserve the pre-v35 behaviour when importing any old config. */
	if (loaded && header.version <= 34)
	{
		cfg.smscolorborder = 1;
		cfg.smsfm = 0;
	}

	/* The fixed 1-in-2 SNES frameskip is retired in v39. Clear it even
	 * when importing a v38 file that had the old Performance row On. */
	if (loaded)
		cfg.sneshackflags &= ~SNPPU_HACK_FRAME_SKIP;

	/* AURORA_FAMICOM_MIC_CFG41_20260828
	 * Preserve v42 Safe Frameskip when v43 only adds CD music.
	 * Older migrations retain the previous one-time conservative default. */
	if (loaded && header.version < 42)
		cfg.safeframeskip = 1;

	/* New policy applies exactly once to every pre-v22 config. Once the
	 * user saves v22, a manual Full selection remains persistent. */
	if (loaded && header.version < 22)
		cfg.sneshackflags |= SNPPU_HACK_MODE7_HALF;

	if (loaded && cfg.magic == VIDEOCFG_MAGIC)
	{
		if (cfg.snescore >= 0 && cfg.snescore < 2)
			MainLoopSnesCoreSetPersisted(cfg.snescore);
		/* v1.0.2 allowed both SIO2 storage hooks to be saved at once.
		   Prefer MMCE when importing such a legacy config; all new changes
		   are mutually exclusive in the setters below. */
		if (cfg.mmceenable == 1 && cfg.mx4sioenable == 1)
			cfg.mx4sioenable = 0;

		/* AURORA_GBA_VIDEO_V2: 240p re-enabled. */
		if (cfg.mode == GSK_VIDMODE_240P ||
		    cfg.mode == GSK_VIDMODE_480I ||
		    cfg.mode == GSK_VIDMODE_480P ||
		    cfg.mode == GSK_VIDMODE_720P ||
		    cfg.mode == GSK_VIDMODE_1080I)
			g_GskVideoMode = cfg.mode;
		else
			g_GskVideoMode = GSK_VIDMODE_480I;

		if (cfg.offx >= -64 && cfg.offx <= 64) g_GskDispOffX = cfg.offx;
		if (cfg.offy >= -64 && cfg.offy <= 64) g_GskDispOffY = cfg.offy;
		if (cfg.overscan >= 0 && cfg.overscan <= 100) g_GskOverscan = cfg.overscan;
		if (cfg.widescreen == 0 || cfg.widescreen == 1) g_GskWidescreen = cfg.widescreen;
		if (cfg.covers == 0 || cfg.covers == 1) CoverSetEnabled(cfg.covers ? TRUE : FALSE);
		if (cfg.smscolorborder == 0 || cfg.smscolorborder == 1)
			PicoDriveBridge_SetSmsColorBorder(cfg.smscolorborder != 0);
		if (header.version >= 39 && header.version <= VIDEOCFG_VERSION &&
		    cfg.safeframeskip >= 0 && cfg.safeframeskip <= 9)
			MainLoopSafeFrameskipSetLevel(cfg.safeframeskip);
		if (cfg.ggzoom == 0 || cfg.ggzoom == 1)
			PicoDriveBridge_SetGgZoom(cfg.ggzoom != 0);
		if (cfg.lightgun == 0 || cfg.lightgun == 1)
			QuicknesBridge_SetLightGunEnabled(cfg.lightgun != 0);
		if (cfg.cdmusic == 0 || cfg.cdmusic == 1)
			_VideoSetCdMusicEnabled(cfg.cdmusic ? TRUE : FALSE);
		if (cfg.smsfm == 0 || cfg.smsfm == 1)
			PicoDriveBridge_SetSmsFm(cfg.smsfm != 0);
		if (cfg.bgmvol >= 0 && cfg.bgmvol <= 400) BgmSetVolume(cfg.bgmvol);
		if (cfg.bgmenable == 0 || cfg.bgmenable == 1) BgmSetEnabled(cfg.bgmenable);
		if (cfg.bgmrate >= 8000 && cfg.bgmrate <= 48000)
			BgmSetRate(cfg.bgmrate);
		/* SNES is always native 32 kHz; v36 reuses the old persisted rate
		 * slot for SEGA volume without changing VideoCfgT size. */
		SnesAudioSetRate(SNSPCDSP_SAMPLERATE);
		if (cfg.segaaudiorate >= 8000 && cfg.segaaudiorate <= 48000)
			PicoDriveBridge_SetAudioRate(cfg.segaaudiorate);
		if (cfg.gamevol >= 0 && cfg.gamevol <= 400) AudMixGameSetVolume(cfg.gamevol);
		if (cfg.segavol >= 0 && cfg.segavol <= 400) AudMixSegaSetVolume(cfg.segavol);
		if (cfg.pcevol >= 0 && cfg.pcevol <= 400) AudMixPceSetVolume(cfg.pcevol);
		if (cfg.bgmtrack >= 1 && cfg.bgmtrack <= 64) BgmSetTrackIndex(cfg.bgmtrack);
		if (cfg.mdrendering >= 0 && cfg.mdrendering <= 2)
			PicoDriveBridge_SetRenderingMode(cfg.mdrendering);
		if (cfg.hddenable == 0 || cfg.hddenable == 1) HddSupportSetEnabled(cfg.hddenable);
		if (cfg.ataexfat == 0 || cfg.ataexfat == 1) AtaBdSupportSetEnabled(cfg.ataexfat);
		/* AURORA_GBA_UI_V1: v45 GBA/GB settings. */
		/* AURORA_GBA_SCALE_V4: 旧的整数倍模式(5..7)迁移到等比放大。 */
		if (cfg.gbascale >= 0 && cfg.gbascale <= 4)
			GbaBridge_SetScalingMode(cfg.gbascale);
		else if (cfg.gbascale >= 5 && cfg.gbascale <= 7)
			GbaBridge_SetScalingMode(1);
		if (cfg.gbscale >= 0 && cfg.gbscale <= 4)
			GbBridge_SetScalingMode(cfg.gbscale);
		else if (cfg.gbscale >= 5 && cfg.gbscale <= 7)
			GbBridge_SetScalingMode(1);
		if (cfg.gbpalette >= 0 && cfg.gbpalette <= 6)
			GbBridge_SetPaletteMode(cfg.gbpalette);
		if (cfg.gbmix >= 0 && cfg.gbmix <= 3)
			GbBridge_SetMixFrames(cfg.gbmix);
		if (cfg.gbcolorcorr >= 0 && cfg.gbcolorcorr <= 2)
			GbBridge_SetColorCorrection(cfg.gbcolorcorr);
		/* AURORA_PAD_FACE_REMAP_V1: v47; SetMap rejects invalid slots. */
		if (header.version >= 47)
			MainLoopPadFaceSetMap((Uint32)cfg.padface & 0xFFu);
		if (cfg.mmceenable == 0 || cfg.mmceenable == 1) MmceSupportSetEnabled(cfg.mmceenable);
		if (cfg.massenable == 0 || cfg.massenable == 1) MassStorageSetEnabled(cfg.massenable);
		if (cfg.smbenable == 0 || cfg.smbenable == 1) SmbSupportSetEnabled(cfg.smbenable);
		if (cfg.mx4sioenable == 0 || cfg.mx4sioenable == 1) Mx4sioSetEnabled(cfg.mx4sioenable);
		/* AURORA_SNES9X2010_V2_PS2LEAN_20260824: ignore legacy Composite configs; default is fixed. */
		SNPPUColorSetProfile(SNPPU_COLOR_PROFILE_ORIGINAL);
if (cfg.famicloneaudio == 0 || cfg.famicloneaudio == 1)
{
	g_FamicloneAudio = cfg.famicloneaudio ? TRUE : FALSE;
	QuicknesBridge_SetDutySwap(g_FamicloneAudio ? true : false);
}
		/* AURORA_SWC_FLOPPY_V5_20260831: legacy field ignored. */
		g_FakeSRAMSize = 0;
		if (cfg.forceregion == SNES_FORCE_REGION_OFF ||
		    cfg.forceregion == SNES_FORCE_REGION_NTSC_U ||
		    cfg.forceregion == SNES_FORCE_REGION_NTSC_J ||
		    cfg.forceregion == SNES_FORCE_REGION_PAL)
		{
			g_SnesForceRegion = cfg.forceregion;
			PicoDriveBridge_SetRegion(g_SnesForceRegion);
		}

		const Int32 uLayerBits =
			SNESPPU_MASK_BG1 | SNESPPU_MASK_BG2 | SNESPPU_MASK_BG3 |
			SNESPPU_MASK_BG4 | SNESPPU_MASK_OBJ;
		if ((cfg.sneshacklayersoff & ~uLayerBits) == 0)
			SNPPURenderSetSoftwareLayerMask(
				(Uint8)(uLayerBits & ~cfg.sneshacklayersoff));
		if ((cfg.sneshackflags & ~SNPPU_HACK_ALL) == 0)
			SNPPURenderSetSoftwareHackFlags((Uint8)cfg.sneshackflags);
		if (cfg.objlimit >= SNPPU_OBJ_LIMIT_OFF && cfg.objlimit < SNPPU_OBJ_LIMIT_NUM)
			SNPPURenderSetObjLimitLevel((Uint8)cfg.objlimit);
		if (cfg.objlimitmode >= SNPPU_OBJ_LIMIT_MODE_SCANLINE &&
		    cfg.objlimitmode < SNPPU_OBJ_LIMIT_MODE_NUM)
			SNPPURenderSetObjLimitMode((Uint8)cfg.objlimitmode);
		if (cfg.sramdevice >= MAINLOOP_SRAMDEVICE_AUTO &&
		    cfg.sramdevice < MAINLOOP_SRAMDEVICE_NUM)
			MainLoopSramSetDevice((MainLoopSramDeviceE)cfg.sramdevice);
		if ((cfg.compatflags & ~VIDEO_COMPAT_ALL) == 0)
			_VideoApplyCompatFlags(cfg.compatflags);
		if (cfg.snesmousemode >= INPUT_SNES_MOUSE_OFF &&
		    cfg.snesmousemode < INPUT_SNES_MOUSE_MODE_NUM)
			InputSnesMouseSetMode(
				(InputSnesMouseModeE)cfg.snesmousemode);
		if (cfg.turbospeed >= MAINLOOP_TURBO_SPEED_NORMAL &&
		    cfg.turbospeed < MAINLOOP_TURBO_SPEED_NUM)
			MainLoopTurboSetSpeed((MainLoopTurboSpeedE)cfg.turbospeed);
		/* AURORA_PICODRIVE_CURRENT_CFG_APPLY
		 * AURORA_MD_PAD_LAYOUT_V1: valores antigos 0/1 = ABC. */
		if (cfg.md6button >= 0 && cfg.md6button <= 3)
		{
			PicoDriveBridge_Set6Button((cfg.md6button & 1) != 0);
			MainLoopMdPadSetLayout(
				(cfg.md6button & 2) ? MAINLOOP_MD_PAD_BCA
				                     : MAINLOOP_MD_PAD_ABC);
		}
		SNCPUSetOverclockLevel(_pSnes ? _pSnes->GetCpu() : NULL,
		                         SNCPU_OVERCLOCK_OFF);
	}
}

/* AURORA_MOUSE_VIDEO_COMPAT_V1 */
/* SNES Mouse lives on Video Config 4/4 and uses Left/Right. */

/* ------------------------------------------------------------------ */
/* Screen                                                              */
/* ------------------------------------------------------------------ */

CVideoScreen::CVideoScreen()
{
	m_iSelect = 0;
}

void CVideoScreen::Process()
{
}

static void _VideoCenter(int x, int y, const char *pStr)
{
	FontPuts(x - FontGetStrWidth(pStr) / 2, y, pStr);
}

static void _VideoRow(int vy, int idx, int sel, const char *pLabel, const char *pValue)
{
	if (idx == sel)
	{
		PolyColor4f(0.0f, 0.5f, 0.0f, 0.5f);
		PolyRect(48, vy - 1, 160, FontGetHeight() + 2);
	}

	FontColor4f(0.5f, 0.5f, 0.5f, 1.0f);
	FontPuts(56, vy, pLabel);

	FontColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	FontPuts(150, vy, pValue);
}

static void _VideoHeader(int vy, const char *pStr)
{
	PolyColor4f(0.0f, 0.2f, 0.2f, 0.5f);
	PolyRect(32, vy, 256 - 64, 9);
	FontColor4f(0.0f, 0.8f, 0.8f, 1.0f);
	_VideoCenter(128, vy, pStr);
}

static const char *_VideoMmceStatus()
{
	int slots;

	if (!MmceSupportIsEnabled()) return "关";
	if (MmceNeedsRestart())      return "需重启";
	if (MmceGetLastError() < 0)  return "驱动错误";
	if (!MmceIsLoaded())         return "开";

	slots = MmceGetAvailableSlots();
	if (slots == 1) return "插槽1";
	if (slots == 2) return "插槽2";
	if (slots == 3) return "插槽1+2";
	return "未找到";
}

/* 内置硬盘三态：关 -> APA(hdd0:) -> exFAT/FAT(ata0:) -> 关。两种格式
   互斥（都用 atad library），切换已加载的栈需重启生效。 */
static const char *_VideoHddModeStatus()
{
	if (HddSupportIsEnabled())
		return HddNeedsRestart() ? "APA(需重启)" : "APA分区";
	if (AtaBdSupportIsEnabled())
	{
		if (AtaBdNeedsRestart())     return "exFAT(需重启)";
		if (AtaBdGetLastError() < 0) return "驱动错误";
		return "exFAT/FAT";
	}
	return "关";
}

static const char *_VideoSafeFrameskipStatus()
{
    static const char *const names[10] =
        { "关", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
    Int32 level = MainLoopSafeFrameskipGetLevel();
    if (level < 0 || level > 9) level = 1;
    return names[level];
}

static const char *_VideoFakeSRAMStatus()
{
   switch (g_FakeSRAMSize)
{
case 8:    return "1 KB";
case 16:   return "2 KB";
case 32:   return "4 KB";
case 64:   return "8 KB";
case 128:  return "16 KB";
case 256:  return "32 KB";
case 512:  return "64 KB";
case 1024: return "128 KB";
case 2048: return "256 KB";
default:   return "自动";
}
}

static const char *_VideoForceRegionStatus()
{
    switch (g_SnesForceRegion)
    {
        case SNES_FORCE_REGION_NTSC_U:
            return "NTSC-U";

        case SNES_FORCE_REGION_NTSC_J:
            return "NTSC-J";

        case SNES_FORCE_REGION_PAL:
            return "PAL";

        case SNES_FORCE_REGION_OFF:
        default:
            return "自动";
    }
}

static const char *_VideoFamicloneAudioStatus()
{
	return g_FamicloneAudio ? "开" : "关";
}

static const char *_VideoHackLayerStatus(Uint8 uLayer)
{
	return (SNPPURenderGetSoftwareLayerMask() & uLayer) ? "开" : "关";
}

static const char *_VideoHackAccurateStatus(Uint8 uFlag)
{
	return (SNPPURenderGetSoftwareHackFlags() & uFlag) ? "关" : "精确";
}

static const char *_VideoHackMode7Status()
{
	return (SNPPURenderGetSoftwareHackFlags() & SNPPU_HACK_MODE7_HALF)
		? "减半" : "完整";
}

static const char *_VideoHackSpriteLimiterStatus()
{
	/* AURORA_V15_MULTICORE_SPRITE_LIMIT_20260824
	 * SNES values are OBJ-tile budgets. Other machines use different native
	 * units/limits, so don't display a misleading SNES number there. */
	const Bool bSnesUnits = (_pSystem == _pSnes) ? TRUE : FALSE;
	switch (SNPPURenderGetObjLimitLevel())
	{
		case SNPPU_OBJ_LIMIT_LIGHT:   return bSnesUnits ? "轻(28)"   : "Light";
		case SNPPU_OBJ_LIMIT_MEDIUM:  return bSnesUnits ? "中(24)"  : "Medium";
		case SNPPU_OBJ_LIMIT_STRONG:  return bSnesUnits ? "强(20)"  : "Strong";
		case SNPPU_OBJ_LIMIT_EXTREME: return bSnesUnits ? "极强(16)" : "Extreme";
		case SNPPU_OBJ_LIMIT_HEAVY:   return bSnesUnits ? "加重(12)"   : "Heavy";
		case SNPPU_OBJ_LIMIT_INSANE:  return bSnesUnits ? "疯狂(8)"   : "Insane";
		default:                      return bSnesUnits ? "关(34)"      : "关";
	}
}

static const char *_VideoHackSpriteLimiterModeStatus()
{
	return SNPPURenderGetObjLimitMode() == SNPPU_OBJ_LIMIT_MODE_SCREEN
		? "按画面" : "按扫描线";
}

static const char *_VideoHackCpuOverclockStatus()
{
	switch (SNCPUGetOverclockLevel())
	{
		case SNCPU_OVERCLOCK_120: return "120%";
		case SNCPU_OVERCLOCK_150: return "150%";
		case SNCPU_OVERCLOCK_200: return "200%";
		case SNCPU_OVERCLOCK_300: return "300%";
		default:                  return "关";
	}
}

/* AURORA_GBA_UI_V1: GBA/GB settings-menu helpers */
static const char *_VideoScaleName(int mode, int gba)
{
	/* AURORA_GBA_SCALE_V4: GBA 与 GB 均仅保留 0..4 (原始/等比/拉伸/4:3/16:9)。 */
	static const char *kGbNames[5] =
	{
		"原始大小", "等比放大", "全屏拉伸", "4:3", "16:9"
	};
	static const char *kGbaNames[5] =
	{
		"原始大小", "等比放大", "全屏拉伸", "4:3", "16:9"
	};
	if (gba)
	{
		if (mode < 0 || mode > 4) mode = 0;
		return kGbaNames[mode];
	}
	if (mode < 0 || mode > 4) mode = 0;
	return kGbNames[mode];
}
static const char *_VideoGbPaletteName(int mode)
{
	static const char *kNames[7] =
		{"自动", "DMG绿", "Pocket灰", "Light黄", "GBC蓝", "SGB 1A", "Special 1"};
	if (mode < 0 || mode > 6) mode = 0;
	return kNames[mode];
}
static const char *_VideoGbMixName(int mode)
{
	static const char *kNames[4] = {"关", "简单混合", "LCD残影", "快速残影"};
	if (mode < 0 || mode > 3) mode = 0;
	return kNames[mode];
}
static const char *_VideoGbCcName(int mode)
{
	static const char *kNames[3] = {"关", "仅GBC", "始终"};
	if (mode < 0 || mode > 2) mode = 1;
	return kNames[mode];
}

static const char *_VideoCompatProfileStatus()
{
	if (g_VideoCompatFlags == 0) return "标准";
	if (g_VideoCompatFlags == VIDEO_COMPAT_ALL) return "保守";
	return "自定义";
}
static const char *_VideoCompatGsCacheStatus()
{
	return (g_VideoCompatFlags & VIDEO_COMPAT_GS_FULL_CACHE) ? "完整" : "局部";
}
static const char *_VideoCompatGifWaitStatus()
{
	return (g_VideoCompatFlags & VIDEO_COMPAT_GIF_LONG_WAIT) ? "加长" : "常规";
}
static const char *_VideoCompatAudioRpcStatus()
{
	return (g_VideoCompatFlags & VIDEO_COMPAT_AUDIO_SMALL_RPC) ? "1 KB" : "4 KB";
}
static const char *_VideoCompatAudioQueueStatus()
{
	return (g_VideoCompatFlags & VIDEO_COMPAT_AUDIO_DEEP_Q) ? "加深" : "常规";
}


/* AURORA_AUDIO_SPLIT_REVIEW_V1_20260821 */
static Int32 _VideoCycleSystemAudioRate(Int32 hz, int dir)
{
    static const Int32 rates[] =
        { 16000, 22050, 24000, 32000, 38000, 44100, 48000 };
    const Int32 count = (Int32)(sizeof(rates) / sizeof(rates[0]));
    Int32 i, idx = 0;

    for (i = 0; i < count; ++i)
    {
        if (rates[i] == hz)
        {
            idx = i;
            break;
        }
    }

    idx += (dir < 0) ? -1 : 1;
    if (idx < 0) idx = count - 1;
    if (idx >= count) idx = 0;
    return rates[idx];
}

static const char *_VideoMdRenderingStatus()
{
	switch (PicoDriveBridge_GetRenderingMode())
	{
		case 0: return "快速";
		case 1: return "良好";
		default: return "精确";
	}
}

static const char *_VideoMx4sioStatus()
{
	if (!Mx4sioIsEnabled())       return "关";
	if (Mx4sioNeedsRestart())     return "需重启";
	if (Mx4sioGetLastError() < 0) return "驱动错误";
	return Mx4sioIsLoaded() ? "开" : "已启用";
}

typedef struct
{
	Int32 mode;
	const char *name;
} VideoModeChoiceT;

/* AURORA_GBA_VIDEO_V2: 240p re-enabled for CRT users; 480p/720p/1080i
 * remain. IDs match GSK_VIDMODE_* (0=240p, 1=480i, 2=480p, 3=1080i, 4=720p). */
static const VideoModeChoiceT _VideoModes[] =
{
	{ GSK_VIDMODE_240P,  "240p (CRT)" },
	{ GSK_VIDMODE_480I,  "480i（默认）" },
	{ GSK_VIDMODE_480P,  "480p 逐行" },
	{ GSK_VIDMODE_720P,  "720p 高清" },
	{ GSK_VIDMODE_1080I, "1080i" }
};

static Int32 _VideoModeIndex(Int32 mode)
{
	Int32 i;
	for (i = 0; i < (Int32)(sizeof(_VideoModes) / sizeof(_VideoModes[0])); i++)
		if (_VideoModes[i].mode == mode)
			return i;
	return 0;
}

void CVideoScreen::Draw()
{
	Int32 vy = 15;
	char  buf[16];
	int   m = _VideoModeIndex(g_GskVideoMode);
	const char *pMode = _VideoModes[m].name;
	/* AURORA_V85_SOFTWARE_HACKS_PAGE
	 * AURORA_MD_MENU_MAPPING_SRAM_FIX_V4: controller page dedicada.
	 * Display order: 0..9, 30..36, 40..44, 20..29, 10..19.
	 * AURORA_PAD_FACE_REMAP_V1: rows 20..23 = 手柄按键 page. */
	int   iPage = (m_iSelect >= 60) ? 1 :
	              ((m_iSelect >= 50) ? 2 :
	              ((m_iSelect >= 31) ? 3 :
	              ((m_iSelect >= 20) ? 5 :
	              ((m_iSelect >= 10) ? 4 : 0))));
	const char *pWide = "关";
	/* AURORA_SNES9X2010_V2_PS2LEAN_20260824: SNES colour profile is intentionally fixed to
	 * the original/default palette on PS2; no menu row is exposed. */

	if (g_GskWidescreen)
		pWide = "开";

	FontSelect(0);

	_VideoHeader(vy,
		iPage == 0 ? "设置菜单 (1/6)" :
		iPage == 1 ? "设置菜单 (2/6)" :
		iPage == 2 ? "设置菜单 (3/6)" :
		iPage == 3 ? "设置菜单 (4/6)" :
		iPage == 5 ? "设置菜单 (5/6)" :
		             "设置菜单 (6/6)");
	vy += 18;

	if (iPage == 0) {
	_VideoHeader(vy, "画面");
	vy += 14;

	_VideoRow(vy, 0, m_iSelect, "视频模式", pMode);  vy += 12;

	_VideoRow(vy, 1, m_iSelect, "宽屏", pWide); vy += 12;

	snprintf(buf, sizeof(buf), "%d", g_GskOverscan);
	_VideoRow(vy, 2, m_iSelect, "过扫描", buf);      vy += 12;

	snprintf(buf, sizeof(buf), "%d", g_GskDispOffX);
	_VideoRow(vy, 3, m_iSelect, "水平偏移", buf);      vy += 12;

	snprintf(buf, sizeof(buf), "%d", g_GskDispOffY);
	_VideoRow(vy, 4, m_iSelect, "垂直偏移", buf);      vy += 12;

	_VideoRow(vy, 5, m_iSelect, "封面显示", CoverIsEnabled() ? "开" : "关"); vy += 12;
	_VideoRow(vy, 6, m_iSelect, "GBA画面",
	          _VideoScaleName(GbaBridge_GetScalingMode(), 1)); vy += 12;
	_VideoRow(vy, 7, m_iSelect, "GB画面",
	          _VideoScaleName(GbBridge_GetScalingMode(), 0)); vy += 12;
	_VideoRow(vy, 8, m_iSelect, "安全跳帧",
	          _VideoSafeFrameskipStatus()); vy += 12;
	_VideoRow(vy, 9, m_iSelect, "连发速度",
	          MainLoopTurboGetSpeedName()); vy += 12;

	}
	else if (iPage == 2)
	{
		_VideoHeader(vy, "音频"); vy += 14;
		_VideoRow(vy, 50, m_iSelect, "菜单音乐",
		          BgmIsEnabled() ? "开" : "关"); vy += 12;
		snprintf(buf, sizeof(buf), "%d", BgmGetVolume() / 2);
		_VideoRow(vy, 51, m_iSelect, "菜单音量", buf); vy += 12;
		snprintf(buf, sizeof(buf), "%d", AudMixGameGetVolume() / 2);
		_VideoRow(vy, 52, m_iSelect, "游戏音量", buf); vy += 12;
	}
	else if (iPage == 4)
	{
		_VideoHeader(vy, "存储 / 设备"); vy += 14;

		_VideoRow(vy, 10, m_iSelect, "USB存储",
		          MassStorageIsEnabled() ? "开" : "关"); vy += 12;
		_VideoRow(vy, 11, m_iSelect, "内置硬盘",
		          _VideoHddModeStatus()); vy += 12;
		_VideoRow(vy, 12, m_iSelect, "MMCE卡",
		          _VideoMmceStatus()); vy += 12;
		_VideoRow(vy, 13, m_iSelect, "SMB网络",
		          SmbGetStatusText()); vy += 12;
		_VideoRow(vy, 14, m_iSelect, "MX4SIO SD卡",
		          _VideoMx4sioStatus()); vy += 12;

_VideoHeader(vy, "杂项"); vy += 14;

/* AURORA_SWC_FLOPPY_V5_20260831: SRAM Size hidden; Auto forced. */

_VideoRow(vy, 18, m_iSelect, "重置模拟器", ""); vy += 12;
_VideoRow(vy, 19, m_iSelect, "退出到OSD", ""); vy += 12;

	}
	else if (iPage == 1)
	{
		/* AURORA_GBA_UI_V1: Gambatte core options */
		_VideoHeader(vy, "GB / GBC 设置"); vy += 14;
		_VideoRow(vy, 60, m_iSelect, "GB调色板",
		          _VideoGbPaletteName(GbBridge_GetPaletteMode())); vy += 12;
		_VideoRow(vy, 61, m_iSelect, "LCD残影",
		          _VideoGbMixName(GbBridge_GetMixFrames())); vy += 12;
		_VideoRow(vy, 62, m_iSelect, "GBC颜色校正",
		          _VideoGbCcName(GbBridge_GetColorCorrection())); vy += 12;
		vy += 6;
		FontColor4f(0.6f, 0.6f, 0.6f, 1.0f);
		_VideoCenter(128, vy, "以上三项重启游戏后生效"); vy += 12;
	}
	else if (iPage == 5)
	{
		/* AURORA_PAD_FACE_REMAP_V1: 手柄按键 page. Each face button
		   cycles its target through A/B/X/Y (SNES carrier names; GBA
		   uses A/B). Applies live to gameplay; menus keep the standard
		   Cross=confirm layout. Persisted with X like the rest. */
		_VideoHeader(vy, "手柄按键"); vy += 14;
		_VideoRow(vy, 20, m_iSelect, "叉键",
		          MainLoopPadFaceGetTargetName(0)); vy += 12;
		_VideoRow(vy, 21, m_iSelect, "圆圈键",
		          MainLoopPadFaceGetTargetName(1)); vy += 12;
		_VideoRow(vy, 22, m_iSelect, "三角键",
		          MainLoopPadFaceGetTargetName(2)); vy += 12;
		_VideoRow(vy, 23, m_iSelect, "方块键",
		          MainLoopPadFaceGetTargetName(3)); vy += 12;
		vy += 6;
		FontColor4f(0.6f, 0.6f, 0.6f, 1.0f);
		_VideoCenter(128, vy, "默认: 圆=A 叉=B 三角=X 方块=Y");
		vy += 12;
		_VideoCenter(128, vy, "仅游戏生效 菜单按键不变");
	}
	else if (iPage == 3)
	{
		_VideoHeader(vy, "性能"); vy += 14;
		_VideoRow(vy, 31, m_iSelect, "配置档",
			_VideoCompatProfileStatus()); vy += 12;
		_VideoRow(vy, 32, m_iSelect, "GS缓存同步",
			_VideoCompatGsCacheStatus()); vy += 12;
		_VideoRow(vy, 33, m_iSelect, "GIF DMA等待",
			_VideoCompatGifWaitStatus()); vy += 12;
		_VideoRow(vy, 34, m_iSelect, "音频RPC块",
			_VideoCompatAudioRpcStatus()); vy += 12;
		_VideoRow(vy, 35, m_iSelect, "音频队列",
			_VideoCompatAudioQueueStatus()); vy += 12;
	}

	/* controls / hints (clear of the vy=215 footer) */
	vy = 184;
	FontColor4f(0.6f, 0.6f, 0.6f, 1.0f);
	_VideoCenter(128, vy, "上下:选择  左右:调整  X:保存"); vy += 12;
	_VideoCenter(128, vy, "O:下一页  □:上一页"); vy += 12;

	if (g_GskVideoMode != GSK_GetActiveVideoMode())
	{
		FontColor4f(1.0f, 0.88f, 0.46f, 1.0f);
		_VideoCenter(128, vy, "视频模式重启后生效");
	}
	else if (MmceNeedsRestart() || Mx4sioNeedsRestart())
	{
		FontColor4f(1.0f, 0.88f, 0.46f, 1.0f);
		_VideoCenter(128, vy, "存储设置重启后生效");
	}
}

void CVideoScreen::Input(Uint32 buttons, Uint32 trigger)
{
	int dir = 0;

	/* AURORA_PD_MEGA_FIX_20260820: Screen -> GB -> Audio -> Performance -> Pad -> Devices. */
	/* AURORA_GBA_UI_V1: Circle cycles pages forward (6-page order).
	   AURORA_PAD_FACE_REMAP_V1: 手柄按键 (20..23) sits between 性能 and 存储. */
	if (trigger & PAD_CIRCLE)
	{
		if (m_iSelect < 10)        m_iSelect = 60;  /* 画面→GB设置 */
		else if (m_iSelect < 20)   m_iSelect = 0;   /* 存储→画面 */
		else if (m_iSelect < 30)   m_iSelect = 10;  /* 手柄→存储 */
		else if (m_iSelect < 40)   m_iSelect = 20;  /* 性能→手柄 */
		else if (m_iSelect < 60)   m_iSelect = 31;  /* 音频→性能 */
		else                       m_iSelect = 50;  /* GB设置→音频 */
	}

	{
		int lo, hi;
		if (m_iSelect < 10)       { lo = 0;  hi = 9;  }
		else if (m_iSelect < 20)  { lo = 10; hi = 19; }
		else if (m_iSelect < 30)  { lo = 20; hi = 23; }
		else if (m_iSelect < 40)  { lo = 31; hi = 35; }
		else if (m_iSelect < 60)  { lo = 50; hi = 52; }
		else                      { lo = 60; hi = 62; }
		if (trigger & PAD_UP)
		{
			m_iSelect--;
			if (m_iSelect < lo) m_iSelect = hi;
			/* AURORA_GBA_UI_V1: retired 15/16/17 stay unreachable. */
			if (m_iSelect == 15 || m_iSelect == 16 || m_iSelect == 17)
				m_iSelect = 14;
		}
		if (trigger & PAD_DOWN)
		{
			m_iSelect++;
			if (m_iSelect > hi) m_iSelect = lo;
			if (m_iSelect == 15 || m_iSelect == 16 || m_iSelect == 17)
				m_iSelect = 18;
		}
	}

	if (trigger & PAD_LEFT)  dir = -1;
	if (trigger & PAD_RIGHT) dir = +1;

	if (dir != 0)
	{
		switch (m_iSelect)
		{
		case 0: /* video mode (live GS reinit) */
			{
				Int32 count = (Int32)(sizeof(_VideoModes) / sizeof(_VideoModes[0]));
				Int32 modeIndex = _VideoModeIndex(g_GskVideoMode) + dir;
				if (modeIndex < 0)      modeIndex = count - 1;
				if (modeIndex >= count) modeIndex = 0;
				MainLoopReinitVideoMode(_VideoModes[modeIndex].mode);
			}
			break;

		case 1: /* widescreen on/off (live) */
			g_GskWidescreen = !g_GskWidescreen;
			GSK_SetWidescreen(g_GskWidescreen);
			break;

		case 2: /* overscan 0..100 (live, step 5) */
			g_GskOverscan += dir * 5;
			if (g_GskOverscan < 0)   g_GskOverscan = 0;
			if (g_GskOverscan > 100) g_GskOverscan = 100;
			GSK_SetOverscan(g_GskOverscan);
			break;

		case 3: /* offset X (live) */
			g_GskDispOffX += dir;
			if (g_GskDispOffX < -64) g_GskDispOffX = -64;
			if (g_GskDispOffX >  64) g_GskDispOffX =  64;
			GSK_SetDisplayOffset(g_GskDispOffX, g_GskDispOffY);
			break;

		case 4: /* offset Y (live) */
			g_GskDispOffY += dir;
			if (g_GskDispOffY < -64) g_GskDispOffY = -64;
			if (g_GskDispOffY >  64) g_GskDispOffY =  64;
			GSK_SetDisplayOffset(g_GskDispOffX, g_GskDispOffY);
			break;

		case 5: /* cover art on/off (live; persisted on X like the rest) */
			CoverToggle();
			break;

		case 6: /* AURORA_GBA_SCALE_V4: GBA scaling (live, 0..4) */
			{
				int m = GbaBridge_GetScalingMode() + dir;
				if (m < 0) m = 4;
				if (m > 4) m = 0;
				GbaBridge_SetScalingMode(m);
			}
			break;
		case 7: /* GB scaling (live, 0..4) */
			{
				int m = GbBridge_GetScalingMode() + dir;
				if (m < 0) m = 4;
				if (m > 4) m = 0;
				GbBridge_SetScalingMode(m);
			}
			break;
		case 8: /* Safe Frameskip: Off, On */
		{
			Int32 level = MainLoopSafeFrameskipGetLevel();
			level += (dir > 0) ? 1 : -1;
			if (level > 9) level = 0;
			if (level < 0) level = 9;
			MainLoopSafeFrameskipSetLevel(level);
			break;
		}
		case 9: /* AURORA_GBA_UI_V1: turbo speed (moved from row 43) */
			MainLoopTurboCycleSpeedDir(dir);
			break;

		case 50: /* Menu Music ON/OFF. */
			BgmSetEnabled(!BgmIsEnabled());
			break;
		case 51: /* Menu volume: UI 0..200, internal 0..400. */
			{ int v = BgmGetVolume() + dir * 2; if (v < 0) v = 0; if (v > 400) v = 400; BgmSetVolume(v); }
			break;
		case 52: /* SNES volume: SNESticle + QuickNES, UI 0..200. */
			{ int v = AudMixGameGetVolume() + dir * 2; if (v < 0) v = 0; if (v > 400) v = 400; AudMixGameSetVolume(v); }
			break;

		case 10: /* Mass / USB on/off -- lista mass0:/mass1: (USB).  O USB core
		           sobe no boot de qualquer forma (seguro); isto controla a
		           listagem.  O MX4SIO agora tem toggle proprio (case 14). */
			MassStorageSetEnabled(!MassStorageIsEnabled());
			break;

		case 11: /* HDD interno: ciclo 关 -> APA(hdd0:) -> exFAT/FAT(ata0:)
		           -> 关.  Os dois modos sao MUTUAMENTE EXCLUSIVOS (ambos
		           registram a library atad); AtaBd/Hdd SetEnabled ja' cuida
		           de desligar o oposto.  Carga preguicosa no primeiro uso. */
			if (AtaBdSupportIsEnabled())
			{
				HddSupportSetEnabled(0);
				AtaBdSupportSetEnabled(0);
			}
			else if (HddSupportIsEnabled())
			{
				HddSupportSetEnabled(0);
				AtaBdSupportSetEnabled(1);
			}
			else
			{
				HddSupportSetEnabled(1);
			}
			break;

		case 12: /* MMCE (mmce0/1) on/off -- lista + carga preguicosa. */
			MmceSupportSetEnabled(!MmceSupportIsEnabled());
			if (MmceSupportIsEnabled())
			{
				BgmIOBegin();
				MmceProbeAvailableSlots();
				BgmIOEnd();
			}
			break;

		case 13: /* SMB on/off. Driver/network stay lazy until smb: is opened. */
			if (SmbSupportIsEnabled())
			{
				BgmIOBegin();
				SmbDisconnect();
				BgmIOEnd();
				SmbSupportSetEnabled(0);
			}
			else
			{
				SmbSupportSetEnabled(1);
			}
			break;

		case 14: /* MX4SIO (SD via SIO2) on/off -- carga preguicosa (deferida).
		            Padrao OFF: quem nao tem o adaptador evita o flood de
		            sondagem do SIO2.  Independente do Mass/USB. */
			Mx4sioSetEnabled(!Mx4sioIsEnabled());
			if (Mx4sioIsEnabled())
			{
				BgmIOBegin();
				Mx4sioLoadIfEnabled();
				BgmIOEnd();
			}
			break;

case 15: /* SRAM Size -- retired, kept for index compatibility */
    g_FakeSRAMSize = 0; /* AURORA_SWC_FLOPPY_V5_20260831 */
    break;

		/* AURORA_PAD_FACE_REMAP_V1: face-button target cycling. Host
		   order 0..3 = Cross / Circle / Triangle / Square. */
		case 20:
			MainLoopPadFaceCycleDir(0, dir);
			break;
		case 21:
			MainLoopPadFaceCycleDir(1, dir);
			break;
		case 22:
			MainLoopPadFaceCycleDir(2, dir);
			break;
		case 23:
			MainLoopPadFaceCycleDir(3, dir);
			break;


		case 31:
			_VideoApplyCompatFlags(
				g_VideoCompatFlags == VIDEO_COMPAT_ALL ? 0 : VIDEO_COMPAT_ALL);
			break;
		case 32:
			_VideoApplyCompatFlags(
				g_VideoCompatFlags ^ VIDEO_COMPAT_GS_FULL_CACHE);
			break;
		case 33:
			_VideoApplyCompatFlags(
				g_VideoCompatFlags ^ VIDEO_COMPAT_GIF_LONG_WAIT);
			break;
		case 34:
			_VideoApplyCompatFlags(
				g_VideoCompatFlags ^ VIDEO_COMPAT_AUDIO_SMALL_RPC);
			break;
		case 35:
			_VideoApplyCompatFlags(
				g_VideoCompatFlags ^ VIDEO_COMPAT_AUDIO_DEEP_Q);
			break;
		case 60: /* GB palette 0..6 -- applies on next game start */
			{
				int m = GbBridge_GetPaletteMode() + dir;
				if (m < 0) m = 6;
				if (m > 6) m = 0;
				GbBridge_SetPaletteMode(m);
			}
			break;
		case 61: /* LCD ghosting 0..3 */
			{
				int m = GbBridge_GetMixFrames() + dir;
				if (m < 0) m = 3;
				if (m > 3) m = 0;
				GbBridge_SetMixFrames(m);
			}
			break;
		case 62: /* GBC colour correction 0..2 */
			{
				int m = GbBridge_GetColorCorrection() + dir;
				if (m < 0) m = 2;
				if (m > 2) m = 0;
				GbBridge_SetColorCorrection(m);
			}
			break;
		}


	}

	/* Square: previous page in the displayed six-page order. */
	/* AURORA_GBA_UI_V1: Square cycles pages backwards.
	   AURORA_PAD_FACE_REMAP_V1: 手柄按键 sits between 性能 and 存储. */
	if (trigger & PAD_SQUARE)
	{
		if (m_iSelect >= 60)      m_iSelect = 0;    /* GB设置→画面 */
		else if (m_iSelect >= 50) m_iSelect = 60;   /* 音频→GB设置 */
		else if (m_iSelect >= 31) m_iSelect = 50;   /* 性能→音频 */
		else if (m_iSelect >= 20) m_iSelect = 31;   /* 手柄→性能 */
		else if (m_iSelect >= 10) m_iSelect = 20;   /* 存储→手柄 */
		else                      m_iSelect = 10;   /* 画面→存储 */
	}

/* Cross / Start: persist ordinary settings; immediate actions never save. */
if (trigger & (PAD_CROSS | PAD_START))
{
    if (m_iSelect == 18)
    {
        if (Aud_IsInitialized()) Aud_Setvol(0);
        MainResetEmulator();
    }
    else if (m_iSelect == 19)
    {
        if (Aud_IsInitialized()) Aud_Setvol(0);
        ExecOSD(0, NULL);
    }
    else
        VideoSettingsSave();
}
}

