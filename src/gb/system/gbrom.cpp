/* AURORA_GAMBATTE_V1 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gbrom.h"
#include "dataio.h"
static Char *s_GbExts[]={(Char*)"gb",(Char*)"gbc",(Char*)"sgb"};
GbRom::GbRom(){m_bLoaded=FALSE;m_pRomMem=NULL;m_pRomData=NULL;m_uRomBytes=0;m_uRomCapacity=0;m_szSourceName[0]=0;}
GbRom::~GbRom(){Unload();}
void GbRom::Unload(){if(m_pRomMem)free(m_pRomMem);m_pRomMem=NULL;m_pRomData=NULL;m_uRomBytes=0;m_uRomCapacity=0;m_szSourceName[0]=0;m_bLoaded=FALSE;}
void GbRom::SetSourceName(const Char *p){if(!p){m_szSourceName[0]=0;return;}strncpy(m_szSourceName,p,sizeof(m_szSourceName)-1);m_szSourceName[sizeof(m_szSourceName)-1]=0;}
Emu::Rom::LoadErrorE GbRom::LoadRom(CDataIO *io,Uint8 *buf,Uint32 cap){Unload();if(!io)return LOADERROR_OPENFILE;io->Seek(0,SEEK_END);Uint32 n=(Uint32)io->GetPos();io->Seek(0,SEEK_SET);if(!n)return LOADERROR_BADROMSIZE;Uint8 *p=NULL;if(buf&&cap>=n)p=buf;else{m_pRomMem=(Uint8*)malloc(n);if(!m_pRomMem)return LOADERROR_OUTOFSPACE;p=m_pRomMem;}size_t got=io->Read(p,(Int32)n);if(got!=n){Unload();return LOADERROR_READFILE;}m_pRomData=p;m_uRomBytes=n;m_uRomCapacity=(buf&&p==buf)?cap:n;m_bLoaded=TRUE;return LOADERROR_NONE;}
Emu::Rom::LoadErrorE GbRom::AttachBuffer(Uint8 *p,Uint32 n,Uint32 cap){Unload();if(!p||!n||cap<n)return LOADERROR_BADROMSIZE;m_pRomData=p;m_uRomBytes=n;m_uRomCapacity=cap;m_bLoaded=TRUE;printf("[GbRom] attached Aurora ROM buffer: %u/%u bytes\n",(unsigned)n,(unsigned)cap);return LOADERROR_NONE;}
Uint32 GbRom::GetNumExts(){return sizeof(s_GbExts)/sizeof(s_GbExts[0]);}
Char *GbRom::GetExtName(Uint32 i){return i<GetNumExts()?s_GbExts[i]:NULL;}
Uint32 GbRom::GetNumRomRegions(){return m_bLoaded?1:0;}
Char *GbRom::GetRomRegionName(Uint32 i){return i==0&&m_bLoaded?(Char*)"Cartridge":NULL;}
Uint32 GbRom::GetRomRegionSize(Uint32 i){return i==0&&m_bLoaded?m_uRomBytes:0;}
Char *GbRom::GetMapperName(){return (Char*)"Gambatte";}
Char *GbRom::GetRomTitle(){return m_szSourceName[0]?m_szSourceName:(Char*)"Game Boy";}
