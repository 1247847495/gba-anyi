/* retired_core_stubs.cpp
 *
 * AURORA_GBA_ONLY_V1: 本编译为 GBA/GB 专用版。
 *
 * NES(FDS)/MD/SMS/PCE/Snes9x2010 五个核心已从构建与链接中完全移除
 * (Makefile SRCS 不再编译它们的实现, 链接行不再挂它们的核心库)。
 * 平台层(mainloop 系列与 uiVideo)仍保留对这些系统与桥接接口的引用,
 * 本文件为全部被引用符号提供空实现以满足链接。
 *
 * 所有桩均不可达: 浏览器只注册 .gba/.gb/.gbc/.sgb/.zip/.gz,
 * 上述系统永远不会被选中执行。
 */

#include "types.h"
#include <string.h>

/* AURORA_GBA_ONLY_V2: SNES 原生核心已移除, 以下头文件仅提供类型。
 * 桩实现见文件末尾 SNES RETIRED 段。 */
#include "snes.h"          /* SnesSystem / SnesAudioSetRate */
#include "snrom.h"         /* SnesRom / g_FakeSRAMSize / g_SnesForceRegion */
#include "snio.h"          /* SnesTurboFile* / SnesIO */
#include "snswc.h"         /* SNSuperWildCard */
#include "snstate.h"       /* SnesStateT / SNStateCompare */
#include "sncpu.h"         /* SNCPUSetExecuteFunc 等 */
#include "snspc.h"         /* SNSPCSetExecuteFunc */
extern "C" {
#include "snspc_c.h"       /* SNSPCExecute_C: C linkage, 同 mainloop_exec.cpp */
}
#include "snppucolor.h"    /* SNPPUColor* */
#include "snppurender.h"   /* SNPPURender* / g_Snes* */

#include "nes/system/nessystem.h"
#include "nes/fceumm/fdssystem.h"
#include "sega/system/segasystem.h"
#include "pce/system/pcesystem.h"
#include "snes/snes9x2010/snes9x2010system.h"

#include "nes/quicknes/quicknes_bridge.h"
#include "nes/fceumm/fceumm_fds_bridge.h"
#include "sega/picodrive/picodrive_bridge.h"
#include "pce/beetle/pce_bridge.h"
#include "snes/snes9x2010/snes9x2010_bridge.h"

/* ------------------------------------------------------------------ */
/* System wrappers                                                     */
/* ------------------------------------------------------------------ */

/* ---- NesSystem (QuickNES) ---- */
NesSystem::NesSystem() {}
NesSystem::~NesSystem() {}
void NesSystem::SetRom(Emu::Rom *) {}
void NesSystem::Reset() {}
void NesSystem::SoftReset() {}
void NesSystem::ExecuteFrame(Emu::SysInputT *, class CRenderSurface *, class CMixBuffer *, ModeE) {}
Int32 NesSystem::GetStateSize() { return 0; }
void NesSystem::SaveState(void *, Int32) {}
void NesSystem::RestoreState(void *, Int32) {}
Int32 NesSystem::GetSRAMBytes() { return 0; }
Uint8 *NesSystem::GetSRAMData() { return NULL; }
const char *NesSystem::GetString(StringE) { return ""; }
Uint32 NesSystem::GetSampleRate() { return 0; }
void NesSystem::SaveState(NesStateT *) {}
Bool NesSystem::RestoreState(NesStateT *) { return FALSE; }

/* ---- FdsSystem (FCEUmm) ---- */
FdsSystem::FdsSystem() {}
FdsSystem::~FdsSystem() {}
void FdsSystem::SetRom(Emu::Rom *) {}
Bool FdsSystem::LoadDisk(const Char *, const Char *) { return FALSE; }
Bool FdsSystem::LoadDiskMemory(const void *, Uint32, const Char *, const Char *) { return FALSE; }
void FdsSystem::Reset() {}
void FdsSystem::SoftReset() {}
void FdsSystem::ExecuteFrame(Emu::SysInputT *, class CRenderSurface *, class CMixBuffer *, ModeE) {}
Int32 FdsSystem::GetStateSize() { return 0; }
void FdsSystem::SaveState(void *, Int32) {}
void FdsSystem::RestoreState(void *, Int32) {}
Int32 FdsSystem::GetSRAMBytes() { return 0; }
Uint8 *FdsSystem::GetSRAMData() { return NULL; }
const char *FdsSystem::GetString(StringE) { return ""; }
Uint32 FdsSystem::GetSampleRate() { return 0; }
Bool FdsSystem::SaveStateChecked(void *, Int32) { return FALSE; }
Bool FdsSystem::RestoreStateChecked(const void *, Int32) { return FALSE; }

/* ---- SegaSystem (PicoDrive) ---- */
SegaSystem::SegaSystem() {}
SegaSystem::~SegaSystem() {}
void SegaSystem::SetRom(Emu::Rom *) {}
Bool SegaSystem::LoadDisc(const Char *, const Char *) { return FALSE; }
void SegaSystem::Reset() {}
void SegaSystem::SoftReset() {}
void SegaSystem::ExecuteFrame(Emu::SysInputT *, class CRenderSurface *, class CMixBuffer *, ModeE) {}
Int32 SegaSystem::GetStateSize() { return 0; }
void SegaSystem::SaveState(void *, Int32) {}
void SegaSystem::RestoreState(void *, Int32) {}
Int32 SegaSystem::GetSRAMBytes() { return 0; }
Uint8 *SegaSystem::GetSRAMData() { return NULL; }
const char *SegaSystem::GetString(StringE) { return ""; }
Uint32 SegaSystem::GetSampleRate() { return 0; }
Bool SegaSystem::SaveStateChecked(void *, Int32) { return FALSE; }
Bool SegaSystem::RestoreStateChecked(const void *, Int32) { return FALSE; }

/* ---- PceSystem (Beetle PCE Fast) ---- */
PceSystem::PceSystem() {}
PceSystem::~PceSystem() {}
void PceSystem::SetRom(Emu::Rom *) {}
Bool PceSystem::LoadDisc(const Char *, const Char *) { return FALSE; }
void PceSystem::Reset() {}
void PceSystem::SoftReset() {}
void PceSystem::ExecuteFrame(Emu::SysInputT *, class CRenderSurface *, class CMixBuffer *, ModeE) {}
Int32 PceSystem::GetStateSize() { return 0; }
void PceSystem::SaveState(void *, Int32) {}
void PceSystem::RestoreState(void *, Int32) {}
Int32 PceSystem::GetSRAMBytes() { return 0; }
Uint8 *PceSystem::GetSRAMData() { return NULL; }
const char *PceSystem::GetString(StringE) { return ""; }
Uint32 PceSystem::GetSampleRate() { return 0; }
Bool PceSystem::SaveStateChecked(void *, Int32) { return FALSE; }
Bool PceSystem::RestoreStateChecked(const void *, Int32) { return FALSE; }

/* ---- Snes9x2010System ---- */
Snes9x2010System::Snes9x2010System() {}
Snes9x2010System::~Snes9x2010System() {}
void Snes9x2010System::SetRom(Emu::Rom *) {}
void Snes9x2010System::Reset() {}
void Snes9x2010System::SoftReset() {}
void Snes9x2010System::ExecuteFrame(Emu::SysInputT *, class CRenderSurface *, class CMixBuffer *, ModeE) {}
Int32 Snes9x2010System::GetStateSize() { return 0; }
void Snes9x2010System::SaveState(void *, Int32) {}
void Snes9x2010System::RestoreState(void *, Int32) {}
Int32 Snes9x2010System::GetSRAMBytes() { return 0; }
Uint8 *Snes9x2010System::GetSRAMData() { return NULL; }
const char *Snes9x2010System::GetString(StringE) { return ""; }
Uint32 Snes9x2010System::GetSampleRate() { return 0; }
Bool Snes9x2010System::SaveStateChecked(void *, Int32) { return FALSE; }
Bool Snes9x2010System::RestoreStateChecked(const void *, Int32) { return FALSE; }

/* ------------------------------------------------------------------ */
/* Rom wrappers                                                        */
/* ------------------------------------------------------------------ */

NesRom::NesRom() {}
NesRom::~NesRom() {}
Emu::Rom::LoadErrorE NesRom::LoadRom(CDataIO *, Uint8 *, Uint32) { return LOADERROR_INVALID; }
void NesRom::Unload() {}
Char *NesRom::GetMapperName() { return (Char *)""; }

NesDisk::NesDisk() {}
NesDisk::~NesDisk() {}
Emu::Rom::LoadErrorE NesDisk::LoadRom(CDataIO *, Uint8 *, Uint32) { return LOADERROR_INVALID; }
void NesDisk::Unload() {}

NesFDSBios::NesFDSBios() {}
NesFDSBios::~NesFDSBios() {}
Emu::Rom::LoadErrorE NesFDSBios::LoadRom(CDataIO *, Uint8 *, Uint32) { return LOADERROR_INVALID; }
void NesFDSBios::Unload() {}

SegaRom::SegaRom() {}
SegaRom::~SegaRom() {}
Emu::Rom::LoadErrorE SegaRom::LoadRom(CDataIO *, Uint8 *, Uint32) { return LOADERROR_INVALID; }
void SegaRom::Unload() {}
Uint32 SegaRom::GetNumExts() { return 0; }
Char *SegaRom::GetExtName(Uint32) { return (Char *)""; }
Uint32 SegaRom::GetNumRomRegions() { return 0; }
Char *SegaRom::GetRomRegionName(Uint32) { return (Char *)""; }
Uint32 SegaRom::GetRomRegionSize(Uint32) { return 0; }
Char *SegaRom::GetMapperName() { return (Char *)""; }
Char *SegaRom::GetRomTitle() { return (Char *)""; }
Emu::Rom::LoadErrorE SegaRom::AttachBuffer(Uint8 *, Uint32, Uint32) { return LOADERROR_INVALID; }
void SegaRom::SetSourceName(const Char *) {}

PceRom::PceRom() {}
PceRom::~PceRom() {}
Emu::Rom::LoadErrorE PceRom::LoadRom(CDataIO *, Uint8 *, Uint32) { return LOADERROR_INVALID; }
void PceRom::Unload() {}
Uint32 PceRom::GetNumExts() { return 0; }
Char *PceRom::GetExtName(Uint32) { return (Char *)""; }
Uint32 PceRom::GetNumRomRegions() { return 0; }
Char *PceRom::GetRomRegionName(Uint32) { return (Char *)""; }
Uint32 PceRom::GetRomRegionSize(Uint32) { return 0; }
Char *PceRom::GetMapperName() { return (Char *)""; }
Char *PceRom::GetRomTitle() { return (Char *)""; }
Emu::Rom::LoadErrorE PceRom::AttachBuffer(Uint8 *, Uint32, Uint32) { return LOADERROR_INVALID; }
void PceRom::SetSourceName(const Char *) {}

Snes9x2010Rom::Snes9x2010Rom() {}
Snes9x2010Rom::~Snes9x2010Rom() {}
Emu::Rom::LoadErrorE Snes9x2010Rom::LoadRom(CDataIO *, Uint8 *, Uint32) { return LOADERROR_INVALID; }
void Snes9x2010Rom::Unload() {}
Uint32 Snes9x2010Rom::GetNumExts() { return 0; }
Char *Snes9x2010Rom::GetExtName(Uint32) { return (Char *)""; }
Uint32 Snes9x2010Rom::GetNumRomRegions() { return 0; }
Char *Snes9x2010Rom::GetRomRegionName(Uint32) { return (Char *)""; }
Uint32 Snes9x2010Rom::GetRomRegionSize(Uint32) { return 0; }
Char *Snes9x2010Rom::GetMapperName() { return (Char *)""; }
Char *Snes9x2010Rom::GetRomTitle() { return (Char *)""; }
Emu::Rom::LoadErrorE Snes9x2010Rom::AttachBuffer(Uint8 *, Uint32, Uint32) { return LOADERROR_INVALID; }
void Snes9x2010Rom::SetSourceName(const Char *) {}
void Snes9x2010Rom::DetachFrontendBacking() {}

/* ------------------------------------------------------------------ */
/* QuickNES bridge                                                     */
/* ------------------------------------------------------------------ */
bool QuicknesBridge_Init(void) { return false; }
void QuicknesBridge_Shutdown(void) {}
bool QuicknesBridge_LoadGame(const void *, size_t, const char *) { return false; }
void QuicknesBridge_UnloadGame(void) {}
void QuicknesBridge_Reset(void) {}
void QuicknesBridge_SoftReset(void) {}
void QuicknesBridge_SetDutySwap(bool) {}
bool QuicknesBridge_SetPalette(const Uint8 *) { return false; }
void QuicknesBridge_SetTurboSpeed(unsigned) {}
void QuicknesBridge_SetSkipVideo(bool) {}
void QuicknesBridge_InvalidateGsResources(void) {}
bool QuicknesBridge_CanDirectGsVideo(void) { return false; }
bool QuicknesBridge_DrawDirectGs(Uint32, Int32, Float32) { return false; }
void QuicknesBridge_RunFrame(Emu::SysInputT *, CRenderSurface *, CMixBuffer *) {}
int QuicknesBridge_GetStateSize(void) { return 0; }
int QuicknesBridge_SaveState(void *, int) { return 0; }
bool QuicknesBridge_LoadState(const void *, int) { return false; }
int QuicknesBridge_GetSRAMBytes(void) { return 0; }
uint8_t *QuicknesBridge_GetSRAMData(void) { return NULL; }
bool QuicknesBridge_IsArkanoidVaus(void) { return false; }
bool QuicknesBridge_TurboFileEnabled(void) { return false; }
int QuicknesBridge_GetTurboFileBytes(void) { return 0; }
uint8_t *QuicknesBridge_GetTurboFileData(void) { return NULL; }
bool QuicknesBridge_TurboFileDirty(void) { return false; }
void QuicknesBridge_ClearTurboFileDirty(void) {}
void QuicknesBridge_SetLightGunEnabled(bool) {}
bool QuicknesBridge_GetLightGunEnabled(void) { return false; }
bool QuicknesBridge_LightGunActive(void) { return false; }
void QuicknesBridge_GetLightGunCursor(Int32 *x, Int32 *y) { if (x) *x = 0; if (y) *y = 0; }
void QuicknesBridge_DrawLightGunCursor(Int32) {}
bool QuicknesBridge_BattleBoxEnabled(void) { return false; }
int QuicknesBridge_GetBattleBoxBytes(void) { return 0; }
uint8_t *QuicknesBridge_GetBattleBoxData(void) { return NULL; }
bool QuicknesBridge_BattleBoxDirty(void) { return false; }
void QuicknesBridge_ClearBattleBoxDirty(void) {}
unsigned QuicknesBridge_GetSampleRate(void) { return 0; }

/* ------------------------------------------------------------------ */
/* FCEUmm FDS bridge                                                   */
/* ------------------------------------------------------------------ */
bool FceummFdsBridge_Init(void) { return false; }
void FceummFdsBridge_Shutdown(void) {}
bool FceummFdsBridge_LoadDisk(const char *, const char *, unsigned) { return false; }
bool FceummFdsBridge_LoadDiskMemory(const void *, Uint32, const char *, const char *, unsigned) { return false; }
void FceummFdsBridge_UnloadGame(void) {}
void FceummFdsBridge_Reset(void) {}
void FceummFdsBridge_SoftReset(void) {}
void FceummFdsBridge_SetSkipVideo(bool) {}
bool FceummFdsBridge_SetPalette(const Uint8 *) { return false; }
void FceummFdsBridge_InvalidateGsResources(void) {}
bool FceummFdsBridge_CanDirectGsVideo(void) { return false; }
bool FceummFdsBridge_DrawDirectGs(Uint32, Int32, Float32) { return false; }
void FceummFdsBridge_RunFrame(Emu::SysInputT *, CRenderSurface *, CMixBuffer *) {}
int FceummFdsBridge_GetStateSize(void) { return 0; }
int FceummFdsBridge_SaveState(void *, int) { return 0; }
bool FceummFdsBridge_LoadState(const void *, int) { return false; }
bool FceummFdsBridge_BeginSideSwap(void) { return false; }
bool FceummFdsBridge_IsSideSwapPending(void) { return false; }
unsigned FceummFdsBridge_GetSelectedSide(void) { return 0; }
bool FceummFdsBridge_IsDiskInserted(void) { return false; }
bool FceummFdsBridge_ConsumeDriveStateChange(unsigned *selectedSide, bool *inserted)
{ if (selectedSide) *selectedSide = 0; if (inserted) *inserted = false; return false; }
void FceummFdsBridge_GetDriveState(unsigned *selectedSide, bool *inserted,
                                   unsigned *swapFramesRemaining, unsigned *swapTargetSide)
{ if (selectedSide) *selectedSide = 0; if (inserted) *inserted = false;
  if (swapFramesRemaining) *swapFramesRemaining = 0; if (swapTargetSide) *swapTargetSide = 0; }
bool FceummFdsBridge_SetDriveState(unsigned, bool, unsigned, unsigned) { return false; }
unsigned FceummFdsBridge_GetSampleRate(void) { return 0; }

/* ------------------------------------------------------------------ */
/* PicoDrive bridge                                                    */
/* ------------------------------------------------------------------ */
bool PicoDriveBridge_Init(void) { return false; }
void PicoDriveBridge_Shutdown(void) {}
size_t PicoDriveBridge_RequiredRomCapacity(size_t nBytes) { return nBytes; }
bool PicoDriveBridge_LoadGame(const void *, size_t, size_t, const char *) { return false; }
int PicoDriveBridge_ProbeSegaCd(const char *) { return 0; }
bool PicoDriveBridge_LoadDisc(const char *, const char *) { return false; }
void PicoDriveBridge_UnloadGame(void) {}
void PicoDriveBridge_Reset(void) {}
void PicoDriveBridge_SoftReset(void) {}
void PicoDriveBridge_Set6Button(bool) {}
bool PicoDriveBridge_Get6Button(void) { return false; }
void PicoDriveBridge_SetRenderingMode(int) {}
int PicoDriveBridge_GetRenderingMode(void) { return 0; }
void PicoDriveBridge_SetSmsColorBorder(bool) {}
bool PicoDriveBridge_GetSmsColorBorder(void) { return false; }
void PicoDriveBridge_SetGgZoom(bool) {}
bool PicoDriveBridge_GetGgZoom(void) { return false; }
void PicoDriveBridge_SetSmsFm(bool) {}
bool PicoDriveBridge_GetSmsFm(void) { return false; }
void PicoDriveBridge_SetAudioRate(int) {}
int PicoDriveBridge_GetAudioRate(void) { return 0; }
int PicoDriveBridge_GetNominalFrameRate(void) { return 0; }
bool PicoDriveBridge_IsMasterSystem(void) { return false; }
bool PicoDriveBridge_Is8Bit(void) { return false; }
void PicoDriveBridge_QueueMasterSystemPause(void) {}
void PicoDriveBridge_SetRegion(int) {}
void PicoDriveBridge_SetMouseInput(bool, int, int, unsigned) {}
void PicoDriveBridge_SetSkipVideo(bool) {}
void PicoDriveBridge_Set32xAudioSacrifice(bool) {}
bool PicoDriveBridge_Is32X(void) { return false; }
bool PicoDriveBridge_IsSegaCD(void) { return false; }
/* 返回 true: 无 Sega CD 时永远允许游戏切换。
 * 浏览器按 X 加载任何游戏前都会无条件调用本函数 (mainloop_browser.cpp),
 * 返回 false 会被误判为 "SEGA CD读取忙" 并拦截所有加载。 */
bool PicoDriveBridge_PrepareGameSwitch(void) { return true; }
void PicoDriveBridge_SetCdAudioSafeWindow(bool) {}
bool PicoDriveBridge_ConsumeCdAudioRefillRequest(void) { return false; }
bool PicoDriveBridge_PrefetchCdAudio(void) { return false; }
void PicoDriveBridge_SetCdMusicEnabled(bool) {}
bool PicoDriveBridge_GetCdMusicEnabled(void) { return false; }
void PicoDriveBridge_RunFrame(Emu::SysInputT *, CRenderSurface *, CMixBuffer *) {}
int PicoDriveBridge_GetStateSize(void) { return 0; }
int PicoDriveBridge_SaveState(void *, int) { return 0; }
bool PicoDriveBridge_LoadState(const void *, int) { return false; }
int PicoDriveBridge_GetSRAMBytes(void) { return 0; }
Uint8 *PicoDriveBridge_GetSRAMData(void) { return NULL; }
unsigned PicoDriveBridge_GetSampleRate(void) { return 0; }
void PicoDriveBridge_InvalidateGsResources(void) {}
bool PicoDriveBridge_IsMegaDrive(void) { return false; }
bool PicoDriveBridge_IsMegaDriveVideo(void) { return false; }
bool PicoDriveBridge_CanDirectGsVideo(void) { return false; }
bool PicoDriveBridge_DrawDirectGs(Uint32, Float32) { return false; }

/* ------------------------------------------------------------------ */
/* Beetle PCE Fast bridge                                              */
/* ------------------------------------------------------------------ */
bool PceBridge_Init(void) { return false; }
void PceBridge_Shutdown(void) {}
bool PceBridge_LoadGame(const void *, size_t, size_t, const char *) { return false; }
bool PceBridge_LoadDisc(const char *, const char *) { return false; }
bool PceBridge_IsDiscLoaded(void) { return false; }
bool PceBridge_QuiesceDiscIO(void) { return false; }
void PceBridge_ResumeDiscIO(void) {}
void PceBridge_UnloadGame(void) {}
void PceBridge_Reset(void) {}
void PceBridge_SoftReset(void) {}
void PceBridge_SetSkipVideo(bool) {}
void PceBridge_SetCdAudioSafeWindow(bool) {}
bool PceBridge_ConsumeCdAudioRefillRequest(void) { return false; }
void PceBridge_SetCdMusicEnabled(bool) {}
bool PceBridge_GetCdMusicEnabled(void) { return false; }
void PceBridge_RunFrame(Emu::SysInputT *, CRenderSurface *, CMixBuffer *) {}
int PceBridge_GetStateSize(void) { return 0; }
int PceBridge_SaveState(void *, int) { return 0; }
bool PceBridge_LoadState(const void *, int) { return false; }
int PceBridge_GetSRAMBytes(void) { return 0; }
Uint8 *PceBridge_GetSRAMData(void) { return NULL; }
void PceBridge_InvalidateGsResources(void) {}
bool PceBridge_CanDirectGsVideo(void) { return false; }
void PceBridge_GetVideoDebug(unsigned *w, unsigned *h, unsigned *pitchPixels,
                             int *fbw, int *nativeClass)
{ if (w) *w = 0; if (h) *h = 0; if (pitchPixels) *pitchPixels = 0;
  if (fbw) *fbw = 0; if (nativeClass) *nativeClass = 0; }
int PceBridge_GetNative240pRasterWidth(void) { return 0; }
bool PceBridge_DrawDirectGs(Uint32, Float32) { return false; }
unsigned PceBridge_GetSampleRate(void) { return 0; }

/* ------------------------------------------------------------------ */
/* Snes9x2010 bridge                                                   */
/* ------------------------------------------------------------------ */
bool Snes9x2010Bridge_Init(void) { return false; }
void Snes9x2010Bridge_Shutdown(void) {}
bool Snes9x2010Bridge_LoadGame(const void *, size_t, size_t, const char *) { return false; }
void Snes9x2010Bridge_UnloadGame(void) {}
void Snes9x2010Bridge_Reset(void) {}
void Snes9x2010Bridge_SoftReset(void) {}
void Snes9x2010Bridge_InvalidateGsResources(void) {}
bool Snes9x2010Bridge_CanDirectGsVideo(void) { return false; }
bool Snes9x2010Bridge_DrawDirectGs(Uint32, Float32) { return false; }
void Snes9x2010Bridge_RunFrame(Emu::SysInputT *, CRenderSurface *, CMixBuffer *) {}
Int32 Snes9x2010Bridge_GetStateSize(void) { return 0; }
Int32 Snes9x2010Bridge_SaveState(void *, Int32) { return 0; }
bool Snes9x2010Bridge_LoadState(const void *, Int32) { return false; }
Int32 Snes9x2010Bridge_GetSRAMBytes(void) { return 0; }
Uint8 *Snes9x2010Bridge_GetSRAMData(void) { return NULL; }
Uint32 Snes9x2010Bridge_GetSampleRate(void) { return 0; }


/* ================================================================== */
/* AURORA_GBA_ONLY_V2: SNES 原生核心 (src/snes) 桩                     */
/*                                                                    */
/* SRCS 已移除 38 个 src/snes 源文件; 以下为平台层引用的符号提供空实现。 */
/* 所有路径不可达: 浏览器只注册 .gba/.gb/.gbc/.sgb/.zip/.gz。          */
/* ================================================================== */

/* ---- 全局变量 (snrom.h / snppurender.h extern) ---- */
SnesForceRegionE g_SnesForceRegion = (SnesForceRegionE)0;
Uint32 g_FakeSRAMSize = 0;
Uint8 g_SnesSoftwareLayerMask = 0;
Uint8 g_SnesSoftwareHackFlags = 0;
Uint8 g_SnesObjLimitLevel = 0;
Uint8 g_SnesObjLimitMode = 0;

/* ---- CPU / SPC 执行入口 (mainloop_exec 调试路径引用) ---- */
extern "C" Int32 SNCPUExecute_ASM(SNCpuT *pCpu) { (void)pCpu; return 0; }
void SNCPUSetExecuteFunc(SNCpuExecuteFuncT pFunc) { (void)pFunc; }
void SNCPUSetOverclockLevel(SNCpuT *pCpu, Uint8 uLevel) { (void)pCpu; (void)uLevel; }
Int32 SNSPCExecute_C(SNSpcT *pSpc) { (void)pSpc; return 0; }
void SNSPCSetExecuteFunc(SNSpcExecuteFuncT pFunc) { (void)pFunc; }

/* ---- PPU 颜色 / 软件渲染设置 ---- */
void SNPPUColorCalibrate(const SNPPUColorCalibT *pCalib) { (void)pCalib; }
void SNPPUColorSetProfile(Int32 iProfile) { (void)iProfile; }
Uint32 *SNPPUColorGetPalette() { return NULL; }
void SNPPURenderSetObjLimitMode(Uint8 uMode) { (void)uMode; }
void SNPPURenderSetObjLimitLevel(Uint8 uLevel) { (void)uLevel; }
void SNPPURenderSetSoftwareHackFlags(Uint8 uFlags) { (void)uFlags; }
void SNPPURenderSetSoftwareLayerMask(Uint8 uMask) { (void)uMask; }
void SNPPURenderInvalidateGsResources(void) {}

/* ---- TurboFile / 音频 / 兼容 ---- */
void SnesAudioSetRate(Uint32 hz) { (void)hz; }
Bool SnesTurboFileEnabled(void) { return FALSE; }
Int32 SnesTurboFileGetBytes(void) { return 0; }
Uint8 *SnesTurboFileGetData(void) { return NULL; }
Bool SnesTurboFileDirty(void) { return FALSE; }
void SnesTurboFileClearDirty(void) {}
void SnesRomResetRuntimeCompatForExternalDevice(void) {}

/* ---- 状态对比 (mainloop_exec 状态校验) ---- */
void SNStateCompare(SnesStateT *pStateA, SnesStateT *pStateB)
{ (void)pStateA; (void)pStateB; }

/* ---- SnesIO / SNSuperWildCard ---- */
void SnesIO::SetMouseInput(Bool bConnected, Int32 nDeltaX, Int32 nDeltaY, Uint32 uButtons)
{ (void)bConnected; (void)nDeltaX; (void)nDeltaY; (void)uButtons; }
Bool SNSuperWildCard::ConsumeDebugTransition(SNSuperWildCard::DebugTransitionT *pOut)
{ (void)pOut; return FALSE; }

/* ---- SnesSystem ---- */
SnesSystem::SnesSystem()
{
    /* 清零 vptr 之后的对象体: 保证 IsSuperWildCard() 等内联访问器
     * 稳定返回 FALSE/NULL, 而非未初始化垃圾值。vptr 由编译器在进入
     * 构造体前设置, 必须保留。 */
    memset((char *)this + sizeof(void *), 0, sizeof(*this) - sizeof(void *));
}
SnesSystem::~SnesSystem() {}
void SnesSystem::SetRom(Emu::Rom *pRom) { (void)pRom; }
void SnesSystem::SetSnesRom(SnesRom *pRom) { (void)pRom; }
Bool SnesSystem::LoadSuperWildCard(const Char *pFirmwarePath, const Char *pDiskPath)
{ (void)pFirmwarePath; (void)pDiskPath; return FALSE; }
Bool SnesSystem::LoadSuperMagicom(const Char *pFirmwarePath, const Char *pDiskPath)
{ (void)pFirmwarePath; (void)pDiskPath; return FALSE; }
Bool SnesSystem::SwapSuperWildCardDisk(const Char *pDiskPath) { (void)pDiskPath; return FALSE; }
void SnesSystem::ShutdownSuperWildCard() {}
Bool SnesSystem::InsertSuperWildCardCartridge(SnesRom *pRom) { (void)pRom; return FALSE; }
void SnesSystem::EjectSuperWildCardCartridge() {}
void SnesSystem::Reset() {}
void SnesSystem::SoftReset() {}
void SnesSystem::ExecuteFrame(Emu::SysInputT *pInput, class CRenderSurface *pTarget, class CMixBuffer *pSound, ModeE eMode)
{ (void)pInput; (void)pTarget; (void)pSound; (void)eMode; }
void SnesSystem::SaveState(struct SnesStateT *pState) { (void)pState; }
Bool SnesSystem::RestoreState(struct SnesStateT *pState) { (void)pState; return FALSE; }
void SnesSystem::SaveState(void *pState, Int32 nStateBytes) { (void)pState; (void)nStateBytes; }
void SnesSystem::RestoreState(void *pState, Int32 nStateBytes) { (void)pState; (void)nStateBytes; }
Int32 SnesSystem::GetStateSize() { return 0; }
Bool SnesSystem::SaveStateChecked(void *pState, Int32 nStateBytes) { (void)pState; (void)nStateBytes; return FALSE; }
Bool SnesSystem::RestoreStateChecked(void *pState, Int32 nStateBytes) { (void)pState; (void)nStateBytes; return FALSE; }
Bool SnesSystem::CanSerializeCX4State() { return FALSE; }
Bool SnesSystem::CanSerializeSpecialChipState() { return FALSE; }
Int32 SnesSystem::GetSRAMBytes() { return 0; }
Uint8 *SnesSystem::GetSRAMData() { return NULL; }
const char *SnesSystem::GetString(StringE eString) { (void)eString; return ""; }
const Char *SnesSystem::GetRegName(Uint32 uAddr) { (void)uAddr; return ""; }

/* ---- SnesRom ---- */
SnesRom::SnesRom()
{
    /* 同 SnesSystem: 清零成员但保留 vptr。 */
    memset((char *)this + sizeof(void *), 0, sizeof(*this) - sizeof(void *));
}
SnesRom::~SnesRom() {}
Emu::Rom::LoadErrorE SnesRom::LoadRom(CDataIO *pFileIO, Uint8 *pBuffer, Uint32 nBufferBytes)
{ (void)pFileIO; (void)pBuffer; (void)nBufferBytes; return LOADERROR_INVALID; }
void SnesRom::Unload() {}
void SnesRom::SetCartInfo(SNRomInfoT *pCartInfo) { (void)pCartInfo; }
Uint32 SnesRom::GetNumExts() { return 0; }
Char *SnesRom::GetExtName(Uint32 uExt) { (void)uExt; return (Char *)""; }
Uint32 SnesRom::GetNumRomRegions() { return 0; }
Char *SnesRom::GetRomRegionName(Uint32 uRegion) { (void)uRegion; return (Char *)""; }
Uint32 SnesRom::GetRomRegionSize(Uint32 uRegion) { (void)uRegion; return 0; }
Char *SnesRom::GetRomTitle() { return (Char *)""; }
Char *SnesRom::GetMapperName() { return (Char *)""; }


/* ================================================================== */
/* AURORA_GBA_ONLY_V2: SnesSystem 成员类构造/析构桩                    */
/*                                                                    */
/* 桩的 SnesSystem 构造函数会触发编译器生成成员初始化代码, 以下类的   */
/* 默认构造/析构原本定义在被移除的 snes 实现文件中, 在此提供空实现。  */
/* 成员最终由 SnesSystem 构造体内的 memset 统一清零。                 */
/* ================================================================== */

SnesPPU::SnesPPU() {}
SnesIO::SnesIO() {}
SNSpcDsp::SNSpcDsp() {}
SNDSP1::SNDSP1() {}
SNDSP2::SNDSP2() {}
SNDSP4::SNDSP4() {}
SNOBC1::SNOBC1() {}
SNCX4::SNCX4() {}
SNGSU::SNGSU() {}
SNSDD1::SNSDD1() {}
SNSRTC::SNSRTC() {}
SNSuperWildCard::SNSuperWildCard() {}
SNSuperWildCard::~SNSuperWildCard() {}


/* ---- SNDSP1/2/4 的 ISNDSP 虚方法 (vtable 完整性所需) ---- */
void SNDSP1::Reset() {}
void SNDSP1::WriteData(Uint32 uAddr, Uint8 uData) { (void)uAddr; (void)uData; }
Uint8 SNDSP1::ReadData(Uint32 uAddr) { (void)uAddr; return 0; }
Uint8 SNDSP1::ReadStatus(Uint32 uAddr) { (void)uAddr; return 0; }

void SNDSP2::Reset() {}
void SNDSP2::WriteData(Uint32 uAddr, Uint8 uData) { (void)uAddr; (void)uData; }
Uint8 SNDSP2::ReadData(Uint32 uAddr) { (void)uAddr; return 0; }
Uint8 SNDSP2::ReadStatus(Uint32 uAddr) { (void)uAddr; return 0; }

void SNDSP4::Reset() {}
void SNDSP4::WriteData(Uint32 uAddr, Uint8 uData) { (void)uAddr; (void)uData; }
Uint8 SNDSP4::ReadData(Uint32 uAddr) { (void)uAddr; return 0; }
Uint8 SNDSP4::ReadStatus(Uint32 uAddr) { (void)uAddr; return 0; }
