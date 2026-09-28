#ifndef _GBASYSTEM_H
#define _GBASYSTEM_H
/* AURORA_GPSP_V1 */
#include "types.h"
#include "emusys.h"
#include "gbarom.h"
#define GBA_STATE_MAGIC 0x54534247u
#define GBA_STATE_VERSION 1u
struct GbaStateHeaderT{Uint32 uMagic,uVersion,nPayloadBytes,uFrame,Reserved[4];};
class GbaSystem:public Emu::System
{
public:
    GbaSystem();virtual ~GbaSystem();virtual void SetRom(Emu::Rom*);
    /* Path-based load: gpSP streams the cartridge from storage. */
    Bool LoadCart(const Char *pPath, const Char *pSystemPath);
    /* AURORA_GBA_ZIP_MEM_V1: memory-based load - gpSP's ROM cache was
       filled directly by the frontend (ZIP/GZ sink extraction). */
    Bool LoadCartMemory(const Char *pPath, const Char *pSystemPath, Int32 nRomBytes);
    virtual void Reset();virtual void SoftReset();
    virtual void ExecuteFrame(Emu::SysInputT*,class CRenderSurface*,class CMixBuffer*,ModeE);
    virtual Int32 GetStateSize();virtual void SaveState(void*,Int32);virtual void RestoreState(void*,Int32);
    virtual Int32 GetSRAMBytes();virtual Uint8 *GetSRAMData();virtual const char *GetString(StringE);virtual Uint32 GetSampleRate();
    Bool SaveStateChecked(void*,Int32);Bool RestoreStateChecked(const void*,Int32);Bool IsRomReady()const{return m_bRomReady;}
private:Bool m_bRomReady;Int32 m_nCachedStateBytes;
};
#endif
