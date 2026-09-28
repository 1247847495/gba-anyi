/* AURORA_GAMBATTE_V1 */
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "gbsystem.h"
#include "gb/gambatte/gambatte_bridge.h"
extern GbRom *_pGbRom;
GbSystem::GbSystem(){m_pGbRom=NULL;m_bRomReady=FALSE;m_nCachedStateBytes=0;}
GbSystem::~GbSystem(){GbBridge_Shutdown();}
void GbSystem::SetRom(Emu::Rom *r)
{
    m_pGbRom=NULL;m_bRomReady=FALSE;m_nCachedStateBytes=0;
    if(!r){GbBridge_Shutdown();return;}
    GbBridge_UnloadGame();
    if(r!=_pGbRom){printf("[GbSystem] rejected non-GbRom object\n");return;}
    GbRom *rom=(GbRom*)r;
    if(!rom->GetData()||!rom->GetBytes())return;
    const char *name=rom->GetSourceName();
    if(!name||!*name)name="game.gb";
    if(!GbBridge_LoadGame(rom->GetData(),(size_t)rom->GetBytes(),(size_t)rom->GetCapacity(),name))
    {
        printf("[GbSystem/Gambatte] LOAD FAILED\n");
        return;
    }
    m_pGbRom=rom;m_bRomReady=TRUE;
    printf("[GbSystem/Gambatte] LOAD OK; SRAM=%d\n",GbBridge_GetSRAMBytes());
}
void GbSystem::Reset(){if(m_bRomReady)GbBridge_Reset();}
void GbSystem::SoftReset(){if(m_bRomReady)GbBridge_Reset();}
void GbSystem::ExecuteFrame(Emu::SysInputT *in,CRenderSurface *dst,CMixBuffer *mix,ModeE mode){(void)mode;if(!m_bRomReady)return;GbBridge_RunFrame(in,dst,mix);}
Int32 GbSystem::GetStateSize(){if(!m_bRomReady)return 0;if(m_nCachedStateBytes>0)return m_nCachedStateBytes;Int32 p=GbBridge_GetStateSize();if(p<=0||p>INT_MAX-(Int32)sizeof(GbStateHeaderT))return 0;m_nCachedStateBytes=(Int32)sizeof(GbStateHeaderT)+p;return m_nCachedStateBytes;}
void GbSystem::SaveState(void *p,Int32 n){(void)SaveStateChecked(p,n);}
void GbSystem::RestoreState(void *p,Int32 n){(void)RestoreStateChecked(p,n);}
Bool GbSystem::SaveStateChecked(void *p,Int32 n)
{
    if(!p||!m_bRomReady)return FALSE;
    Int32 total=GetStateSize();
    if(total<=(Int32)sizeof(GbStateHeaderT)||n<total)return FALSE;
    GbStateHeaderT h;memset(&h,0,sizeof(h));
    Int32 bytes=total-(Int32)sizeof(h);
    Uint8 *payload=((Uint8*)p)+sizeof(h);
    memset(payload,0,(size_t)bytes);
    Int32 wrote=GbBridge_SaveState(payload,bytes);
    if(wrote!=bytes)return FALSE;
    h.uMagic=GB_STATE_MAGIC;h.uVersion=GB_STATE_VERSION;h.nPayloadBytes=(Uint32)wrote;h.uFrame=m_uFrame;
    memcpy(p,&h,sizeof(h));return TRUE;
}
Bool GbSystem::RestoreStateChecked(const void *p,Int32 n)
{
    if(!p||!m_bRomReady||n<(Int32)sizeof(GbStateHeaderT))return FALSE;
    GbStateHeaderT h;memcpy(&h,p,sizeof(h));
    if(h.uMagic!=GB_STATE_MAGIC||h.uVersion!=GB_STATE_VERSION||!h.nPayloadBytes||h.nPayloadBytes>(Uint32)(n-(Int32)sizeof(h)))return FALSE;
    Int32 expected=GetStateSize();
    if(expected<=0||(Uint32)expected!=(Uint32)sizeof(h)+h.nPayloadBytes)return FALSE;
    if(!GbBridge_LoadState(((const Uint8*)p)+sizeof(h),(Int32)h.nPayloadBytes))return FALSE;
    m_uFrame=h.uFrame;return TRUE;
}
Int32 GbSystem::GetSRAMBytes(){return m_bRomReady?GbBridge_GetSRAMBytes():0;}
Uint8 *GbSystem::GetSRAMData(){return m_bRomReady?GbBridge_GetSRAMData():NULL;}
const char *GbSystem::GetString(StringE s)
{
    switch(s)
    {
        case STRING_SHORTNAME:return "GB";
        case STRING_FULLNAME:return "Game Boy / Gambatte";
        case STRING_SRAMEXT:return "srm";
        case STRING_STATEEXT:return "gbt";
    }
    return "";
}
Uint32 GbSystem::GetSampleRate(){return GbBridge_GetSampleRate();}
