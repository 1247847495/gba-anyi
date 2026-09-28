#ifndef _GBSYSTEM_H
#define _GBSYSTEM_H
/* AURORA_GAMBATTE_V1 */
#include "types.h"
#include "emusys.h"
#include "gbrom.h"
#define GB_STATE_MAGIC 0x54534247u
#define GB_STATE_VERSION 1u
struct GbStateHeaderT{Uint32 uMagic,uVersion,nPayloadBytes,uFrame,Reserved[4];};
class GbSystem:public Emu::System
{
public:
    GbSystem();virtual ~GbSystem();virtual void SetRom(Emu::Rom*);
    virtual void Reset();virtual void SoftReset();
    virtual void ExecuteFrame(Emu::SysInputT*,class CRenderSurface*,class CMixBuffer*,ModeE);
    virtual Int32 GetStateSize();virtual void SaveState(void*,Int32);virtual void RestoreState(void*,Int32);
    virtual Int32 GetSRAMBytes();virtual Uint8 *GetSRAMData();virtual const char *GetString(StringE);virtual Uint32 GetSampleRate();
    Bool SaveStateChecked(void*,Int32);Bool RestoreStateChecked(const void*,Int32);Bool IsRomReady()const{return m_bRomReady;}
private:GbRom *m_pGbRom;Bool m_bRomReady;Int32 m_nCachedStateBytes;
};
#endif
