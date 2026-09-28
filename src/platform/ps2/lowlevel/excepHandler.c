/*********************************************************************
 * Copyright (C) 2003 Tord Lindstrom (pukko@home.se)
 * This file is subject to the terms and conditions of the PS2Link License.
 * See the file LICENSE in the main directory of this distribution for more
 * details.
 */

#include <tamtypes.h>
#include <kernel.h>
#include <debug.h>
#include "excepHandler.h"

extern void *_gp;
extern int userThreadID;

////////////////////////////////////////////////////////////////////////
typedef union 
{
    unsigned int  uint128 __attribute__(( mode(TI) ));
    unsigned long long uint64[2];
} eeReg __attribute__((packed));

////////////////////////////////////////////////////////////////////////
// Prototypes
/* AURORA_EE_EXCEPTION_SCREEN_V1_20260906: the regs pointer argument was
   dropped; the assembly trampoline (excepHandler.s) stores everything into
   _savedRegs before calling, so the C side reads the global directly. */
void pkoDebug(int cause, int badvaddr, int status, int epc);

extern void pkoExceptionHandler(void);

/* Last modal debug marker painted by MainLoopModalPrintf; printing it here
   names the exact load sub-step that was executing when the EE died. */
extern char _MainLoop_ModalStr[256];

////////////////////////////////////////////////////////////////////////
static const unsigned char regName[32][5] =
{
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "t8", "t9", "s0", "s1", "s2", "s3", "s4", "s5",
    "s6", "s7", "k0", "k1", "gp", "sp", "fp", "ra"
};

static char codeTxt[13][24] =
{
    "Interrupt", "TLB modification", "TLB load/inst fetch", "TLB store",
    "Address load/inst fetch", "Address store", "Bus error (instr)",
    "Bus error (data)", "Syscall", "Breakpoint", "Reserved instruction",
    "Coprocessor unusable", "Arithmetic overflow"
};

char _exceptionStack[8*1024] __attribute__((aligned(16)));
eeReg _savedRegs[32+4] __attribute__((aligned(16)));

////////////////////////////////////////////////////////////////////////
// The 'exception handler', only dumps registers to console or screen atm
void
pkoDebug(int cause, int badvaddr, int status, int epc)
{
    int i;
    int code;
    static void (* excpPrintf)(const char *, ...);

    /* AURORA_EXC_NO_SYSCALL_V1_20260912: FlushCache() is a raw `syscall`
       instruction. We run here with Status.EXL=1, and on the EE a syscall
       taken while EXL=1 does NOT update EPC -- when the kernel's syscall
       path executes `eret` it jumps back to the STALE EPC (the original
       faulting instruction) with pkoDebug's registers (a0=0, sp=_exceptionStack),
       so the CPU re-executes the fault, re-enters this handler, and wanders
       through nested exceptions until a `jr ra` happens to land back inside
       pkoDebug. Every EPC/BadVAddr/register value printed after that is an
       artifact of the wander, not the real crash context (confirmed
       2026-09-12: 测试版18's dump showed EPC=nextLineCycle/BadVAddr=4 with
       a0=0/sp=exception stack -- all fake). Removing the syscalls keeps
       execution linear so the printed dump is the FIRST, REAL fault.
       scr_printf/init_scr write straight to the GS/framebuffer (no syscall);
       SleepThread below never returns, so it is safe. */
#if 0
    FlushCache(0);
    FlushCache(2);
#endif

#if 0
    if (userThreadID) {
        TerminateThread(userThreadID);
        DeleteThread(userThreadID);
    }
#endif

    code = cause & 0x7c;

    init_scr();
    excpPrintf = scr_printf;

    excpPrintf("\n\n             Exception handler: %s exception\n\n",
               codeTxt[code>>2]);

    excpPrintf("      Cause %08x  BadVAddr %08x  Status %08x  EPC %08x\n",
               cause, badvaddr, status, epc);
    excpPrintf("      Last marker: %s\n\n",
               _MainLoop_ModalStr[0] ? _MainLoop_ModalStr : "(none)");

    for(i = 0; i < 32/2; i++) {
        excpPrintf("%4s: %016llX%016llX %4s: %016llX%016llX\n",
                   regName[i],    _savedRegs[i].uint64[1],    _savedRegs[i].uint64[0],
                   regName[i+16], _savedRegs[i+16].uint64[1], _savedRegs[i+16].uint64[0]);
    }
    excpPrintf("\n");
    SleepThread();
}


////////////////////////////////////////////////////////////////////////
// Installs exception handlers for the 'usual' exceptions..
void
installExceptionHandlers(void)
{
    int i;

    // Skip exception #8 (syscall) & 9 (breakpoint)
    for (i = 1; i < 4; i++) {
        SetVTLBRefillHandler(i, pkoExceptionHandler);
    }
    for (i = 4; i < 8; i++) {
        SetVCommonHandler(i, pkoExceptionHandler);
    }
    for (i = 10; i < 14; i++) {
        SetVCommonHandler(i, pkoExceptionHandler);
    }
}

