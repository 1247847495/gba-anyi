/* Script for -z combreloc */
/* Copyright (C) 2014-2025 Free Software Foundation, Inc.
   Copying and distribution of this script, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved.  */
OUTPUT_FORMAT("elf32-nlittlemips", "elf32-nbigmips", "elf32-nlittlemips")
OUTPUT_ARCH(mips:5900)
ENTRY(__start)
SEARCH_DIR("=/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/lib32"); SEARCH_DIR("=/usr/local/lib32"); SEARCH_DIR("=/lib32"); SEARCH_DIR("=/usr/lib32"); SEARCH_DIR("=/usr/local/ps2dev/ee/mips64r5900el-ps2-elf/lib"); SEARCH_DIR("=/usr/local/lib"); SEARCH_DIR("=/lib"); SEARCH_DIR("=/usr/lib");
/* AURORA_GBA32_BOOSTER_V3: explicit PHDRS.
   V2 placed the PROGBITS .gba32booster after the NOBITS .bss inside the
   single PT_LOAD, which materialized the whole .bss into the file
   (FileSiz grew 0x3ee9b0 -> 0xb538a0, ~4.1MB -> ~11.9MB stripped) and
   put the booster code at the very tail of the image. Real-hardware
   testing froze when entering a GBA game (GB/GBC fine - they never call
   the wrap), consistent with a loader failing on the oversized/tail
   region. V3 keeps segment "main" byte-identical to the baseline layout
   (FileSiz 0x3ee9b0, .bss stays NOBITS) and moves the tiny booster
   section into its own second PT_LOAD so the file size stays ~4.2MB.
   Symbols keep their zero-displacement V2 addresses (GMBT/GPSP/front-
   end untouched); only the ELF program-header structure changes. */
/* AURORA_GBA32_BOOSTER_V5: SINGLE PT_LOAD (baseline-identical layout).
   V4 kept the booster in its own second PT_LOAD at 0xc53580 and passed
   every static check (zero symbol displacement, LOAD1 byte-identical,
   clear-loop untouched, heap-start patched) yet real hardware still
   froze on EVERY GBA game entry. The baseline image has exactly ONE
   PT_LOAD - the real-machine loader path (wLaunchELF/ps2-packer style)
   very likely never maps a second PT_LOAD, so the booster bytes never
   reached 0xc53580 and ExecuteFrame jumped into garbage. V5 merges the
   booster back into the single "main" PT_LOAD (file grows to ~11.9MB
   because .bss after .data must be materialized - V2 already proved
   that size alone is harmless; its hang was the crt0 clear loop) while
   keeping every V4 fix:
     - _end stays at the baseline address (crt0 clear loop untouched,
       stops right before the 16-byte-aligned booster),
     - booster code+state live at [_end+pad, ...) outside the clear
       range but INSIDE the one and only PT_LOAD,
     - tools/patch_gba32_heap.py still rewrites _main's heap base to
       __gba32_heap_start (after the booster), so malloc never lands on
       the booster. Zero symbol displacement preserved. */
PHDRS
{
  abiflags 0x70000003 FLAGS(4); /* PT_MIPS_ABIFLAGS */
  main     PT_LOAD;
}
SECTIONS
{
  /* Read-only sections, merged into text segment: */
  PROVIDE (__executable_start = SEGMENT_START("text-segment", 0x0100000));
  . = SEGMENT_START("text-segment", 0x0100000);
  /* Place the build-id as close to the ELF headers as possible.  This
     maximises the chance the build-id will be present in core files,
     which GDB can then use to locate the associated debuginfo file.  */
  .note.gnu.build-id  : { *(.note.gnu.build-id) } :main
  .interp         : { *(.interp) } :main
  .MIPS.abiflags        : { *(.MIPS.abiflags) } :main :abiflags
  .MIPS.xhash        : { *(.MIPS.xhash) } :main
  .reginfo        : { *(.reginfo) } :main
  .dynamic        : { *(.dynamic) } :main
  .hash           : { *(.hash) } :main
  .gnu.hash       : { *(.gnu.hash) } :main
  .dynsym         : { *(.dynsym) } :main
  .dynstr         : { *(.dynstr) } :main
  .gnu.version    : { *(.gnu.version) } :main
  .gnu.version_d  : { *(.gnu.version_d) } :main
  .gnu.version_r  : { *(.gnu.version_r) } :main
  .rel.dyn        :
    {
      *(.rel.init)
      *(.rel.text .rel.text.* .rel.gnu.linkonce.t.*)
      *(.rel.fini)
      *(.rel.rodata .rel.rodata.* .rel.gnu.linkonce.r.*)
      *(.rel.data.rel.ro .rel.data.rel.ro.* .rel.gnu.linkonce.d.rel.ro.*)
      *(.rel.data .rel.data.* .rel.gnu.linkonce.d.*)
      *(.rel.tdata .rel.tdata.* .rel.gnu.linkonce.td.*)
      *(.rel.tbss .rel.tbss.* .rel.gnu.linkonce.tb.*)
      *(.rel.ctors)
      *(.rel.dtors)
      *(.rel.got)
      *(.rel.dyn)
      *(.rel.sdata .rel.sdata.* .rel.gnu.linkonce.s.*)
      *(.rel.sbss .rel.sbss.* .rel.gnu.linkonce.sb.*)
      *(.rel.sdata2 .rel.sdata2.* .rel.gnu.linkonce.s2.*)
      *(.rel.sbss2 .rel.sbss2.* .rel.gnu.linkonce.sb2.*)
      *(.rel.bss .rel.bss.* .rel.gnu.linkonce.b.*)
      PROVIDE_HIDDEN (__rel_iplt_start = .);
      *(.rel.iplt)
      PROVIDE_HIDDEN (__rel_iplt_end = .);
    } :main
  .rela.dyn       :
    {
      *(.rela.init)
      *(.rela.text .rela.text.* .rela.gnu.linkonce.t.*)
      *(.rela.fini)
      *(.rela.rodata .rela.rodata.* .rela.gnu.linkonce.r.*)
      *(.rela.data .rela.data.* .rela.gnu.linkonce.d.*)
      *(.rela.tdata .rela.tdata.* .rela.gnu.linkonce.td.*)
      *(.rela.tbss .rela.tbss.* .rela.gnu.linkonce.tb.*)
      *(.rela.ctors)
      *(.rela.dtors)
      *(.rela.got)
      *(.rela.sdata .rela.sdata.* .rela.gnu.linkonce.s.*)
      *(.rela.sbss .rela.sbss.* .rela.gnu.linkonce.sb.*)
      *(.rela.sdata2 .rela.sdata2.* .rela.gnu.linkonce.s2.*)
      *(.rela.sbss2 .rela.sbss2.* .rela.gnu.linkonce.sb2.*)
      *(.rela.bss .rela.bss.* .rela.gnu.linkonce.b.*)
      PROVIDE_HIDDEN (__rela_iplt_start = .);
      *(.rela.iplt)
      PROVIDE_HIDDEN (__rela_iplt_end = .);
    } :main
  .rel.plt        :
    {
      *(.rel.plt)
    } :main
  .rela.plt       :
    {
      *(.rela.plt)
    } :main
  /* Start of the executable code region.  */
  .init           :
  {
    KEEP (*(SORT_NONE(.init)))
  } :main
  .plt            : { *(.plt) } :main
  .iplt           : { *(.iplt) } :main
  .text           :
  {
    _ftext = .;
    *(.text.unlikely .text.*_unlikely .text.unlikely.*)
    *(.text.exit .text.exit.*)
    *(.text.startup .text.startup.*)
    *(.text.hot .text.hot.*)
    *(SORT(.text.sorted.*))
    *(.text .stub .text.* .gnu.linkonce.t.*)
    /* .gnu.warning sections are handled specially by elf.em.  */
    *(.gnu.warning)
    *(.mips16.fn.*) *(.mips16.call.*)
  } :main
  .fini           :
  {
    KEEP (*(SORT_NONE(.fini)))
  } :main
  PROVIDE (__etext = .);
  PROVIDE (_etext = .);
  PROVIDE (etext = .);
  /* Start of the Read Only Data region.  */
  .rodata         : { *(.rodata .rodata.* .gnu.linkonce.r.*) } :main
  .rodata1        : { *(.rodata1) } :main
  .sdata2         :
  {
    *(.sdata2 .sdata2.* .gnu.linkonce.s2.*)
  } :main
  .sbss2          : { *(.sbss2 .sbss2.* .gnu.linkonce.sb2.*) } :main
  .eh_frame_hdr   : { *(.eh_frame_hdr) *(.eh_frame_entry .eh_frame_entry.*) } :main
  .eh_frame       : ONLY_IF_RO { KEEP (*(.eh_frame)) *(.eh_frame.*) } :main
  .sframe         : ONLY_IF_RO { *(.sframe) *(.sframe.*) } :main
  .gcc_except_table   : ONLY_IF_RO { *(.gcc_except_table .gcc_except_table.*) } :main
  .gnu_extab   : ONLY_IF_RO { *(.gnu_extab*) } :main
  /* These sections are generated by the Sun/Oracle C++ compiler.  */
  .exception_ranges   : ONLY_IF_RO { *(.exception_ranges*) } :main
  /* Various note sections.  Placed here so that they are always included
     in the read-only segment and not treated as orphan sections.  The
     current orphan handling algorithm does place note sections after R/O
     data, but this is not guaranteed to always be the case.  */
  .note.build-id :      { *(.note.build-id) } :main
  .note.GNU-stack :     { *(.note.GNU-stack) } :main
  .note.gnu-property :  { *(.note.gnu-property) } :main
  .note.ABI-tag :       { *(.note.ABI-tag) } :main
  .note.package :       { *(.note.package) } :main
  .note.dlopen :        { *(.note.dlopen) } :main
  .note.netbsd.ident :  { *(.note.netbsd.ident) } :main
  .note.openbsd.ident : { *(.note.openbsd.ident) } :main
  /* Start of the Read Write Data region.  */
  /* Adjust the address for the data segment.  We want to adjust up to
     the same address within the page on the next page up.  */
  . = ALIGN(128) + (. & (128 - 1));
  /* Exception handling.  */
  .eh_frame       : ONLY_IF_RW { KEEP (*(.eh_frame)) *(.eh_frame.*) } :main
  .sframe         : ONLY_IF_RW { *(.sframe) *(.sframe.*) } :main
  .gnu_extab      : ONLY_IF_RW { *(.gnu_extab) } :main
  .gcc_except_table   : ONLY_IF_RW { *(.gcc_except_table .gcc_except_table.*) } :main
  .exception_ranges   : ONLY_IF_RW { *(.exception_ranges*) } :main
  /* Thread Local Storage sections.  */
  .tdata	  :
   {
     PROVIDE_HIDDEN (__tdata_start = .);
     *(.tdata .tdata.* .gnu.linkonce.td.*)
   } :main
  .tbss		  : { *(.tbss .tbss.* .gnu.linkonce.tb.*) *(.tcommon) } :main
  .preinit_array    :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } :main
  .init_array    :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
    PROVIDE_HIDDEN (__init_array_end = .);
  } :main
  .fini_array    :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
    KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } :main
  .ctors          :
  {
    /* gcc uses crtbegin.o to find the start of
       the constructors, so we make sure it is
       first.  Because this is a wildcard, it
       doesn't matter if the user does not
       actually link against crtbegin.o; the
       linker won't look for a file to match a
       wildcard.  The wildcard also means that
       it doesn't matter which directory crtbegin.o
       is in.  */
    KEEP (*crtbegin.o(.ctors))
    KEEP (*crtbegin?.o(.ctors))
    /* We don't want to include the .ctor section from
       the crtend.o file until after the sorted ctors.
       The .ctor section from the crtend file contains
       the end of ctors marker and it must be last */
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
  } :main
  .dtors          :
  {
    KEEP (*crtbegin.o(.dtors))
    KEEP (*crtbegin?.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
  } :main
  .jcr            : { KEEP (*(.jcr)) } :main
  .data.rel.ro : { *(.data.rel.ro.local* .gnu.linkonce.d.rel.ro.local.*) *(.data.rel.ro .data.rel.ro.* .gnu.linkonce.d.rel.ro.*) } :main
  .data           :
  {
    _fdata = .;
    *(.data .data.* .gnu.linkonce.d.*)
    SORT(CONSTRUCTORS)
  } :main
  .data1          : { *(.data1) } :main
  .got.plt        : { *(.got.plt) } :main
  . = .; HIDDEN (_gp = ALIGN (16) + 0x7ff0);
  .got            : { *(.got) } :main
  /* We want the small data sections together, so single-instruction offsets
     can access them all, and initialized data all before uninitialized, so
     we can shorten the on-disk segment size.  */
  .sdata          :
  {
    *(.sdata .sdata.* .gnu.linkonce.s.*)
  } :main
  .lit8           : { *(.lit8) } :main
  .lit4           : { *(.lit4) } :main
  .srdata         : { *(.srdata) } :main
  _edata = .;
  PROVIDE (edata = .);
  . = ALIGN(ALIGNOF(NEXT_SECTION));
  __bss_start = .;
  _fbss = .;
  .sbss           :
  {
    *(.dynsbss)
    *(.sbss .sbss.* .gnu.linkonce.sb.*)
    *(.scommon)
  } :main
  .bss            :
  {
    *(.dynbss)
    *(.bss .bss.* .gnu.linkonce.b.*)
    *(COMMON)
    /* Align here to ensure that in the common case of there only being one
       type of .bss section, the section occupies space up to _end.
       Align after .bss to ensure correct alignment even if the
       .bss section disappears because there are no input sections.
       FIXME: Why do we need it? When there is no .bss section, we do not
       pad the .data section.  */
      . = ALIGN(. != 0 ? 32 / 8 : 1);
  } :main
  . = ALIGN(32 / 8);
  /* Start of the Large Data region.  */
  . = SEGMENT_START("ldata-segment", .);
  /* AURORA_GBA32_BOOSTER_V4: _end is BOTH crt0's bss-clear terminator
     (__start wipes [__bss_start, _end) with 16-byte sq stores) AND, via
     _main's "la $a0,_end" -> SetupHeap, the malloc heap start. V2/V3
     placed the booster inside that range, so real hardware zeroed the
     code at every boot -> every GBA game froze on entry (GB/GBC never
     call the wrap and stayed fine). V4 therefore:
       - keeps _end at the baseline address: crt0's clear loop stops
         right BEFORE the booster section (range is half-open),
       - places .gba32booster at [_end, ...) OUTSIDE the clear range in
         its own PT_LOAD "booster" (segment "main" stays byte-identical
         to the baseline image, ~4.2MB file),
       - defines __gba32_heap_start after the booster; Makefile's
         post-link step (tools/patch_gba32_heap.py) rewrites _main's
         "la $a0,_end" into "la $a0,__gba32_heap_start" so the heap
         starts AFTER the booster code (~0x2e8 bytes higher; harmless
         for every core). Zero symbol displacement is preserved. */
  _end = .;
  /* NB: crt0's clear loop is 16-byte grained (sq zero). With the
     baseline _end (0x...578, only 8-byte aligned) the LAST store
     actually wipes [ALIGN(_end,16)-16, ALIGN(_end,16)), i.e. it runs
     8 bytes PAST _end. Pad the booster start up to a 16-byte boundary
     so those overshoot bytes land in dead space, never in booster
     code. */
  . = ALIGN(16);
  .gba32booster : { *(.gba32booster_code) *(.gba32booster_data) } :main
  . = ALIGN(16);
  __gba32_heap_start = .;
  PROVIDE (end = .);
  /* Start of the Tiny Data region.  */
  /* Stabs debugging sections.  */
  .stab          0 : { *(.stab) }
  .stabstr       0 : { *(.stabstr) }
  .stab.excl     0 : { *(.stab.excl) }
  .stab.exclstr  0 : { *(.stab.exclstr) }
  .stab.index    0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment 0 (INFO) : { *(.comment); LINKER_VERSION; }
  .gnu.build.attributes : { *(.gnu.build.attributes .gnu.build.attributes.*) }
  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1.  */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions.  */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2.  */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2.  */
  .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line .debug_line.* .debug_line_end) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions.  */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }
  /* DWARF 3.  */
  .debug_pubtypes 0 : { *(.debug_pubtypes) }
  .debug_ranges   0 : { *(.debug_ranges) }
  /* DWARF 5.  */
  .debug_addr     0 : { *(.debug_addr) }
  .debug_line_str 0 : { *(.debug_line_str) }
  .debug_loclists 0 : { *(.debug_loclists) }
  .debug_macro    0 : { *(.debug_macro) }
  .debug_names    0 : { *(.debug_names) }
  .debug_rnglists 0 : { *(.debug_rnglists) }
  .debug_str_offsets 0 : { *(.debug_str_offsets) }
  .debug_sup      0 : { *(.debug_sup) }
  .gnu.attributes 0 : { KEEP (*(.gnu.attributes)) }
  .gptab.sdata : { *(.gptab.data) *(.gptab.sdata) } :main
  .gptab.sbss : { *(.gptab.bss) *(.gptab.sbss) } :main
  PROVIDE(_heap_size = -1);
  PROVIDE(_stack = -1);
  PROVIDE(_stack_size = 128 * 1024);
  /DISCARD/ : { *(.note.GNU-stack) *(.gnu_debuglink) *(.gnu.lto_*) *(.gnu_object_only) }
}
