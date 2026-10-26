OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

PHDRS
{
    code    PT_LOAD    FLAGS(5) ; /* R_E */
    rodata  PT_LOAD    FLAGS(4) ; /* R__ */
    data    PT_LOAD    FLAGS(6) ; /* RW_ */
    tls     PT_TLS     FLAGS(6) ; /* RW_ */
}

SECTIONS
{

    PROVIDE(__start__ = 0x08000000);
    . = __start__;

    .text ALIGN(0x1000) :
    {
        . = ALIGN(4);
        KEEP( *(.text.main) )
        KEEP( *(.init) )
        . = ALIGN(4);

        *(.text)
        *(.text.*)
        *(.glue_7)
        *(.glue_7t)
        *(.stub)
        *(.gnu.warning)
        *(.gnu.linkonce.t*)
        . = ALIGN(4);

        KEEP( *(.fini) )
        . = ALIGN(4);
    } : code

    . = ALIGN(0x1000);

    .rodata :
    {
        *(.rodata)
        *(.roda)
        *(.rodata.*)
        *all.rodata*(*)
        *(.gnu.linkonce.r*)
        SORT(CONSTRUCTORS)
        . = ALIGN(4);
        __tdata_align = .;
        LONG (ALIGNOF(.tdata));
        . = ALIGN(4);
    } : rodata

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } : rodata
    __exidx_start = .;
    .ARM.exidx : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) } : rodata
    __exidx_end = .;

    . = ALIGN(0x1000);

    .data :
    {
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d*)
        CONSTRUCTORS
        . = ALIGN(4);
    } : data

    .tdata : ALIGN(4)
    {
        __tdata_lma = .;
        *(.tdata)
        *(.tdata.*)
        *(.gnu.linkonce.td.*)
        . = ALIGN(4);
        __tdata_lma_end = .;
    } : data

    .tbss : ALIGN(4)
    {
        *(.tbss)
        *(.tbss.*)
        *(.gnu.linkonce.tb.*)
        *(.tcommon)
        . = ALIGN(4);
    } : data

    .init_array ALIGN(4) :
    {
        PROVIDE (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array))
        PROVIDE (__init_array_end = .);
    } : data

    __bss_start__ = .;
    .bss ALIGN(4) :
    {
        *(.dynbss)
        *(.bss)
        *(.bss.*)
        *(.gnu.linkonce.b*)
        *(COMMON)

        . = 8 + ABSOLUTE(ALIGN(ABSOLUTE(. - 8), MAX(ALIGNOF(.tdata), ALIGNOF(.tbss))));
        __tls_start = .;
        . += SIZEOF(.tdata);
        . = ALIGN(ALIGNOF(.tbss));
        . += SIZEOF(.tbss);
        __tls_end = .;
        /* Pad with a single zero byte so p_filesz covers the entire data+TLS region.
           This is needed for Azahar which doesn't map p_memsz > p_filesz. */
        BYTE(0)
    } : data
    __bss_end__ = .;

    __end__ = ABSOLUTE(.) ;

    /DISCARD/ : { *(.group .comment .note) }
}
