OUTPUT_ARCH(m68k)
SEARCH_DIR(.)
/*GROUP(-lbcc -lc -lgcc)*/
__DYNAMIC  =  0;

/*
 * Setup the memory map of the SEGA Genesis.
 * stack grows down from high memory.
 *
 * The memory map look like this:
 * +--------------------+ <- low memory
 * | .text              |
 * |        _etext      |
 * |        ctor list   | the ctor and dtor lists are for
 * |        dtor list   | C++ support
 * +--------------------+
 * | .data              | initialized data goes here
 * |        _data       | 
 * |        _edata      |
 * +--------------------+
 * | .bss               |
 * |        _bstart     | start of bss, cleared by crt0
 * |        _bend       | start of heap, used by sbrk()
 * |        __bss_start | start of bss, cleared by crt0
 * |        _end        | start of heap, used by sbrk()
 * +--------------------+
 * .                    .
 * .                    .
 * .                    .
 * |        __stack     | top of stack
 * +--------------------+
 */
MEMORY
{
	rom : ORIGIN = 0x00000000, LENGTH = 0x00400000
	ram : ORIGIN = 0x00ff0000, LENGTH = 0x00010000
  }

/*
 * allocate the stack to be at the top of memory, since the stack
 * grows down
 */

PROVIDE (__stack = 0x00fffff0);

/*
 * Initalize some symbols to be zero so we can reference them in the
 * crt0 without core dumping. These functions are all optional, but
 * we do this so we can have our crt0 always use them if they exist.
 * This is so BSPs work better when using the crt0 installed with gcc.
 * We have to initalize them twice, so we cover a.out (which prepends
 * an underscore) and coff object file formats.
 */
PROVIDE (hardware_init_hook = 0);
PROVIDE (_hardware_init_hook = 0);
PROVIDE (software_init_hook = 0);
PROVIDE (_software_init_hook = 0);

SECTIONS
{
  .text 0x00000000:
  {
    *(.text)
    . = ALIGN(0x4);
     __CTOR_LIST__ = .;
    LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
    *(.ctors)
    LONG(0)
    __CTOR_END__ = .;
    __DTOR_LIST__ = .;
    LONG((__DTOR_END__ - __DTOR_LIST__) / 4 - 2)
    *(.dtors)
     LONG(0)
    __DTOR_END__ = .;
    *(.rodata*)
    *(.gcc_except_table)

    __INIT_SECTION__ = . ;
    *(.init)
    SHORT (0x4e75)	/* rts */

    __FINI_SECTION__ = . ;
    *(.fini)
    SHORT (0x4e75)	/* rts */

    _etext = .;
    *(.lit)
  } > rom

  .data BLOCK (0x4) :
  {
    *(.shdata)
    *(.data)
    _edata = .;
  } > ram

  .bss 0xff0000 :
  {
    __bss_start = . ;
    *(.shbss)
    *(.bss)
    *(COMMON)
    *(.eh_fram)
    *(.eh_frame)
    _end =  ALIGN (0x8);
    __end = _end;
  } > ram

  .stab 0 (NOLOAD) :
  {
    *(.stab)
  }

  .stabstr 0 (NOLOAD) :
  {
    *(.stabstr)
  }
}
