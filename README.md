# GBA模拟器anyi汉化版（gba-anyi）

**PlayStation 2 实机专用的 Game Boy 系列模拟器 —— 支持 GB / GBC / GBA。**

| 支持平台 | 模拟核心 |
| --- | --- |
| Game Boy（GB） | Gambatte |
| Game Boy Color（GBC） | Gambatte |
| Game Boy Advance（GBA） | gpSP |

anyi-gba（anyi 汉化版）基于 **SNESticle Aurora** 深度定制汉化而来：全界面简体中文，并针对 PS2 真机硬件做了大量专有性能优化。

当前版本：**anyi-gba v1.02 测试版**

## 功能特性

### 模拟核心

- **GBA —— gpSP 核心**
  - ≤16MB ROM：JIT 动态翻译执行，保持满速运行
  - \>16MB ROM（32MB 超大游戏）：启动时自动切换解释器模式，确保稳定
  - 32MB 游戏专属加速：ReGBA 式纯轮换（round-robin）页面置换 + IOP 协处理器异步预取（64KB 块，顺序流检测自动加大预读量，不阻塞模拟主线程）
  - GBA ROM 启动时只预读前 4MB，其余数据按需换页载入，缩短进入游戏时间
- **GB / GBC —— Gambatte 核心**

### 存档系统

- 电池存档（SRAM，`.srm`）自动保存
- 即时存档 / 读档：支持快捷键（L2 + ×）与菜单操作两种方式，即时存档文件使用 `a` 后缀（如 `游戏名.a1a`）
- ROM CRC 校验，防止跨游戏误读存档

### ROM 支持

- 直接运行 `.gba` / `.gb` / `.gbc`，以及 ZIP / GZ 压缩包（流式解压，无需事先解包）：
  - ≤16MB 压缩游戏：32KB 块流式解压直接载入 ROM 缓存，不产生临时文件
  - \>16MB 压缩游戏：64KB 前缀解压 + 与压缩包同设备的缓存文件，退出游戏时自动清理

### 存储与网络

- 支持记忆卡（MC）、USB、内置 HDD、MX4SIO 与 SMB 网络共享
- SMB 连接可在软件内直接配置 PS2 本机 IP、子网掩码、网关（参考 `SMB.CNF.example`）

### 界面（全简体中文）

- 菜单与系统消息全部简体中文
- ROM 文件名 GBK 编码自动转换为 UTF-8 显示（不改动文件本体，读写不受影响）
- 设置菜单分页管理：**画面 / 音频 / 性能 / 手柄按键 / 存储**
  - 240p CRT 视频输出与画面调整选项
  - **手柄按键自定义**：叉 / 圈 / 三角 / 方块可自由映射为 GBA 的 A / B / X / Y（配置保存至记忆卡，仅对游戏内生效）

## 从源码构建

本仓库已完整内置全部源码（含各模拟核心，无需下载任何子模块）。构建需要 [PS2Dev](https://github.com/ps2dev) 工具链（ps2sdk）：

```bash
git clone https://github.com/1247847495/gba-anyi.git
cd gba-anyi
make                # 首次构建可使用: make AUTO_INSTALL=yes 自动安装工具链
```

构建产物位于 `build/` 目录（`anyi-gba*.elf`）。将 ELF 复制到记忆卡、U 盘等设备，通过 wLaunchELF 等启动器即可运行。

## 版权与致谢

- 本项目基于 **SNESticle Aurora**（Vinícius Nunes，@itsveenee）定制而来；其血统追溯自 **SNESticle Revive**（@ReyFxck）与原版 **SNEsticle**（Icer Addis）。见 [CREDITS.md](CREDITS.md)。
- **GBA 核心 gpSP**：原作者 Exophase 及 libretro 移植贡献者，GNU GPLv2（`src/third_party/gpsp/COPYING`）。
- **GB/GBC 核心 Gambatte**：sinamas 及贡献者，GNU GPLv2（`src/third_party/gambatte/COPYING`）。
- 32MB 游戏的页面置换策略参考了 **ReGBA** 项目的实现思路，特此致谢。
- 本项目 GPL 覆盖的代码遵循 **GNU GPLv2**（[LICENSE](LICENSE)）；第三方组件遵循各自许可证（[THIRD_PARTY.md](THIRD_PARTY.md)、[LICENSES/](LICENSES) 目录）。
- **代码许可证 ≠ 品牌授权**：SNEsticle Aurora / Aurora 的名称与品牌由 @itsveenee 保留。本项目以独立的 **anyi** 名称发布，仅为事实性出处标注（"based on SNESticle Aurora"）。见 [BRANDING.md](BRANDING.md)。
- 说明：本项目大量使用 AI 辅助生成代码，所有改动均已在 PS2 真机上测试验证。
