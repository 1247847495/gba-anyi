/* AURORA_GBA32_BOOSTER_V4 (2026-09-16).
 *
 * 布局: 本翻译单元的所有代码和静态状态都落 .gba32booster_code 段,
 * 链接脚本 (tools/ps2_link_gba32booster.ld) 把它放进自己的 "booster"
 * PT_LOAD, 位于 _end 之后 (crt0 的 16 字节 sq 清零循环够不到这里),
 * __gba32_heap_start 之前 (堆起点被 tools/patch_gba32_heap.py 重写,
 * 从 _end 推到 booster 之后, 所以 malloc 永远不会覆盖本段)。
 *
 * 为什么必须这样: V2/V3 把 booster 放在 [__bss_start,_end) 清零区间
 * 内, 真机每次开机 crt0 都把 booster 清成全零 nop sled -> 所有 GBA
 * 游戏第一帧挂死 (GB/GBC 不走 wrap 所以正常)。静态变量若留在普通
 * .bss 还会平移 SDK/gsKit 符号, 所以这里连状态都放 LOAD2 (PROGBITS,
 * 带显式初值), .bss 与 baseline 逐字节同构, GMBT/GPSP/前端符号零
 * 平移。
 *
 * 逻辑 (ReGBA 自适应跳帧, 仅对超过 gpSP cache 窗口的 32MB 游戏生效):
 *   - <= 窗口 (16MB 及以下): 无条件透传, 行为与基线完全一致。
 *   - >  窗口 (32MB): 每帧 time(NULL) 检测秒跳变, 秒边界结算上一秒
 *     的 emulated/rendered -> Q16 跳帧参数; skip>0 时按 (skip+1) 周期
 *     跳过渲染 (target=NULL), 逻辑帧照跑, 音频 (mix) 照喂。
 */

#define G32B_SEC __attribute__((section(".gba32booster_code")))
#define G32B_DATA __attribute__((section(".gba32booster_data")))

#include <time.h>

/* 前向声明即可 - 参数类型照常 C++ 检查, 函数名走 extern "C"
 * 字面符号 (见 Makefile 的 -Wl,--wrap=mangled 说明)。 */
namespace Emu { struct SysInputT; }
class CRenderSurface;
class CMixBuffer;

extern "C" {
/* gpSP 侧 (gpsp_bridge.cpp): gamepak 大小与 cache 窗口查询。 */
extern unsigned GPSP_gamepak_size;
extern unsigned GPSP_gamepak_cache_window_bytes(void);

void __real__Z18GbaBridge_RunFramePN3Emu9SysInputTEP14CRenderSurfaceP10CMixBuffer(
    Emu::SysInputT *input, CRenderSurface *target, CMixBuffer *mix);
void __real__Z20GbaBridge_UnloadGamev(void);

void __wrap__Z18GbaBridge_RunFramePN3Emu9SysInputTEP14CRenderSurfaceP10CMixBuffer(
    Emu::SysInputT *input, CRenderSurface *target, CMixBuffer *mix);
void __wrap__Z20GbaBridge_UnloadGamev(void);
}

/* ---- 状态 (全部在 booster PT_LOAD 内, PROGBITS 带初值) ---- */
G32B_DATA static int      s_SkipFrameQ = 0;   /* Q16 skip: 0 = 每帧渲染 */
G32B_DATA static unsigned s_SkipCtrQ   = 0;   /* 帧计数 (mod skip+1) */
G32B_DATA static unsigned s_Emulated   = 0;   /* 本秒逻辑帧数 */
G32B_DATA static unsigned s_Rendered   = 0;   /* 本秒渲染帧数 */
G32B_DATA static time_t   s_Sec        = 0;   /* 上次秒跳变时刻 */

G32B_SEC static void gba32BoosterReset(void)
{
    s_SkipFrameQ = 0;
    s_SkipCtrQ   = 0;
    s_Emulated   = 0;
    s_Rendered   = 0;
    s_Sec        = 0;
}

/* ReGBA update_frameskip: E=上秒逻辑帧率, R=上秒渲染帧率。 */
G32B_SEC static void gba32BoosterTick(unsigned emulated, unsigned rendered)
{
    if (emulated < 59) {
        if (s_SkipFrameQ == 0)
            s_SkipFrameQ = 3600 / ((rendered ? rendered : 1) * (60 - emulated));
        else
            s_SkipFrameQ -= (60 - emulated) / 100;
    } else if (emulated > 61) {
        s_SkipFrameQ += (emulated - 60) / 100;
    }
    if (rendered > 58 || s_SkipFrameQ <= 1)
        s_SkipFrameQ = 0;
}

extern "C" G32B_SEC void __wrap__Z18GbaBridge_RunFramePN3Emu9SysInputTEP14CRenderSurfaceP10CMixBuffer(
    Emu::SysInputT *input, CRenderSurface *target, CMixBuffer *mix)
{
    /* <= cache 窗口 (16MB 及以下): 完全原行为, 零改动。 */
    if (GPSP_gamepak_size <= GPSP_gamepak_cache_window_bytes()) {
        __real__Z18GbaBridge_RunFramePN3Emu9SysInputTEP14CRenderSurfaceP10CMixBuffer(
            input, target, mix);
        return;
    }

    /* 32MB: 秒边界结算上一秒的帧率, 更新跳帧参数。 */
    time_t now = time(NULL);
    if (now != s_Sec) {
        if (s_Sec != 0)
            gba32BoosterTick(s_Emulated, s_Rendered);
        s_Sec      = now;
        s_Emulated = 0;
        s_Rendered = 0;
    }

    s_Emulated++;

    unsigned period = (unsigned)s_SkipFrameQ + 1;
    if (s_SkipFrameQ > 0 && (s_SkipCtrQ % period) != 0) {
        /* 跳帧: 逻辑照跑, 不渲染。 */
        __real__Z18GbaBridge_RunFramePN3Emu9SysInputTEP14CRenderSurfaceP10CMixBuffer(
            input, NULL, mix);
    } else {
        s_Rendered++;
        __real__Z18GbaBridge_RunFramePN3Emu9SysInputTEP14CRenderSurfaceP10CMixBuffer(
            input, target, mix);
    }

    s_SkipCtrQ++;
}

extern "C" G32B_SEC void __wrap__Z20GbaBridge_UnloadGamev(void)
{
    gba32BoosterReset();
    __real__Z20GbaBridge_UnloadGamev();
}
