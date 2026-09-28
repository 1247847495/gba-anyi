#pragma once

#include "types.h"

/* AURORA_CONTROLLER_OPTIONS_V2
 * User-facing Max is the historical one-frame ON / one-frame OFF cadence. */
enum MainLoopTurboSpeedE
{
	MAINLOOP_TURBO_SPEED_NORMAL = 0,
	MAINLOOP_TURBO_SPEED_HALF,
	MAINLOOP_TURBO_SPEED_QUARTER,
	MAINLOOP_TURBO_SPEED_NUM
};

void MainLoopTurboSetSpeed(MainLoopTurboSpeedE eSpeed);
MainLoopTurboSpeedE MainLoopTurboGetSpeed(void);
void MainLoopTurboCycleSpeedDir(Int32 dir);
const char *MainLoopTurboGetSpeedName(void);
void MainLoopTurboAdvanceHostFrame(void);
void MainLoopTurboRearmHostPhase(void);

/* AURORA_MD_PAD_LAYOUT_V1
 * Ordem dos três botões frontais do DualShock: Square / Cross / Circle.
 * ABC = A/B/C. BCA = B/C/A. SMS/GG usam somente os lógicos B/C. */
enum MainLoopMdPadLayoutE
{
	MAINLOOP_MD_PAD_ABC = 0,
	MAINLOOP_MD_PAD_BCA,
	MAINLOOP_MD_PAD_NUM
};

void MainLoopMdPadSetLayout(MainLoopMdPadLayoutE eLayout);
MainLoopMdPadLayoutE MainLoopMdPadGetLayout(void);
void MainLoopMdPadCycleLayoutDir(Int32 dir);
const char *MainLoopMdPadGetLayoutName(void);

/* AURORA_PAD_FACE_REMAP_V1_20260917
 * User-configurable mapping of the four PS2 face buttons (Cross,
 * Circle, Triangle, Square) onto the four canonical face targets
 * (A/B/X/Y, named after the SNES carrier bits every core consumes).
 * Applied once at the top of _MainLoopInput(), so SNES / GBA / NES /
 * MD / SMS / PCE / FDS all follow it uniformly. Menu navigation reads
 * the pad outside _MainLoopInput() and is never remapped.
 *
 * Map byte layout: 2 bits per host button, target index
 *   bits[1:0] = Cross, [3:2] = Circle, [5:4] = Triangle, [7:6] = Square.
 * Default 0xE1 = Cross->B, Circle->A, Triangle->X, Square->Y
 * (the historical layout, i.e. identity remap). */
enum MainLoopPadFaceTargetE
{
	MAINLOOP_PAD_FACE_A = 0,
	MAINLOOP_PAD_FACE_B,
	MAINLOOP_PAD_FACE_X,
	MAINLOOP_PAD_FACE_Y,
	MAINLOOP_PAD_FACE_NUM
};

void MainLoopPadFaceSetMap(Uint32 uMap);
Uint32 MainLoopPadFaceGetMap(void);
void MainLoopPadFaceCycleDir(Int32 iHostButton, Int32 dir);
const char *MainLoopPadFaceGetTargetName(Int32 iHostButton);

Uint16 _MainLoopInput(Uint32 pad);
void _MainLoopInputProcess(Uint32 buttons);
void _MainLoopInputSuppressUntilRelease();
void _MainLoopQuickStateExecuteConfirmed(Bool bSave);
