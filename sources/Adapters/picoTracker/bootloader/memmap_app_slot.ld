MEMORY {
    FLASH(rx) : ORIGIN = 0x10010000, LENGTH = 0x007F0000
    RAM(rwx) : ORIGIN =  0x20000000, LENGTH = 256k
}

ENTRY(reset_handler)

SECTIONS {
    .text : { *(.text*) *(.rodata*) } > FLASH
    .data : { *(.data*) } > RAM AT> FLASH
    .bss  : { *(.bss*) *(COMMON) } > RAM
}