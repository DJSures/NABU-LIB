SECTION code_user

PUBLIC _vdp_paint

EXTERN __vdp_textBuffer

_vdp_paint:
    push ix
    push iy

    ld	d,0x03
	ld	e,0x00          ; 300 bytes //XXX - HARDCODING FOR TESTING

	ld	c,0xA0		    ; the I/O port number
    ld  hl, __vdp_textBuffer
vdp_write_loop:
    outi
    dec	de
    ld	a,d
    or  e
    jp	nz,vdp_write_loop

    pop iy
    pop ix
    ret