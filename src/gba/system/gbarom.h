#ifndef _GBAROM_H
#define _GBAROM_H
/* AURORA_GPSP_V1: browser-side GBA cartridge descriptor.
 *
 * gpSP streams the cartridge from storage itself (16 MiB LRU window),
 * so this class exists for extension registration and title display
 * only; no frontend-side ROM image is retained.
 */
#include "types.h"
#include "emurom.h"
class GbaRom : public Emu::Rom
{
public:
    GbaRom(); virtual ~GbaRom();
    virtual LoadErrorE LoadRom(CDataIO *pFileIO, Uint8 *pBuffer=NULL, Uint32 nBufferBytes=0);
    virtual void Unload(); virtual Uint32 GetNumExts(); virtual Char *GetExtName(Uint32 uExt);
    virtual Uint32 GetNumRomRegions(); virtual Char *GetRomRegionName(Uint32 uRegion); virtual Uint32 GetRomRegionSize(Uint32 uRegion);
    virtual Char *GetMapperName(); virtual Char *GetRomTitle();
    void SetSourceName(const Char *pName); const Char *GetSourceName() const{return m_szSourceName;}
private:
    Bool m_bLoaded; Char m_szSourceName[1024];
};
#endif
