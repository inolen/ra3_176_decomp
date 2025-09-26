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
    . = 0x000bdf40 - 0x000bdf40; */sqlite/*.o      (.rodata);
    . = 0x000ca0e0 - 0x000bdf40; */httpd/*.o       (.rodata);

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
