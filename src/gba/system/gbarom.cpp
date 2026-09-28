/* AURORA_GPSP_V1 */
#include <stdio.h>
#include <string.h>
#include "gbarom.h"
#include "dataio.h"
static Char *s_GbaExts[]={(Char*)"gba"};
GbaRom::GbaRom(){m_bLoaded=FALSE;m_szSourceName[0]=0;}
GbaRom::~GbaRom(){Unload();}
void GbaRom::Unload(){m_szSourceName[0]=0;m_bLoaded=FALSE;}
void GbaRom::SetSourceName(const Char *p){if(!p){m_szSourceName[0]=0;return;}strncpy(m_szSourceName,p,sizeof(m_szSourceName)-1);m_szSourceName[sizeof(m_szSourceName)-1]=0;}
Emu::Rom::LoadErrorE GbaRom::LoadRom(CDataIO *io,Uint8 *buf,Uint32 cap)
{
    /* gpSP needs no frontend buffer; the browser only needs the file to
     * exist. Consume the stream so IsLoaded() reflects reality. */
    (void)buf; (void)cap;
    Unload();
    if(!io)return LOADERROR_OPENFILE;
    io->Seek(0,SEEK_END);Uint32 n=(Uint32)io->GetPos();io->Seek(0,SEEK_SET);
    if(!n)return LOADERROR_BADROMSIZE;
    m_bLoaded=TRUE;return LOADERROR_NONE;
}
Uint32 GbaRom::GetNumExts(){return sizeof(s_GbaExts)/sizeof(s_GbaExts[0]);}
Char *GbaRom::GetExtName(Uint32 i){return i<GetNumExts()?s_GbaExts[i]:NULL;}
Uint32 GbaRom::GetNumRomRegions(){return m_bLoaded?1:0;}
Char *GbaRom::GetRomRegionName(Uint32 i){return i==0&&m_bLoaded?(Char*)"Cartridge":NULL;}
Uint32 GbaRom::GetRomRegionSize(Uint32 i){return i==0&&m_bLoaded?0:0;}
Char *GbaRom::GetMapperName(){return (Char*)"gpSP";}
Char *GbaRom::GetRomTitle(){return m_szSourceName[0]?m_szSourceName:(Char*)"Game Boy Advance";}
