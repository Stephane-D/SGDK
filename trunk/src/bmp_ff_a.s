	.align	2
	.globl	blitTileMap
	.type	blitTileMap, @function
blitTileMap:

	movm.l #0x2030,-(%sp)           | VDP autoinc should be equal to 2
	jbsr VDP_getScreenHeight
	lsr.w #3,%d0
	move.l %d0,%d1
	and.l #8191,%d1
	move.l %d1,%a0
	lea (-20,%a0),%a0
	move.l %a0,%d2
	jpl .L53
	addq.l #1,%d2
.L53:
	asr.l #1,%d2
	lsl.l #6,%d2
	jbsr VDP_getScreenWidth
	lsr.w #3,%d0
	and.l #8191,%d0
	move.w #-32,%a0
	add.l %d0,%a0
	move.l %a0,%d0
	jpl .L54
	addq.l #1,%d0
.L54:
	asr.l #1,%d0
	move.l %d2,%a0
	add.l %d0,%a0
	add.l %a0,%a0
	move.l bmp_buffer_0,%d1
	cmp.l bmp_buffer_read,%d1
	jne .L41

	move.l %a0,%d0
	lsl.l #2,%d0
	move.l %d0,%a2
	add.l #vramwrite_tab+196608,%a2
	jra .L42

	.align	2
.L41:
	move.l %a0,%d0
	lsl.l #2,%d0
	move.l %d0,%a2
	add.l #vramwrite_tab+212992,%a2

.L42:
	move.l bmp_tilemap_read,%a0
	move.l #12582916,%a3
	move.l #12582912,%a1
	moveq #19,%d2

	.align	2
.L48:
	move.l (%a2),(%a3)

	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)

	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)
	move.l (%a0)+,(%a1)

	lea (512,%a2),%a2
	dbra %d2,.L48

	movm.l (%sp)+,#0xc04
	rts


	.align	2
	.globl	drawLine
	.type	drawLine, @function
drawLine:
	movm.l #0x3f3c,-(%sp)

	move.l 48(%sp),%d0          |
	move.w %d0,%a5              | a5 = dx
	move.w 54(%sp),%a2          | a2 = dy
	move.w 58(%sp),%d7          | d7 = step_x
	move.w 62(%sp),%a4          | a4 = step_y
	move.b 67(%sp),%d6          | d6 = color (c)
	move.l bmp_buffer_write,%a1 | a1 = dst

	moveq #-1,%d2               | d2 = prev_off
	moveq #0,%d1
	move.w 46(%sp),%d1          | d1 = off

	move.w %d0,%d3
	lsr.w #1,%d3                | d3 = delta = dx >> 1
	move.w %d0,%d4              | d4 = cnt = dx
	move.w #64636,%d5           | d5 = off_msk = ~((BMP_YPIXPERTILEMASK * BMP_WIDTH) | BMP_XPIXPERTILEMASK)

	lea offset2tile,%a3         | a3 = offset2tile
	.align	2

.L140:                          | {
	eor.w %d1,%d2               |   if ((prev_off ^ off) & off_msk)
	and.w %d5,%d2               |   {
	jeq .L137                   |

	moveq #0,%d0                |
	move.w %d1,%d0              |
	add.w %d0,%d0               |
	move.w (%a3,%d0.l),%d0      |       d0 = tile_ind = offset2tile[off]
	move.w %d0,%d2              |
	add.w basetile_ind,%d2      |       d2 = usr_tile = basetile_ind + tile_ind
	add.w %d0,%d0               |
	move.l %d0,%a0              |
	add.l bmp_tilemap_write,%a0 |       a0 = tile = &bmp_tilemap_write[tile_ind]
	cmp.w (%a0),%d2             |       if (*tile != usr_tile)
	jeq .L137                   |       {

	move.w %d2,(%a0)            |           *tile = usr_tile;
	move.w %d1,%d0              |
	and.w %d5,%d0               |           d0 = off & off_msk
	move.l %d0,%a0
	add.l bmp_buffer_write,%a0  |           a0 = pix = bmp_buffer_write[off & ~((BMP_YPIXPERTILEMASK * BMP_WIDTH) | BMP_XPIXPERTILEMASK)]

	moveq #0, %d0
	move.l %d0,(%a0)            |           pix[(BMP_PITCH * 0) / 4] = 0;
	move.l %d0,128(%a0)         |           pix[(BMP_PITCH * 1) / 4] = 0;
	move.l %d0,256(%a0)         |           ...
	move.l %d0,384(%a0)
	move.l %d0,512(%a0)
	move.l %d0,640(%a0)
	move.l %d0,768(%a0)
	move.l %d0,896(%a0)
                                |       }
                                |   }

.L137:
	move.b %d6,(%a1,%d1.l)      |   dst[off] = c;
	move.w %d1,%d2              |   prev_off = off;
	add.w %d7,%d1               |   off += step_x;
	sub.w %a2,%d3               |   if ((delta -= dy) < 0)
	jpl .L135                   |   {

	add.w %a4,%d1               |       off += step_y;
	add.w %a5,%d3               |       delta += dx;
                                |   }
	.align	2
.L135:
	dbra %d4,.L140              | } while(--cnt >= 0)

.L142:
	movm.l (%sp)+,#0x3cfc
	rts


	.align	2
	.globl	getTile
	.type	getTile, @function
getTile:

    moveq #0,%d0
	move.w 6(%sp),%d0               | d0 = offset
	add.w %d0,%d0
	lea offset2tile,%a0
	move.w (%a0,%d0.l),%d0          | d0 = offset2tile[offset]
	add.w %d0,%d0
	move.l bmp_tilemap_write,%a0
	move.w (%a0,%d0.w),%d0          | return bmp_tilemap_write[offset2tile[offset]];
	rts


	.align	2
	.globl	isUserTile
	.type	isUserTile, @function
isUserTile:

    moveq #0,%d0
	move.w 6(%sp),%d0               | d0 = offset
	add.w %d0,%d0
	lea offset2tile,%a0
	move.w (%a0,%d0.l),%d0          | d0 = tile_ind = offset2tile[offset]
	add.w %d0,%d0
	move.l bmp_tilemap_write,%a0
	cmpi.w #15,(%a0,%d0.w)          | bmp_tilemap_write[tile_ind] >= TILE_USERINDEX
	sle %d0
	rts


	.align	2
	.globl	setUserTile
	.type	setUserTile, @function
setUserTile:
	move.l %d2,-(%sp)

    moveq #0,%d0
	move.w 10(%sp),%d0              | d0 = offset
	move.l %d0,%d2                  | d2 = offset
	add.w %d0,%d0
	lea offset2tile,%a0
	move.w (%a0,%d0.l),%d0          | d0 = tile_ind = offset2tile[offset]
	move.l %d0,%d1
	add.w %d1,%d1
	move.l bmp_tilemap_write,%a0
	add.l %d1,%a0                   | a0 = tile = &bmp_tilemap_write[tile_ind]
	add.w basetile_ind,%d0          | d0 = usr_tile = tile_ind + basetile_ind

	cmp.w (%a0),%d0                 | if (*tile != usr_tile)
	jeq .L152                       | {

	move.w %d0,(%a0)                |   *tile = usr_tile;

	move.w %d2,%d0                  |
	and.w #64636,%d0                |   d0 = off = offset & ~((BMP_YPIXPERTILEMASK * BMP_WIDTH) | BMP_XPIXPERTILEMASK)

	move.l bmp_buffer_write,%a0
	add.l %d0,%a0                   |   a0 = pix = &bmp_buffer_write[off]

	moveq #0, %d0
	move.l %d0,(%a0)                |   pix[(BMP_PITCH * 0) / 4] = 0;
	move.l %d0,128(%a0)             |   pix[(BMP_PITCH * 1) / 4] = 0;
	move.l %d0,256(%a0)             |   ...
	move.l %d0,384(%a0)
	move.l %d0,512(%a0)
	move.l %d0,640(%a0)
	move.l %d0,768(%a0)
	move.l %d0,896(%a0)

.L152:                              | }
	move.l (%sp)+,%d2
	rts
