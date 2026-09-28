/* AURORA_GPSP_V1 */
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "gbasystem.h"
#include "gba/gpsp/gpsp_bridge.h"
GbaSystem::GbaSystem(){m_bRomReady=FALSE;m_nCachedStateBytes=0;}
GbaSystem::~GbaSystem(){GbaBridge_Shutdown();}
void GbaSystem::SetRom(Emu::Rom *r)
{
    /* Browser launches go through LoadCart(); SetRom(NULL) is the
     * unload path used when the frontend tears the previous game down. */
    m_bRomReady=FALSE;m_nCachedStateBytes=0;
    if(!r){GbaBridge_Shutdown();return;}
    printf("[GbaSystem] use LoadCart for gpSP cartridges\n");
}
Bool GbaSystem::LoadCart(const Char *path, const Char *systemPath)
{
    m_bRomReady=FALSE;m_nCachedStateBytes=0;
    if(!GbaBridge_LoadCart(path,systemPath))
    {
        printf("[GbaSystem/gpSP] LOAD FAILED\n");
        return FALSE;
    }
    m_bRomReady=TRUE;
    printf("[GbaSystem/gpSP] LOAD OK; SRAM=%d\n",GbaBridge_GetSRAMBytes());
    return TRUE;
}
Bool GbaSystem::LoadCartMemory(const Char *path, const Char *systemPath, Int32 nRomBytes)
{
    m_bRomReady=FALSE;m_nCachedStateBytes=0;
    if(nRomBytes<=0||!GbaBridge_LoadCartMemory(path,systemPath,(unsigned)nRomBytes))
    {
        printf("[GbaSystem/gpSP] MEM LOAD FAILED\n");
        return FALSE;
    }
    m_bRomReady=TRUE;
    printf("[GbaSystem/gpSP] MEM LOAD OK; SRAM=%d\n",GbaBridge_GetSRAMBytes());
    return TRUE;
}
void GbaSystem::Reset(){if(m_bRomReady)GbaBridge_Reset();}
void GbaSystem::SoftReset(){if(m_bRomReady)GbaBridge_Reset();}
void GbaSystem::ExecuteFrame(Emu::SysInputT *in,CRenderSurface *dst,CMixBuffer *mix,ModeE mode){(void)mode;if(!m_bRomReady)return;GbaBridge_RunFrame(in,dst,mix);}
Int32 GbaSystem::GetStateSize(){if(!m_bRomReady)return 0;if(m_nCachedStateBytes>0)return m_nCachedStateBytes;Int32 p=GbaBridge_GetStateSize();if(p<=0||p>INT_MAX-(Int32)sizeof(GbaStateHeaderT))return 0;m_nCachedStateBytes=(Int32)sizeof(GbaStateHeaderT)+p;return m_nCachedStateBytes;}
void GbaSystem::SaveState(void *p,Int32 n){(void)SaveStateChecked(p,n);}
void GbaSystem::RestoreState(void *p,Int32 n){(void)RestoreStateChecked(p,n);}
Bool GbaSystem::SaveStateChecked(void *p,Int32 n)
{
    if(!p||!m_bRomReady)return FALSE;
    Int32 total=GetStateSize();
    if(total<=(Int32)sizeof(GbaStateHeaderT)||n<total)return FALSE;
    GbaStateHeaderT h;memset(&h,0,sizeof(h));
    Int32 bytes=total-(Int32)sizeof(h);
    Uint8 *payload=((Uint8*)p)+sizeof(h);
    memset(payload,0,(size_t)bytes);
    Int32 wrote=GbaBridge_SaveState(payload,bytes);
    if(wrote!=bytes)return FALSE;
    h.uMagic=GBA_STATE_MAGIC;h.uVersion=GBA_STATE_VERSION;h.nPayloadBytes=(Uint32)wrote;h.uFrame=m_uFrame;
    memcpy(p,&h,sizeof(h));return TRUE;
}
Bool GbaSystem::RestoreStateChecked(const void *p,Int32 n)
{
    if(!p||!m_bRomReady||n<(Int32)sizeof(GbaStateHeaderT))return FALSE;
    GbaStateHeaderT h;memcpy(&h,p,sizeof(h));
    if(h.uMagic!=GBA_STATE_MAGIC||h.uVersion!=GBA_STATE_VERSION||!h.nPayloadBytes||h.nPayloadBytes>(Uint32)(n-(Int32)sizeof(h)))return FALSE;
    Int32 expected=GetStateSize();
    if(expected<=0||(Uint32)expected!=(Uint32)sizeof(h)+h.nPayloadBytes)return FALSE;
    if(!GbaBridge_LoadState(((const Uint8*)p)+sizeof(h),(Int32)h.nPayloadBytes))return FALSE;
    m_uFrame=h.uFrame;return TRUE;
}
Int32 GbaSystem::GetSRAMBytes(){return m_bRomReady?GbaBridge_GetSRAMBytes():0;}
Uint8 *GbaSystem::GetSRAMData(){return m_bRomReady?GbaBridge_GetSRAMData():NULL;}
const char *GbaSystem::GetString(StringE s)
{
    switch(s)
    {
        case STRING_SHORTNAME:return "GBA";
        case STRING_FULLNAME:return "Game Boy Advance / gpSP";
        case STRING_SRAMEXT:return "srm";
        case STRING_STATEEXT:return "gst";
    }
    return "";
}
Uint32 GbaSystem::GetSampleRate(){return GbaBridge_GetSampleRate();}
