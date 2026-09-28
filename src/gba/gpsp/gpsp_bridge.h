#ifndef _GPSP_BRIDGE_H
#define _GPSP_BRIDGE_H
/* AURORA_GPSP_V1: gpSP libretro bridge for PS2/GBAticle Aurora.
 *
 * gpSP loads the cartridge itself through filestream_open(info->path),
 * so the frontend hands over a real filesystem path (plain file, or a
 * temp .gba extracted from ZIP/GZ under GBAticle/SYSTEM) and never
 * keeps a second full ROM copy in EE RAM.
 *
 * Video: the PS2 build defines USE_XBGR1555_FORMAT, so gba_screen_pixels
 * holds GBA-native BGR555 that the bridge expands into Aurora's RGBA32
 * render surface. Audio: stereo interleaved batches at the core's rate
 * (65536 Hz default) fed into AudMixBuffer's linear resampler.
 */
#include <stddef.h>
#include "types.h"
namespace Emu { struct SysInputT; }
class CRenderSurface;
class CMixBuffer;
bool GbaBridge_Init(void);
void GbaBridge_Shutdown(void);
/* Path-based cartridge load; gpSP owns the ROM window (16 MiB LRU). */
bool GbaBridge_LoadCart(const char *pPath, const char *pSystemPath);
/* AURORA_GBA_ZIP_MEM_V1_20260911: ZIP/GZ carts that fit gpSP's ROM cache
 * window are decompressed straight into the cache blocks - no temp file.
 * Query the window first, fill it via the sink (MinizExtract*ToSink),
 * then load with GbaBridge_LoadCartMemory(). The sink returns 0 when a
 * write would leave the window, aborting the extraction so the caller
 * can fall back to the file-based path. */
int GbaBridge_RomCacheWindowBytes(void);
size_t GbaBridge_RomCacheSink(void *pOpaque, unsigned long long uFileOfs,
                              const void *pBuf, size_t nBytes);
bool GbaBridge_LoadCartMemory(const char *pPath, const char *pSystemPath,
                              unsigned uRomBytes);
void GbaBridge_UnloadGame(void);
void GbaBridge_Reset(void);
void GbaBridge_RunFrame(Emu::SysInputT *pInput, CRenderSurface *pTarget, CMixBuffer *pMixBuf);
int GbaBridge_GetStateSize(void);
int GbaBridge_SaveState(void *pData, int nBytes);
bool GbaBridge_LoadState(const void *pData, int nBytes);
int GbaBridge_GetSRAMBytes(void);
Uint8 *GbaBridge_GetSRAMData(void);
unsigned GbaBridge_GetSampleRate(void);
bool GbaBridge_IsCartLoaded(void);
/* AURORA_GPSP_STATE_V1: cartridge identity (CRC32 over the on-disk cart
 * file + exact byte count) for frontend savestate matching. False when no
 * cart is loaded or the core could not fingerprint the file. */
bool GbaBridge_GetCartIdentity(Uint32 *pCRC, Uint32 *pBytes);
/* Frontend SRAM persistence flushes the core's 128 KiB gamepak_backup. */
void GbaBridge_SyncSramRegion(void);
/* AURORA_GBA_UI_V1: 画面缩放 0=原始1:1居中 1=等比放大 2=全屏拉伸 (即时生效) */
Int32 GbaBridge_GetScalingMode(void);
void GbaBridge_SetScalingMode(Int32 mode);
#endif
