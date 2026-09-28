#pragma once

#include "types.h"

class CScreen;

#ifdef __cplusplus
extern "C" {
#endif

void MainLoopModalPrintf(Int32 Time, const Char *pFormat, ...);
void MainLoopStatusPrintf(Int32 Time, const Char *pFormat, ...);

/* AURORA_MODAL_SILENT_V23_20260912: MainLoopModalPrintf twin that keeps the
   full frame-pumping side effects but hides the modal text (see
   mainloop_ui.cpp). Used by the GB/GBC diag markers only. */
void MainLoopModalPrintfSilent(Int32 Time, const Char *pFormat, ...);

/* AURORA_BOOT_MARK_V1_20260912: boot-phase marker for the EE exception
   screen.  MainLoopModalPrintf is unusable before the GS / font / log
   screen exist (it pumps MainLoopRender() frames); this variant ONLY
   overwrites _MainLoop_ModalStr and never touches _MainLoop_ModalCount,
   so nothing is rendered and nothing on screen changes.  The string sits
   in BSS purely so excepHandler.c's "Last marker:" line can name the
   exact boot sub-step that was running when the EE died. */
void BootModalMark(const char *pFormat, ...);

void ScrPrintf(const Char *pFormat, ...);

/* Boot import log: resumo limpo "IOP imported: OK/BAD" no boot splash.
   BootImport(name, ret) guarda falha (ret<0); BootImportFlush() imprime. */
void BootImport(const char *pName, int ret);
void BootImportFlush(void);
void BootMark(const char *pLabel);

#ifdef __cplusplus
}
#endif

void _MainLoopSetScreen(CScreen *pScreen);
void _UICycle(int dir);
void _MainLoopCycleScreen(int dir);
void MainLoopShutdown();
