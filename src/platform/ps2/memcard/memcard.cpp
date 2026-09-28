
#include <kernel.h>
#include <libmc.h>
#define NEWLIB_PORT_AWARE
#include <fileXio.h>
#include <fileXio_rpc.h>
#undef NEWLIB_PORT_AWARE
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "types.h"

#include "memcard.h"

static Uint8 _MemCard_IconData[]={
#include "memcard_icon.inc"
};

static Bool _MemCard_bInitialized=FALSE;

/* PS2SDK extends the newlib errno table with EFORMAT. Keep the fallback
   for older SDK snapshots that expose the mcman result but not the name. */
#ifndef EFORMAT
#define EFORMAT 47
#endif

MemCardStatusE MemCardGetStatus(Int32 iPort)
{
	char Root[8];
	iox_stat_t Status;
	int Result;

	if (!_MemCard_bInitialized || iPort < 0 || iPort > 1)
	{
		return MEMCARD_STATUS_ERROR;
	}

	snprintf(Root, sizeof(Root), "mc%d:/", (int)iPort);
	memset(&Status, 0, sizeof(Status));
	Result = fileXioGetStat(Root, &Status);
	printf("MemCard: status('%s') -> %d\n", Root, Result);

	if (Result >= 0)
	{
		return MEMCARD_STATUS_READY;
	}

	if (Result == -EFORMAT)
	{
		return MEMCARD_STATUS_UNFORMATTED;
	}

	/* mcman maps no-card/detection failures to ENXIO. Treat the other
	   ordinary path/device failures as absent too; callers only need to
	   distinguish the destructive, user-actionable unformatted case. */
	if (Result == -ENXIO || Result == -ENODEV || Result == -ENOENT)
	{
		return MEMCARD_STATUS_NOT_PRESENT;
	}

	return MEMCARD_STATUS_ERROR;
}

Bool MemCardFormat(Int32 iPort)
{
	char Device[8];
	int Result;

	if (!_MemCard_bInitialized || iPort < 0 || iPort > 1)
	{
		return FALSE;
	}

	snprintf(Device, sizeof(Device), "mc%d:", (int)iPort);

	/* Use fileXio -> iomanX -> mcman's format operation. This deliberately
	   avoids libmc/mcInit: its separate MCSERV RPC bind can hang on the same
	   real-hardware/NetherSX2 setups for which all normal card I/O was moved
	   to newlib stdio in the first place. */
	Result = fileXioFormat(Device, NULL, NULL, 0);
	printf("MemCard: format('%s') -> %d\n", Device, Result);
	return Result >= 0 ? TRUE : FALSE;
}

/* Unicode -> Shift-JIS 映射表（JIS X 0208 子集 + 简体字别名），
   由 tools/gen_sjis_table.py 自动生成，按 Unicode 升序供二分查找。 */
#include "mc_sjis_table.inc"

/* icon.sys 的 title 转 Shift-JIS。PS2 浏览器(OSDSYS)只认 SJIS 编码且
   BIOS 字库仅覆盖 JIS X 0208：ASCII 按全角映射（1 字节 ASCII 在 BIOS
   浏览器中不显示）；中文(UTF-8)先解码成 Unicode 码点再查表转 SJIS，
   简体特有字（拟/汉/戏 等）由表内别名映射为同义日文汉字（擬/漢/戲），
   字库中不存在的字符退化为全角 '?'。
   title 容量：unsigned short[34]，两行各 32 字节(nlOffset=32)，
   最多写入 64 字节。返回写入的字节数。 */
static unsigned short _McUtf8ToSjis(unsigned int cp)
{
	int lo = 0, hi = MC_SJIS_TABLE_SIZE - 1;
	while (lo <= hi)
	{
		int mid = (lo + hi) / 2;
		if (_McSjisTable[mid][0] == cp)
			return _McSjisTable[mid][1];
		if (_McSjisTable[mid][0] < cp)
			lo = mid + 1;
		else
			hi = mid - 1;
	}
	return 0;
}

static int _TitleToSjis(unsigned char *pDst, const char *pSrc)
{
	const unsigned char *p = (const unsigned char *)pSrc;
	int n = 0;

	while (*p && n + 2 <= 64)
	{
		unsigned int cp;
		unsigned short sj;
		int len;

		if (p[0] < 0x80)
		{
			cp = p[0]; len = 1;
			if      (cp == ' ')             sj = 0x8140;            /* 全角空格 */
			else if (cp >= '0' && cp <= '9') sj = 0x824F + (cp - '0');
			else if (cp >= 'A' && cp <= 'Z') sj = 0x8260 + (cp - 'A');
			else if (cp >= 'a' && cp <= 'z') sj = 0x8281 + (cp - 'a');
			else                            sj = 0x8148;            /* '?' */
		}
		else
		{
			if ((p[0] & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80)
			{
				cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F); len = 2;
			}
			else if ((p[0] & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80
			      && (p[2] & 0xC0) == 0x80)
			{
				cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6)
				   | (p[2] & 0x3F); len = 3;
			}
			else
			{
				cp = 0; len = 1;   /* 无效 UTF-8 序列 -> '?' */
			}
			sj = cp ? _McUtf8ToSjis(cp) : 0;
			if (!sj)
				sj = 0x8148;       /* 字库未覆盖 -> '?' */
		}

		pDst[n++] = (unsigned char)(sj >> 8);
		pDst[n++] = (unsigned char)(sj & 0xFF);
		p += len;
	}
	return n;
}

int MemCardCreateSave(char *pDir, char *pTitle, Bool bForceWrite)
{
	int icon_size;
	char* icon_buffer;
	mcIcon icon_sys;
	char Path[256];
	int mkRet;

	static iconIVECTOR bgcolor[4] = {
		{  68,  23, 116,  0 },
		{ 255, 255, 255,  0 },
		{ 255, 255, 255,  0 },
		{  68,  23, 116,  0 },
	};

	static iconFVECTOR lightdir[3] = {
		{ 0.5, 0.5, 0.5, 0.0 },
		{ 0.0,-0.4,-0.1, 0.0 },
		{-0.5,-0.5, 0.5, 0.0 },
	};

	static iconFVECTOR lightcol[3] = {
		{ 0.30, 0.30, 0.30, 0.00 },
		{ 0.35, 0.35, 0.35, 0.00 },
		{ 0.40, 0.40, 0.40, 0.00 },
	};

	/* Ambiente alto (0.55) para dar BRILHO uniforme em todas as faces -
	   inclusive as que nao encaram as luzes direcionais (laterais). */
	static iconFVECTOR ambient = { 0.55, 0.55, 0.55, 0.00 };

	printf("MemCard: CreateSave('%s', '%s', force=%d) init=%d\n",
	       pDir, pTitle, (int)bForceWrite, (int)_MemCard_bInitialized);

	if (!_MemCard_bInitialized)
	{
		printf("MemCard: CreateSave bail (mc not initialized)\n");
		return -1;
	}

	/* mkdir() goes through iomanX -> mcman, which accepts paths of
	   the form "mc0:/GBAticle". On a fresh card the directory does
	   not exist and mkdir returns 0. On an already-populated card it
	   returns -1 with errno=EEXIST, which is fine: the existing files
	   will be overwritten below. */
	mkRet = mkdir(pDir, 0777);
	printf("MemCard: mkdir('%s') -> %d (errno=%d)\n",
	       pDir, mkRet, mkRet < 0 ? errno : 0);

	if (mkRet < 0 && errno != EEXIST)
	{
		if (!bForceWrite)
		{
			printf("MemCard: CreateSave bail (mkdir failed, no force)\n");
		 	return -1;
		}
	}

	memset(&icon_sys, 0, sizeof(mcIcon));
	strcpy((char *)icon_sys.head, "PS2D");
	/* Titulo em Shift-JIS fullwidth (obrigatorio no OSDSYS). */
	_TitleToSjis((unsigned char *)&icon_sys.title, pTitle);
	/* nlOffset = byte onde a 2a linha comeca.  "SNESticle Aurora" = 16
	   chars * 2 bytes = 32 bytes; nlOffset=32 (= fim do texto) deixa
	   tudo numa LINHA SO' (2a linha vazia). */
	icon_sys.nlOffset = 32;
	icon_sys.trans = 0x60;
	memcpy(icon_sys.bgCol, bgcolor, sizeof(bgcolor));
	memcpy(icon_sys.lightDir, lightdir, sizeof(lightdir));
	memcpy(icon_sys.lightCol, lightcol, sizeof(lightcol));
	memcpy(icon_sys.lightAmbient, ambient, sizeof(ambient));
	strcpy((char *)icon_sys.view, "icon.icn");
	strcpy((char *)icon_sys.copy, "icon.icn");
	strcpy((char *)icon_sys.del, "icon.icn");

	sprintf(Path, "%s/icon.sys", pDir);
	if (!MemCardWriteFile(Path, (Uint8 *)&icon_sys, sizeof(icon_sys)))
	{
		printf("MemCard: icon.sys write failed\n");
		return -5;
	}

	icon_size = sizeof(_MemCard_IconData);
	icon_buffer = (char *)_MemCard_IconData;

	sprintf(Path, "%s/icon.icn", pDir);
	if (!MemCardWriteFile(Path, (Uint8 *)icon_buffer, icon_size))
	{
		printf("MemCard: icon.icn write failed\n");
		return -6;
	}
	printf("MemCard: CreateSave OK\n");
	return 0;
}

Bool MemCardCheckNewCard()
{
	/* Hot-swap detection used to rely on libmc's mcGetInfo + mcSync,
	   which talks to MCMAN/MCSERV over SIF RPC 0x80000400.  All other
	   memcard I/O in this build goes through newlib stdio + iomanX
	   (fopen / fwrite / fread / mkdir on mc0:/), which does NOT need
	   that RPC, so we drop the libmc dependency entirely.  Card-swap
	   detection mid-game is a nice-to-have, not a correctness
	   requirement, and skipping it makes the code work in environments
	   (NetherSX2, certain BIOS revisions) where the legacy MCMAN/MCSERV
	   RPC does not get registered. */
	return FALSE;
}

void MemCardInit()
{
	/* We used to call mcInit(MC_TYPE_MC) here.  mcInit's first action
	   is sceSifBindRpc(&g_cdata, 0x80000400, 0) in a do/while spin loop
	   that waits until g_cdata.server is non-NULL.  On NetherSX2 / on
	   real PS2 with the SCPH BIOS, RPC 0x80000400 is registered by the
	   IOP-side mcserv module; in our boot, ps2_drivers'
	   init_memcard_driver(true) loads sio2man.irx + mcman.irx +
	   mcserv.irx from its embedded buffers but - for reasons that are
	   still unclear (probably a clash with rom0:XSIO2MAN loaded later
	   in mainloop_iop.cpp) - the mcserv RPC server never registers, so
	   that do/while spin loop hangs forever.

	   We do not actually need libmc to read or write SRAM: the whole
	   memcard.cpp file now uses newlib stdio (fopen / fread / fwrite /
	   fclose / mkdir on mc0:/...), which goes through fileXio -> iomanX
	   -> mcman.irx and never touches the libmc RPC client.  So we just
	   declare the subsystem initialised and let actual file I/O
	   surface any remaining problems with concrete fopen errno values
	   instead of an infinite SifBindRpc spin. */
	printf("MemCard: stdio-only init (skipping libmc mcInit)\n");
	_MemCard_bInitialized = TRUE;
}

/* All memcard I/O now goes through newlib stdio (fopen/fread/fwrite/
   fclose). Once init_ps2_filesystem_driver() has run, mc paths of the
   form "mc0:/GBAticle/<rom>.srm" are handled by iomanX -> mcman.irx
   directly, which is the same path picodrive / OPL / uLE use and is
   much more reliable than the legacy rom0:FILEIO RPC the project
   used before (fioOpen / fioRead).

   In particular, with newlib stdio the SRAM read path actually
   returns the full requested byte count on the first try, instead of
   the partial / zero-byte reads we saw with fioRead on real hardware
   - which was the root cause of "save works but load does not". */

Bool MemCardWriteFile(char *pPath, Uint8 *pData, Uint32 nBytes)
{
	FILE *fp;
	size_t result;

	if (!_MemCard_bInitialized)
	{
		printf("MemCard: Write skipped (not init): %s\n", pPath);
		return FALSE;
	}

	fp = fopen(pPath, "wb");
	printf("MemCard: fopen-W('%s') -> %p\n", pPath, fp);
	if (!fp)
	{
		printf("MemCard: Write FAIL open: %s (errno=%d)\n", pPath, errno);
		return FALSE;
	}

	result = fwrite(pData, 1, nBytes, fp);
	fflush(fp);
	fclose(fp);
	printf("MemCard: fwrite('%s') %u/%u%s\n",
	       pPath, (unsigned)result, (unsigned)nBytes,
	       (result == nBytes) ? "" : " <<< MISMATCH");
	return (result == nBytes);
}

Bool MemCardReadFile(char *pPath, Uint8 *pData, Uint32 nBytes)
{
	FILE *fp;
	size_t result;

	printf("MemCard: ReadFile('%s', %u) init=%d\n",
	       pPath, (unsigned)nBytes, (int)_MemCard_bInitialized);

	if (!_MemCard_bInitialized)
	{
		printf("MemCard: Read skipped (not init): %s\n", pPath);
		return FALSE;
	}

	fp = fopen(pPath, "rb");
	printf("MemCard: fopen-R('%s') -> %p\n", pPath, fp);
	if (!fp)
	{
		printf("MemCard: Read FAIL open: %s (errno=%d)\n", pPath, errno);
		return FALSE;
	}

	result = fread(pData, 1, nBytes, fp);
	fclose(fp);
	printf("MemCard: fread('%s') %u/%u%s\n",
	       pPath, (unsigned)result, (unsigned)nBytes,
	       (result == nBytes) ? "" : " <<< MISMATCH");
	return (result == nBytes);
}
