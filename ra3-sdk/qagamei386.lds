OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386");
OUTPUT_ARCH(i386);

ENTRY(_start);

SEARCH_DIR("/usr/i386-redhat-linux/lib");
SEARCH_DIR("/usr/lib");
SEARCH_DIR("/usr/local/lib");
SEARCH_DIR("/lib");

PHDRS
{
  text               PT_LOAD         FILEHDR PHDRS ;
  data               PT_LOAD                       ;
  dynamic            PT_DYNAMIC                    ;
  gnu_eh_frame       0x6474e550      FLAGS(4)      ;
}

SECTIONS
{
  /* -- read-only sections -------------------------------------------------- */

  .hash                                 0x000000b4 :
  {
    *(.hash);
  } :text

  .dynsym                               0x00002c20 :
  {
    *(.dynsym);
  } :text

  .dynstr                               0x00009940 :
  {
    *(.dynstr);
  } :text

  .gnu.version                          0x000104ce :
  {
    *(.gnu.version);
  } :text

  .gnu.version_r                        0x00011274 :
  {
    *(.gnu.version_r);
  } :text

  .rel.dyn                              0x00011314 :
  {
    *(.rel.text .rel.text.*);
    *(.rel.data.rel.ro*);
    *(.rel.data .rel.data.*);
    *(.rel.got);
  } :text

  .rel.plt                              0x0001680c :
  {
    *(.rel.plt);
  } :text

  .init                                 0x00018e0c :
  {
    KEEP(*(.init));
  } :text =0x90909090

  .plt                                  0x00018e24 :
  {
    *(.plt);
  } :text

  .text                                 0x0001da34 :
  {
    . = 0x0001da34 - 0x0001da34; /usr/lib/gcc-lib/*/*/*/*/*/crti.o (.text .text.*);
    . = 0x0001da58 - 0x0001da34; /usr/lib/gcc-lib/*/*/crtbeginS.o  (.text .text.*);
    . = 0x0001db00 - 0x0001da34; */sqlite/*.o                      (.text .text.*);
    . = 0x0005b22c - 0x0001da34; */httpd/*.o                       (.text .text.*);
    . = 0x0005e950 - 0x0001da34; */game/ai_chat.o                  (.text .text.*);
    . = 0x00061fa8 - 0x0001da34; */game/ai_cmd.o                   (.text .text.*);
    . = 0x000651cc - 0x0001da34; */game/ai_dmnet.o                 (.text .text.*);
    . = 0x0006ac10 - 0x0001da34; */game/ai_dmq3.o                  (.text .text.*);
    . = 0x00072654 - 0x0001da34; */game/ai_main.o                  (.text .text.*);
    . = 0x00075de4 - 0x0001da34; */game/ai_team.o                  (.text .text.*);
    . = 0x000781ac - 0x0001da34; */game/arena.o                    (.text .text.*);
    . = 0x00081fd0 - 0x0001da34; */game/arenacfg.o                 (.text .text.*);
    . = 0x0008452c - 0x0001da34; */game/arena_common.o             (.text .text.*);
    . = 0x000846b8 - 0x0001da34; */game/bg_lib.o                   (.text .text.*);
    . = 0x00085054 - 0x0001da34; */game/bg_misc.o                  (.text .text.*);
    . = 0x00086384 - 0x0001da34; */game/bg_pmove.o                 (.text .text.*);
    . = 0x0008a74c - 0x0001da34; */game/bg_slidemove.o             (.text .text.*);
    . = 0x0008b918 - 0x0001da34; */game/g_active.o                 (.text .text.*);
    . = 0x0008e1e4 - 0x0001da34; */game/g_arenas.o                 (.text .text.*);

    *(.text .text.*);
  } :text =0x90909090

  .fini                                 0x000bdf0c :
  {
    KEEP(*(.fini));
  } :text =0x90909090

  PROVIDE(__etext = .);
  PROVIDE(_etext = .);
  PROVIDE(etext = .);

  .rodata                               0x000bdf40 :
  {
    . = 0x000bdf40 - 0x000bdf40; */sqlite/*.o          (.rodata);
    . = 0x000ca0e0 - 0x000bdf40; */httpd/*.o           (.rodata);
    . = 0x000ca5d8 - 0x000bdf40; */game/ai_chat.o      (.rodata);
    . = 0x000ca980 - 0x000bdf40; */game/ai_cmd.o       (.rodata);
    . = 0x000cb200 - 0x000bdf40; */game/ai_dmnet.o     (.rodata);
    . = 0x000cb620 - 0x000bdf40; */game/ai_dmq3.o      (.rodata);
    . = 0x000cbe20 - 0x000bdf40; */game/ai_main.o      (.rodata);
    . = 0x000cc3d0 - 0x000bdf40; */game/ai_team.o      (.rodata);
    . = 0x000cc4e0 - 0x000bdf40; */game/arena.o        (.rodata);
    . = 0x000cda80 - 0x000bdf40; */game/arenacfg.o     (.rodata);
    . = 0x000cdea6 - 0x000bdf40; */game/arena_common.o (.rodata);
    . = 0x000cdf20 - 0x000bdf40; */game/bg_lib.o       (.rodata);
    . = 0x000cdf60 - 0x000bdf40; */game/bg_misc.o      (.rodata);
    . = 0x000cf890 - 0x000bdf40; */game/bg_pmove.o     (.rodata);
    . = 0x000cfa28 - 0x000bdf40; */game/bg_slidemove.o (.rodata);
    . = 0x000cfaa0 - 0x000bdf40; */game/g_active.o     (.rodata);
    . = 0x000cfbe0 - 0x000bdf40; */game/g_arenas.o     (.rodata);

    *(.rodata);
  } :text

  .eh_frame_hdr                         0x000d6604 :
  {
    *(.eh_frame_hdr);
  } :text :gnu_eh_frame

  .eh_frame                             0x000d6620 :
  {
    KEEP(*(.eh_frame));
  } :text

  /* -- data sections ------------------------------------------------------- */

  .data                                 0x000d76e0 :
  {
    . = 0x000d76e0 - 0x000d76e0; /usr/lib/gcc-lib/*/*/*/*/*/crti.o (.data .data.*);
    . = 0x000d76e0 - 0x000d76e0; /usr/lib/gcc-lib/*/*/crtbeginS.o  (.data .data.*);
    . = 0x000d7700 - 0x000d76e0; */sqlite/*.o                      (.data .data.*);
    . = 0x000db020 - 0x000d76e0; */httpd/*.o                       (.data .data.*);
    . = 0x000db0e4 - 0x000d76e0; */game/ai_chat.o                  (.data .data.*);
    . = 0x000db0f4 - 0x000d76e0; */game/ai_cmd.o                   (.data .data.*);
    . = 0x000db14c - 0x000d76e0; */game/ai_dmnet.o                 (.data .data.*);
    . = 0x000db15c - 0x000d76e0; */game/ai_dmq3.o                  (.data .data.*);
    . = 0x000db19c - 0x000d76e0; */game/ai_main.o                  (.data .data.*);
    . = 0x000db1b0 - 0x000d76e0; */game/ai_team.o                  (.data .data.*);
    . = 0x000db1c0 - 0x000d76e0; */game/arena.o                    (.data .data.*);
    . = 0x000db200 - 0x000d76e0; */game/arenacfg.o                 (.data .data.*);
    . = 0x000db258 - 0x000d76e0; */game/arena_common.o             (.data .data.*);
    . = 0x000db27c - 0x000d76e0; */game/bg_lib.o                   (.data .data.*);
    . = 0x000db280 - 0x000d76e0; */game/bg_misc.o                  (.data .data.*);
    . = 0x000dbb8c - 0x000d76e0; */game/bg_pmove.o                 (.data .data.*);
    . = 0x000dbbc0 - 0x000d76e0; */game/bg_slidemove.o             (.data .data.*);
    . = 0x000dbbc0 - 0x000d76e0; */game/g_active.o                 (.data .data.*);
    . = 0x000dbbdc - 0x000d76e0; */game/g_arenas.o                 (.data .data.*);

    *(.data .data.*);
  } :data

  .dynamic                              0x000de3a0 :
  {
    *(.dynamic);
  } :data :dynamic

  .ctors                                0x000de468 :
  {
    KEEP(*crtbegin.o(.ctors));
    KEEP(*(EXCLUDE_FILE (*crtend.o ) .ctors));
    KEEP(*(SORT(.ctors.*)));
    KEEP(*(.ctors));
  } :data

  .dtors                                0x000de470 :
  {
    KEEP(*crtbegin.o(.dtors));
    KEEP(*(EXCLUDE_FILE (*crtend.o ) .dtors));
    KEEP(*(SORT(.dtors.*)));
    KEEP(*(.dtors));
  } :data

  .jcr                                  0x000de478 :
  {
    KEEP(*(.jcr));
  } :data

  .got                                  0x000de47c :
  {
    *(.got.plt);
    *(.got);
  } :data

  _edata = .;
  PROVIDE (edata = .);
  __bss_start = .;

  .bss                                  0x000dfc20 :
  {
    . = 0x000dfc20 - 0x000dfc20; /usr/lib/gcc-lib/*/*/*/*/*/crti.o (.dynbss .bss);
    . = 0x000dfc20 - 0x000dfc20; /usr/lib/gcc-lib/*/*/crtbeginS.o  (.bss);
    . = 0x000dfc40 - 0x000dfc20; */sqlite/*.o                      (.bss);
    . = 0x000e01e0 - 0x000dfc20; */httpd/*.o                       (.bss);
    . = 0x000e2be0 - 0x000dfc20; */game/ai_chat.o                  (.bss);
    . = 0x000e2d20 - 0x000dfc20; */game/ai_cmd.o                   (.bss);
    . = 0x000e2d2c - 0x000dfc20; */game/ai_dmnet.o                 (.bss);
    . = 0x000e2d2c - 0x000dfc20; */game/ai_dmq3.o                  (.bss);
    . = 0x000e2d30 - 0x000dfc20; */game/ai_main.o                  (.bss);
    . = 0x000e2d3c - 0x000dfc20; */game/ai_team.o                  (.bss);
    . = 0x000e2d60 - 0x000dfc20; */game/arena.o                    (.bss);
    . = 0x000e2d60 - 0x000dfc20; */game/arenacfg.o                 (.bss);
    . = 0x000e35e0 - 0x000dfc20; */game/arena_common.o             (.bss);
    . = 0x000e3660 - 0x000dfc20; */game/bg_lib.o                   (.bss);
    . = 0x000e3660 - 0x000dfc20; */game/bg_misc.o                  (.bss);
    . = 0x000e3660 - 0x000dfc20; */game/bg_pmove.o                 (.bss);
    . = 0x000e3660 - 0x000dfc20; */game/bg_slidemove.o             (.bss);
    . = 0x000e3660 - 0x000dfc20; */game/g_active.o                 (.bss);
    . = 0x000e3660 - 0x000dfc20; */game/g_arenas.o                 (.bss);

    *(.bss);
    *(COMMON);

    /* align here to ensure that the .bss section occupies space up to _end */
   . = ALIGN(32 / 8);
  } :data

  _end = .;
  PROVIDE(end = .);

  /* -- debug sections ------------------------------------------------------ */

  .comment        0 : { *(.comment); }
  .debug_aranges  0 : { *(.debug_aranges); }
  .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*); }
  .debug_abbrev   0 : { *(.debug_abbrev); }
  .debug_line     0 : { *(.debug_line); }
  .debug_ranges   0 : { *(.debug_ranges); }
}
