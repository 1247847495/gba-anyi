/* AURORA_EE_EXCEPTION_SCREEN_V1_20260906
 *
 * Missing assembly trampoline for excepHandler.c (PS2Link-derived code that
 * existed in the tree but was never compiled or installed). When the EE takes
 * a level-1 exception (TLB miss, address error, bus error, reserved
 * instruction, overflow...) the stock kernel default simply halts with the
 * framebuffer frozen on the last drawn frame. PCSX2 tolerates several of these
 * faults, which is exactly why some freezes only reproduce on real hardware.
 *
 * This stub saves all GPRs into _savedRegs, reads the COP0 cause/badvaddr/
 * status/epc, switches to the reserved _exceptionStack, installs _gp and
 * tail-calls pkoDebug(), which paints the register dump to the screen. */

    .set noreorder
    .set noat

    .globl pkoExceptionHandler
    .ent  pkoExceptionHandler
pkoExceptionHandler:
    /* $26 (k0) = &_savedRegs */
    lui     $26, %hi(_savedRegs)
    daddiu  $26, $26, %lo(_savedRegs)

    /* Save every GPR we can. $26 is the only clobbered one (kernel scratch). */
    sq      $0,  0x000($26)
    sq      $1,  0x010($26)
    sq      $2,  0x020($26)
    sq      $3,  0x030($26)
    sq      $4,  0x040($26)
    sq      $5,  0x050($26)
    sq      $6,  0x060($26)
    sq      $7,  0x070($26)
    sq      $8,  0x080($26)
    sq      $9,  0x090($26)
    sq      $10, 0x0A0($26)
    sq      $11, 0x0B0($26)
    sq      $12, 0x0C0($26)
    sq      $13, 0x0D0($26)
    sq      $14, 0x0E0($26)
    sq      $15, 0x0F0($26)
    sq      $16, 0x100($26)
    sq      $17, 0x110($26)
    sq      $18, 0x120($26)
    sq      $19, 0x130($26)
    sq      $20, 0x140($26)
    sq      $21, 0x150($26)
    sq      $22, 0x160($26)
    sq      $23, 0x170($26)
    sq      $24, 0x180($26)
    sq      $25, 0x190($26)
    sq      $27, 0x1B0($26)
    sq      $28, 0x1C0($26)
    sq      $29, 0x1D0($26)
    sq      $30, 0x1E0($26)
    sq      $31, 0x1F0($26)

    /* COP0: a0=cause a1=badvaddr a2=status a3=epc */
    mfc0    $4, $13
    mfc0    $5, $8
    mfc0    $6, $12
    mfc0    $7, $14

    /* Switch to the dedicated exception stack (8 KiB, grows down). */
    lui     $26, %hi(_exceptionStack)
    daddiu  $26, $26, %lo(_exceptionStack)
    daddiu  $29, $26, 8192 - 32

    /* pkoDebug is C code that reaches globals through $gp. */
    lui     $28, %hi(_gp)
    daddiu  $28, $28, %lo(_gp)

    jal     pkoDebug
    nop

1:  b       1b
    nop

    .end    pkoExceptionHandler
