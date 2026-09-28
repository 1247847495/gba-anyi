# -*- coding: utf-8 -*-
"""localize_cn.py - Localize SNESticle Aurora user-visible strings to Chinese.

Applies exact-quoted-literal replacements per source file. Only display
strings are translated; paths, device prefixes, ROM header magic, config
keys and debug printf() text stay untouched.
"""

import io
import sys

sys.stdout.reconfigure(encoding="utf-8")

# Replacement tables: (file, [(old, new), ...])
# Longest-first ordering is applied automatically per file.

T = {}

# ---------------------------------------------------------------- uiVideo --
T["src/platform/ps2/ui/uiVideo.cpp"] = [
    # page headers
    ("Settings Menu (1/6)", "设置菜单 (1/6)"),
    ("Settings Menu (2/6)", "设置菜单 (2/6)"),
    ("Settings Menu (3/6)", "设置菜单 (3/6)"),
    ("Settings Menu (4/6)", "设置菜单 (4/6)"),
    ("Settings Menu (5/6)", "设置菜单 (5/6)"),
    ("Settings Menu (6/6)", "设置菜单 (6/6)"),
    ("Screen", "画面"),
    ("Audio", "音频"),
    ("Storage / Devices", "存储 / 设备"),
    ("Misc.", "杂项"),
    ("SNES Hacks", "SNES兼容设置"),
    ("Performance", "性能"),
    ("Controller options", "手柄选项"),
    # page 1 rows
    ("Video Mode", "视频模式"),
    ("Widescreen", "宽屏"),
    ("Overscan", "过扫描"),
    ("Offset X", "水平偏移"),
    ("Offset Y", "垂直偏移"),
    ("Cover Art", "封面显示"),
    ("SMS VDP border", "SMS彩色边框"),
    ("GG Zoom", "GG缩放"),
    ("Safe Frameskip", "安全跳帧"),
    # page 2 rows
    ("Menu Music", "菜单音乐"),
    ("Menu volume", "菜单音量"),
    ("SNES volume", "SNES音量"),
    ("SEGA volume", "SEGA音量"),
    ("PCE volume", "PCE音量"),
    ("SNES audio", "SNES音频"),
    ("SEGA audio", "SEGA音频"),
    ("32 kHz native", "原生32kHz"),
    ("SMS FM audio", "SMS FM音源"),
    ("CD music", "CD音乐"),
    # page 3 (storage) rows
    ("Mass / USB", "USB存储"),
    ("HDD Support", "硬盘支持"),
    ("MMCE Cards", "MMCE卡"),
    ("SMB (Network)", "SMB网络"),
    ("MX4SIO (SD)", "MX4SIO SD卡"),
    ("Force Region", "强制制式"),
    ("Famiclone Audio", "兼容机音频"),
    ("Reset emulator", "重置模拟器"),
    ("Exit to OSD", "退出到OSD"),
    # page 4 rows
    ("SNES Core", "SNES核心"),
    ("BG1 Layer", "BG1图层"),
    ("BG2 Layer", "BG2图层"),
    ("BG3 Layer", "BG3图层"),
    ("BG4 Layer", "BG4图层"),
    ("Sprites / OBJ", "精灵图层"),
    ("Color Math", "颜色混合"),
    ("Window Effects", "窗口效果"),
    ("Mode 7 Quality", "Mode 7画质"),
    ("Sprite Limiter", "精灵限制"),
    ("Limiter Mode", "限制模式"),
    # page 5 rows
    ("Profile", "配置档"),
    ("GS Cache Sync", "GS缓存同步"),
    ("GIF DMA Wait", "GIF DMA等待"),
    ("Audio RPC Chunk", "音频RPC块"),
    ("Audio Queue", "音频队列"),
    ("MD rendering", "MD渲染"),
    # page 6 rows
    ("Use mouse", "使用鼠标"),
    ("MD pad", "MD手柄"),
    ("MD mapping", "MD键位"),
    ("Turbo Speed", "连发速度"),
    ("Light Gun", "光线枪"),
    # footer hints
    ("Up/Dn: select   L/R: change   X: save", "上下:选择  左右:调整  X:保存"),
    ("O: next   Square: previous", "O:下一页  □:上一页"),
    ("mode applies after reboot", "视频模式重启后生效"),
    ("storage applies after reboot", "存储设置重启后生效"),
    # status value strings
    ("240p/288p (CRT)", "240p/288p (CRT)"),  # keep
    ("480i (default)", "480i（默认）"),
    ("Driver Error", "驱动错误"),
    ("Restart", "需重启"),
    ("Slots 1+2", "插槽1+2"),
    ("Slot 1", "插槽1"),
    ("Slot 2", "插槽2"),
    ("Not Found", "未找到"),
    ("Accurate", "精确"),
    ("Half", "减半"),
    ("Full", "完整"),
    ("Light (28)", "轻(28)"),
    ("Medium (24)", "中(24)"),
    ("Strong (20)", "强(20)"),
    ("Extreme (16)", "极强(16)"),
    ("Heavy (12)", "加重(12)"),
    ("Insane (8)", "疯狂(8)"),
    ("Off (34)", "关(34)"),
    ("Per Scanline", "按扫描线"),
    ("Per Screen", "按画面"),
    ("Standard", "标准"),
    ("Conservative", "保守"),
    ("Custom", "自定义"),
    ("Range", "局部"),
    ("Long", "加长"),
    ("Deep", "加深"),
    ("Normal", "常规"),
    ("Enabled", "已启用"),
    ("Enable", "启用"),
    ("Disable", "停用"),
    ("Fast", "快速"),
    ("Good", "良好"),
    ("6-button", "六键"),
    ("3-button", "三键"),
    ("Auto", "自动"),
    ("ON", "开"),
    ("OFF", "关"),
    ("On", "开"),
    ("Off", "关"),
]

# ------------------------------------------------------------- uiNetwork --
T["src/platform/ps2/ui/uiNetwork.cpp"] = [
    ("SMB Network", "SMB网络"),
    ("Press X to show Internet options", "按 X 显示网络选项"),
    ("Status", "状态"),
    ("Server / Share", "服务器 / 共享"),
    ("Server IP", "服务器IP"),
    ("Port", "端口"),
    ("Share", "共享名"),
    ("Username", "用户名"),
    ("Password", "密码"),
    ("Actions", "操作"),
    ("Save & Connect", "保存并连接"),
    ("Disconnect", "断开连接"),
    ("L/R: octet  Up/Dn: value", "左右:切换组  上下:调数值"),
    ("X/Triangle: done", "X/△: 完成"),
    ("L/R: cursor  Up/Dn: character", "左右:移光标  上下:改字符"),
    ("X: next  Square: delete  Triangle: done", "X:下一个 □:删除 △:完成"),
    ("X: edit/select  Square: reset field", "X:编辑/选择  □:重置"),
    ("Circle: reload saved config", "O:重新载入配置"),
    ("SMB: Saving config...", "SMB: 正在保存配置..."),
    ("SMB: Connected\\n%s", "SMB: 已连接\\n%s"),
    ("SMB: %s (error %d)", "SMB: %s（错误 %d）"),
    ("SMB: Disconnected", "SMB: 已断开"),
]

# -------------------------------------------------------------- uiBrowser --
T["src/platform/ps2/ui/uiBrowser.cpp"] = [
    ("Copy File", "复制文件"),
    ("Paste File", "粘贴文件"),
    ("Delete file", "删除文件"),
    ("File Menu", "文件菜单"),
    ("No Covers", "无封面"),
    ("SMB: Connecting...", "SMB: 正在连接..."),
    ("Copy path too long.", "复制路径过长。"),
    ("Path too long.", "路径过长。"),
    ("SMB: %s\\nCheck config/network", "SMB: %s\\n请检查配置/网络"),
]

# ------------------------------------------------------------------ uiLog --
T["src/platform/ps2/ui/uiLog.cpp"] = [
    ("Message Log", "消息日志"),
]

# ----------------------------------------------------------- mainloop_menu --
T["src/platform/ps2/system/mainloop_menu.cpp"] = [
    ("Browse State Files", "浏览即时存档"),
    ("Browse SRAM Files", "浏览SRAM存档"),
    ("Ask Save Location Again", "重新询问存档位置"),
    ("SRAM Storage: %s", "SRAM存储: %s"),
    ("Not chosen", "未选择"),
    ("Storage: %s", "存储位置: %s"),
    ("Quick Slot: 1 (Auto)", "快速存档位: 1（自动）"),
    ("Quick Slot: %d", "快速存档位: %d"),
    ("Quick target: %s", "快速存档目标: %s"),
    ("X: open  SELECT: file menu", "X: 打开  SELECT: 文件菜单"),
    ("Delete removes both state banks", "删除会同时移除双份存档"),
    ("HDD: choose partition first", "HDD: 请先选择分区"),
    ("Save state?", "保存进度？"),
    ("Load state?", "读取进度？"),
    ("Left/Right: choose   X: confirm", "左右: 选择   X: 确认"),
    ("Default: NO", "默认: 否"),
    ("USB / mass", "USB存储"),
    ("Memory Card", "记忆卡"),
    ("Internal HDD", "内置硬盘"),
    ("Choose quick-save location", "选择快速存档位置"),
    ("X: choose, save, return", "X: 选择并保存返回"),
    ("Circle: cancel", "O: 取消"),
    ("Change later in State Manager", "之后可在存档管理中更改"),
    ("Saving state slot %d...", "正在保存到存档位 %d..."),
    ("YES - Format mc%d:", "是 - 格式化 mc%d:"),
    ("No - Cancel", "否 - 取消"),
    ("mc0: is not formatted", "mc0: 未格式化"),
    ("mc1: is not formatted", "mc1: 未格式化"),
    ("FORMATTING ERASES THE ENTIRE CARD", "格式化将清空整张记忆卡"),
    ("Select YES, then press X", "选择“是”后再按 X"),
    ("Circle: cancel safely", "O: 安全取消"),
    ("Formatting mc%d:...", "正在格式化 mc%d:..."),
    ("Could not format mc%d:.", "无法格式化 mc%d:"),
    ("Format failed - Circle: cancel", "格式化失败 - O: 取消"),
    ("Memory card formatted, but save failed.", "记忆卡已格式化，但保存失败。"),
    ("Memory card formatted.", "记忆卡已格式化。"),
    ("Next L2+X will ask the save location.", "下次按 L2+X 将询问存档位置。"),
    ("CD I/O busy; save deferred.", "CD读取忙，保存已推迟。"),
    ("CD I/O busy; format deferred.", "CD读取忙，格式化已推迟。"),
    ("State browser unavailable.", "存档浏览器不可用。"),
    ("SRAM browser unavailable.", "SRAM浏览器不可用。"),
    ('_MainLoop_StateConfirmNoEntry[]  = "NO";',
     '_MainLoop_StateConfirmNoEntry[]  = "否";'),
    ('_MainLoop_StateConfirmYesEntry[] = "YES";',
     '_MainLoop_StateConfirmYesEntry[] = "是";'),
    ('{ "Internal HDD", "hdd0:" }', '{ "内置硬盘", "hdd0:" }'),
]

# ---------------------------------------------------------- mainloop_state --
T["src/platform/ps2/system/mainloop_state.cpp"] = [
    ("No save-state operation yet.", "尚未进行存档操作。"),
    ("USB -> MC fallback", "USB优先，回退记忆卡"),
    ("USB only", "仅USB"),
    ("Memory Card", "记忆卡"),
    ("Internal HDD", "内置硬盘"),
    ("Auto", "自动"),
    ("No game loaded.", "未加载游戏。"),
    ("This system cannot save states.", "该系统不支持即时存档。"),
    ("NES state unavailable for this cartridge/mapper.", "该卡带/映射器不支持NES存档。"),
    ("FCEUmm FDS state unavailable.", "FCEUmm FDS存档不可用。"),
    ("PicoDrive state unavailable.", "PicoDrive存档不可用。"),
    ("Beetle PCE Fast state unavailable.", "Beetle PCE Fast存档不可用。"),
    ("Snes9x 2010 state unavailable.", "Snes9x 2010存档不可用。"),
    ("SWC state with external cartridge is not serialized.",
     "SWC外接卡带状态暂不支持序列化。"),
    ("Insert a Super Wild Card disk before using save states.",
     "使用即时存档前请先插入Super Wild Card磁盘。"),
    ("Super Wild Card state extension unavailable.", "Super Wild Card状态扩展不可用。"),
    ("No SNES ROM loaded.", "未加载SNES ROM。"),
    ("%s state is not serialized yet.", "%s状态暂不支持序列化。"),
    ("Stop movie recording/playback first.", "请先停止录像录制或回放。"),
    ("Save states are disabled during netplay.", "联机对战期间已停用即时存档。"),
    ("Ready: Super Wild Card + DRAM/FDC state.", "就绪: Super Wild Card + DRAM/FDC状态。"),
    ("Ready: PicoDrive cartridge state.", "就绪: PicoDrive卡带状态。"),
    ("Ready: PC Engine HuCard state.", "就绪: PC Engine HuCard状态。"),
    ("Ready: Famicom Disk System state.", "就绪: Famicom Disk System状态。"),
    ("Ready: Snes9x 2010 SNES state.", "就绪: Snes9x 2010 SNES状态。"),
    ("Ready: NES cartridge and mapper state.", "就绪: NES卡带与映射器状态。"),
    ("Ready: base SNES hardware.", "就绪: SNES基本硬件。"),
    ("Cannot identify the loaded ROM.", "无法识别已加载的ROM。"),
    ("Loaded slot %d from %s.", "已读取存档位 %d（来自%s）。"),
    ("Slot %d is incomplete or corrupt.", "存档位 %d 不完整或已损坏。"),
    ("Slot %d belongs to another ROM.", "存档位 %d 属于其他ROM。"),
    ("No state found in slot %d.", "存档位 %d 中没有存档。"),
    ("Could not allocate core state buffer (%u bytes).",
     "无法分配核心状态缓冲区（%u字节）。"),
    ("Could not snapshot the NES core state.", "无法生成NES核心状态快照。"),
    ("Could not snapshot the FCEUmm FDS state.", "无法生成FCEUmm FDS状态快照。"),
    ("Could not snapshot the PicoDrive state.", "无法生成PicoDrive状态快照。"),
    ("Could not snapshot the Beetle PCE Fast state.", "无法生成Beetle PCE Fast状态快照。"),
    ("Could not snapshot the Snes9x 2010 state.", "无法生成Snes9x 2010状态快照。"),
    ("Could not snapshot the Super Wild Card state.", "无法生成Super Wild Card状态快照。"),
    ("Saved slot %d to %s.", "已保存到存档位 %d（%s）。"),
    ("mc%d: is not formatted.", "mc%d: 未格式化。"),
    ("Could not save slot %d to %s.", "无法保存到存档位 %d（%s）。"),
]

# ------------------------------------------------------------- mainloop_smb --
T["src/platform/ps2/system/mainloop_smb.cpp"] = [
    ("Connecting", "连接中"),
    ("Connected", "已连接"),
    ("No SMB.CNF", "缺少SMB.CNF"),
    ("Bad SMB.CNF", "SMB.CNF无效"),
    ("Save Error", "保存错误"),
    ("Network Error", "网络错误"),
    ("Net Init Error", "网络初始化错误"),
    ("DHCP Timeout", "DHCP超时"),
    ("Driver Error", "驱动错误"),
    ("Connect Error", "连接错误"),
    ("SMB1 Required", "需要SMB1"),
    ("Auth Error", "认证错误"),
    ("Share Error", "共享错误"),
    ("Browse Error", "浏览错误"),
    ("Enabled", "已启用"),
    ("Off", "关"),
]

# ------------------------------------------------------------ mainloop_load --
T["src/platform/ps2/system/mainloop_load.cpp"] = [
    ("ERROR: loading rom %d\\n", "错误: 加载ROM %d\\n"),
    ("ERROR: loading fds bios!\\n", "错误: 加载FDS BIOS失败!\\n"),
    ("ERROR: cannot open NES palette", "错误: 无法打开NES调色板"),
    ("ERROR: NES .pal must be exactly 192 bytes", "错误: NES .pal必须正好192字节"),
    ("ERROR: could not apply NES palette", "错误: 无法应用NES调色板"),
    ("NES palette loaded.", "NES调色板已加载。"),
    ("Copier/cart SRAM saved.", "拷贝机/卡带SRAM已保存。"),
    ("WARNING: copier/cart SRAM save failed!", "警告: 拷贝机/卡带SRAM保存失败!"),
    ("ERROR: cannot create SNESticle/SYSTEM", "错误: 无法创建SNESticle/SYSTEM"),
    ("ERROR: CUE/TOC or its first track is unreadable", "错误: CUE/TOC或其第一轨无法读取"),
    ("ERROR: cannot configure CD video raster", "错误: 无法配置CD视频光栅"),
    ("ERROR: Sega CD failed; put its region BIOS .bin in SYSTEM",
     "错误: SEGA CD启动失败; 请将对应区域BIOS .bin放入SYSTEM"),
    ("ERROR: PCE CD failed; put syscard3.pce in SYSTEM",
     "错误: PCE CD启动失败; 请将syscard3.pce放入SYSTEM"),
    ("ERROR: cannot configure FDS video raster", "错误: 无法配置FDS视频光栅"),
    ("ERROR: FDS BIOS path is too long", "错误: FDS BIOS路径过长"),
    ("ERROR: put disksys.rom in SNESticle/SYSTEM", "错误: 请将disksys.rom放入SNESticle/SYSTEM"),
    ("ERROR: cannot read SYSTEM/disksys.rom", "错误: 无法读取SYSTEM/disksys.rom"),
    ("ERROR: disksys.rom must be exactly 8192 bytes", "错误: disksys.rom必须正好8192字节"),
    ("ERROR: FCEUmm could not boot FDS; check SYSTEM/disksys.rom",
     "错误: FCEUmm无法启动FDS; 请检查SYSTEM/disksys.rom"),
    ("ERROR: not enough memory for FDS ZIP", "错误: 内存不足，无法解压FDS"),
    ("ERROR: cannot extract complete FDS from ZIP", "错误: 无法从ZIP完整解出FDS"),
    ("ERROR: SYSTEM must be writable for FDS ZIP", "错误: FDS ZIP需要SYSTEM目录可写"),
    ("ERROR: cannot write temporary FDS image", "错误: 无法写入临时FDS镜像"),
    ("ERROR: FCEUmm could not boot FDS from ZIP", "错误: FCEUmm无法从ZIP启动FDS"),
    ("SWC boot failed: %s", "SWC启动失败: %s"),
    ("Super Wild Card: no disk inserted", "Super Wild Card: 未插入磁盘"),
    ("Magicom boot failed: %s", "Magicom启动失败: %s"),
    ("Super Magicom: no disk inserted", "Super Magicom: 未插入磁盘"),
    ("Inserting cartridge...", "正在插入卡带..."),
    ("SWC: cannot open cartridge", "SWC: 无法打开卡带"),
    ("SWC: invalid SNES cartridge", "SWC: 无效的SNES卡带"),
    ("SWC: unsupported mapper/coprocessor cartridge", "SWC: 不支持的映射器/协处理器卡带"),
    ("SWC: cartridge insertion failed", "SWC: 卡带插入失败"),
    ("Creating disk 1...", "正在创建磁盘1..."),
    ("Copier: cartridge inserted; could not create _1 disk",
     "拷贝机: 卡带已插入; 无法创建_1磁盘"),
    ("Inserting disk 1...", "正在插入磁盘1..."),
    ("Copier: cartridge inserted; _1 disk insert failed",
     "拷贝机: 卡带已插入; _1磁盘插入失败"),
    ("Copier: cartridge + %s inserted", "拷贝机: 已插入卡带 + %s"),
    ("SWC cartridge inserted: %s", "SWC卡带已插入: %s"),
    ("SWC: too many numbered disks", "SWC: 编号磁盘过多"),
    ("Creating disk %u...", "正在创建磁盘 %u..."),
    ("SWC: could not create disk image", "SWC: 无法创建磁盘镜像"),
    ("Copier: created disk %u: %s", "拷贝机: 已创建磁盘 %u: %s"),
    ("SWC: unsupported D88 floppy image", "SWC: 不支持的D88软盘镜像"),
    ("Inserting disk %u...", "正在插入磁盘 %u..."),
    ("Inserting disk: %s...", "正在插入磁盘: %s..."),
    ("SWC disk insert failed: %s", "SWC磁盘插入失败: %s"),
    ("READ-ONLY", "只读"),
    ("Classic copier BIOS missing in SYSTEM", "SYSTEM中缺少经典拷贝机BIOS"),
    ("ERROR: cannot configure SWC SNES raster", "错误: 无法配置SWC SNES光栅"),
    ("SWC: no existing disk to insert", "SWC: 没有可插入的磁盘"),
    ("SWC: disk %u inserted", "SWC: 磁盘 %u 已插入"),
    ("SWC: floppy inserted: %s", "SWC: 软盘已插入: %s"),
    ("SWC: current disk is not .d88", "SWC: 当前磁盘不是.d88格式"),
    ("SWC: disk name must end in _1.d88, _2.d88, ...",
     "SWC: 磁盘名必须以 _1.d88、_2.d88 等结尾"),
    ("Copier: no numbered floppy image found", "拷贝机: 未找到编号软盘镜像"),
    ("SWC disk swap failed: %s", "SWC磁盘切换失败: %s"),
    ("Requires classic Super Magicom V1H/V31 BIOS (V3H soft-upgrade unsupported)",
     "需要经典Super Magicom V1H/V31 BIOS（不支持V3H软升级）"),
    ("Requires classic 16 KiB Super Wild Card BIOS", "需要经典16 KiB Super Wild Card BIOS"),
    ("Unrecognized classic copier .rom", "无法识别的经典拷贝机.rom"),
    ("Copier: unsupported or invalid D88 floppy image", "拷贝机: 不支持或无效的D88软盘镜像"),
    ("SWC BIOS missing/invalid in SNESticle/SYSTEM", "SWC BIOS缺失或无效 (SNESticle/SYSTEM)"),
    ("CD I/O busy; try again.", "CD读取忙，请重试。"),
    ("FDS in GZ is not supported; use plain .fds or ZIP", "不支持GZ格式的FDS; 请使用.fds或ZIP"),
    ("ERROR: ROM is too large or unreadable", "错误: ROM过大或无法读取"),
    ("ERROR: not enough memory for ROM probe", "错误: 内存不足，无法探测ROM"),
    ("ERROR: cannot probe Sega ROM", "错误: 无法探测SEGA ROM"),
    ("ERROR: cannot configure video raster", "错误: 无法配置视频光栅"),
    ("ERROR: not enough memory for ROM", "错误: 内存不足，无法加载ROM"),
    ("ERROR: Cannot read complete ROM", "错误: 无法读取完整ROM"),
    ("ERROR: Cannot load disksys.rom", "错误: 无法加载disksys.rom"),
    ("ERROR: QuickNES cannot run this NES image (format/mapper/FDS)",
     "错误: QuickNES无法运行此NES镜像（格式/映射器/FDS）"),
    ("ERROR: PicoDrive cannot run this SEGA image", "错误: PicoDrive无法运行此SEGA镜像"),
    ("ERROR: Beetle PCE Fast cannot run this HuCard", "错误: Beetle PCE Fast无法运行此HuCard"),
    ("ERROR: Snes9x 2010 cannot run this SNES image", "错误: Snes9x 2010无法运行此SNES镜像"),
    ("WARNING: Unsupported NES Mapper", "警告: 不支持的NES映射器"),
]

# ---------------------------------------------------------- mainloop_browser --
T["src/platform/ps2/system/mainloop_browser.cpp"] = [
    ("Sega CD I/O busy; try again.", "SEGA CD读取忙，请重试。"),
    ("ERROR: %s\\n", "错误: %s\\n"),
]

# ---------------------------------------------------------- mainloop_install --
T["src/platform/ps2/system/mainloop_install.cpp"] = [
    ("Unable to open SYSTEM.CNF on cd.", "无法读取光盘上的SYSTEM.CNF。"),
    ("%s added to mc0:title.db", "%s 已添加到 mc0:title.db"),
    ("Unable to add to %s", "无法添加到 %s"),
    ("Unable to find PSX ELF", "未找到PSX ELF"),
]

# ---------------------------------------------------------- mainloop_process --
T["src/platform/ps2/system/mainloop_process.cpp"] = [
    ("FDS: Disk %d Side %c inserted", "FDS: 磁盘 %d 面 %c 已插入"),
]

# ---------------------------------------------------- mainloop_menu_runtime --
T["src/platform/ps2/system/mainloop_menu_runtime.cpp"] = [
    ("SRAM saved.", "SRAM已保存。"),
    ("Error saving SRAM!", "SRAM保存失败!"),
    ("CD I/O busy; menu deferred.", "CD读取忙，菜单已推迟。"),
    ("Saving SRAM...", "正在保存SRAM..."),
]

# ------------------------------------------------------------------ input --
T["src/platform/ps2/input/input.cpp"] = [
    ("Controller (no P1)", "手柄（未接P1）"),
    ("Controller", "手柄"),
    ("USB (driver off)", "USB（驱动未开）"),
    ("USB (no mouse)", "USB（无鼠标）"),
    ("Off", "关"),
]

# ---------------------------------------------------------- mainloop_input --
T["src/platform/ps2/system/mainloop_input.cpp"] = [
    ("Half", "半速"),
    ("Quarter", "四分之一速"),
    ("Max", "最快"),
]

# ------------------------------------------------------------- mainloop_ui --
T["src/platform/ps2/system/mainloop_ui.cpp"] = [
    ("[IOP] %-11s imported OK", "[IOP] %-11s 导入成功"),
    ("[IOP] %-11s imported BAD (err=%d)", "[IOP] %-11s 导入失败 (err=%d)"),
    ("[IOP] imported: OK", "[IOP] 全部模块导入成功"),
    ("[IOP] imported: BAD", "[IOP] 模块导入存在失败"),
]


def apply(path, pairs):
    with io.open(path, "r", encoding="utf-8", newline="") as f:
        src = f.read()

    # longest-first so longer literals win over shorter prefixes
    pairs = sorted(pairs, key=lambda p: len(p[0]), reverse=True)

    total = 0
    missed = []
    for old, new in pairs:
        if old == new:
            continue
        needle = '"%s"' % old
        repl = '"%s"' % new
        n = src.count(needle)
        if n == 0:
            # allow raw (non-quoted) forms for special declarations
            if old.startswith("_MainLoop") or old.startswith("{ "):
                n = src.count(old)
                if n:
                    src = src.replace(old, new)
                    total += n
                    continue
            missed.append(old)
            continue
        src = src.replace(needle, repl)
        total += n

    with io.open(path, "w", encoding="utf-8", newline="") as f:
        f.write(src)

    print("%-55s %3d replacements" % (path, total))
    if missed:
        for m in missed:
            print("    MISS: %s" % m)
    return total


def main():
    grand = 0
    for path, pairs in T.items():
        grand += apply(path, pairs)
    print("TOTAL: %d replacements" % grand)


if __name__ == "__main__":
    main()
