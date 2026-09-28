#ifndef _GAMBATTE_BRIDGE_H
#define _GAMBATTE_BRIDGE_H
/* AURORA_GAMBATTE_V1: Gambatte libretro bridge for PS2/SNESticle Aurora.
 *
 * Gambatte loads Game Boy / Game Boy Color cartridges from memory
 * (info->data), outputs RGB565 and stereo interleaved batches near
 * 32 kHz (blipper) or 64 kHz (cc resampler, Aurora uses blipper
 * defaults). SRAM and RTC are exposed through the standard
 * retro_get_memory interface.
 */
#include <stddef.h>
#include "types.h"
namespace Emu { struct SysInputT; }
class CRenderSurface;
class CMixBuffer;
bool GbBridge_Init(void);
void GbBridge_Shutdown(void);
bool GbBridge_LoadGame(const void *pData, size_t nBytes, size_t nCapacity, const char *pName);
void GbBridge_UnloadGame(void);
void GbBridge_Reset(void);
void GbBridge_RunFrame(Emu::SysInputT *pInput, CRenderSurface *pTarget, CMixBuffer *pMixBuf);
int GbBridge_GetStateSize(void);
int GbBridge_SaveState(void *pData, int nBytes);
bool GbBridge_LoadState(const void *pData, int nBytes);
int GbBridge_GetSRAMBytes(void);
Uint8 *GbBridge_GetSRAMData(void);
unsigned GbBridge_GetSampleRate(void);
bool GbBridge_IsGameLoaded(void);
/* AURORA_GBA_UI_V1: 前端设置菜单接口 */
Int32 GbBridge_GetScalingMode(void);     /* 0=原始 1=等比 2=拉伸 3=4:3 4=16:9, 即时生效 */
void GbBridge_SetScalingMode(Int32 mode);
Int32 GbBridge_GetPaletteMode(void);     /* 0..6, 重启游戏后生效 */
void GbBridge_SetPaletteMode(Int32 mode);
Int32 GbBridge_GetMixFrames(void);       /* 0..3 LCD残影, 重启游戏后生效 */
void GbBridge_SetMixFrames(Int32 mode);
Int32 GbBridge_GetColorCorrection(void); /* 0..2 GBC颜色校正, 重启游戏后生效 */
void GbBridge_SetColorCorrection(Int32 mode);
#endif
