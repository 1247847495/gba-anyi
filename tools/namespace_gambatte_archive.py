#!/usr/bin/env python3
# AURORA_GAMBATTE_V1
#
# Namespace Gambatte for embedding beside other libretro cores.
#
# Gambatte's libretro-common objects define the same globals as the
# copies embedded in Beetle PCE Fast / FCEUmm / Snes9x2010 / gpSP.
# Rename every global in the raw archive with a GMBT_ prefix via
# objcopy --redefine-syms, keeping only the frontend ABI (GB_retro_*)
# visible through the bridge.

from __future__ import annotations

import argparse
from pathlib import Path
import re
import shutil
import subprocess

FORCED = {
    "retro_init", "retro_deinit", "retro_api_version",
    "retro_get_system_info", "retro_get_system_av_info",
    "retro_set_environment", "retro_set_video_refresh",
    "retro_set_audio_sample", "retro_set_audio_sample_batch",
    "retro_set_input_poll", "retro_set_input_state",
    "retro_set_controller_port_device", "retro_reset", "retro_run",
    "retro_serialize_size", "retro_serialize", "retro_unserialize",
    "retro_cheat_reset", "retro_cheat_set", "retro_load_game",
    "retro_load_game_special", "retro_unload_game", "retro_get_region",
    "retro_get_memory_data", "retro_get_memory_size",
    "retro_base_directory",
}

CORE_OPTION_RE = re.compile(
    r"\b(?:option_cats|option_defs|options)_[A-Za-z0-9_]+\b"
)

def output(cmd):
    return subprocess.check_output(cmd, text=True, errors="replace")

def defined_symbols(nm, archive):
    text = output([nm, "-g", "--defined-only", str(archive)])
    out = set()
    for line in text.splitlines():
        line = line.strip()
        if not line or line.endswith(":"):
            continue
        parts = line.split()
        if len(parts) >= 2:
            sym = parts[-1]
            if sym and not sym.endswith(":"):
                out.add(sym)
    return out

def discover_core_option_symbols(core_dir):
    symbols = set()
    for path in sorted(core_dir.glob("libretro_core_options*.h")):
        try:
            text = path.read_text(encoding="utf-8-sig", errors="replace")
        except OSError:
            continue
        symbols.update(CORE_OPTION_RE.findall(text))
    return symbols

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--nm", required=True)
    ap.add_argument("--objcopy", required=True)
    ap.add_argument("--ranlib", required=True)
    ap.add_argument("--raw", required=True, type=Path)
    ap.add_argument("--output", required=True, type=Path)
    args = ap.parse_args()

    if not args.raw.is_file():
        raise SystemExit(f"raw archive missing: {args.raw}")

    gambatte = defined_symbols(args.nm, args.raw)
    option_symbols = discover_core_option_symbols(
        args.raw.parent / "libgambatte" / "libretro")

    rename = gambatte | FORCED | option_symbols
    rename = {
        s for s in rename
        if s
        and not s.startswith("GMBT_")
        and s not in {"_gp", "__gnu_local_gp"}
        and not s.startswith("__gnu_lto_")
    }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(args.raw, args.output)
    map_path = args.output.with_suffix(args.output.suffix + ".symbols.map")

    with map_path.open("w", encoding="utf-8") as f:
        for s in sorted(rename):
            f.write(f"{s} GMBT_{s}\n")

    if rename:
        subprocess.check_call([
            args.objcopy,
            f"--redefine-syms={map_path}",
            str(args.output),
        ])

    subprocess.check_call([args.ranlib, str(args.output)])

    final_syms = defined_symbols(args.nm, args.output)
    required = {
        "GMBT_retro_init",
        "GMBT_retro_run",
        "GMBT_retro_load_game",
        "GMBT_retro_set_input_state",
    }
    missing = sorted(required - final_syms)
    if missing:
        raise SystemExit(
            "namespace failed; missing required GMBT exports: "
            + ", ".join(missing)
        )

    print(f"[Gambatte namespace] nm globals: {len(gambatte)}")
    print(f"[Gambatte namespace] total mapping entries: {len(rename)}")
    print(f"[Gambatte namespace] output: {args.output}")
    print(f"[Gambatte namespace] map:    {map_path}")

if __name__ == "__main__":
    main()
