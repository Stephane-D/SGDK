	.globl	clearBitmapBuffer
	.type	clearBitmapBuffer, @function
clearBitmapBuffer:
	move.l 4(%sp),%a0           | a0 = buffer
	lea 20480(%a0),%a0          | a0 = buffer end

	movm.l %d2-%d7/%a2-%a6,-(%sp)

    moveq #0,%d1
    move.l %d1,%d2
    move.l %d1,%d3
    move.l %d1,%d4
    move.l %d1,%d5
    move.l %d1,%d6
    move.l %d1,%d7
    move.l %d1,%a1
    move.l %d1,%a2
    move.l %d1,%a3
    move.l %d1,%a4
    move.l %d1,%a5
    move.l %d1,%a6

	moveq #38,%d0

.L01:
	movm.l %d1-%d7/%a1-%a6,-(%a0)
	movm.l %d1-%d7/%a1-%a6,-(%a0)
	movm.l %d1-%d7/%a1-%a6,-(%a0)
	movm.l %d1-%d7/%a1-%a6,-(%a0)
	movm.l %d1-%d7/%a1-%a6,-(%a0)
	movm.l %d1-%d7/%a1-%a6,-(%a0)
	movm.l %d1-%d7/%a1-%a6,-(%a0)
	movm.l %d1-%d7/%a1-%a6,-(%a0)
	movm.l %d1-%d7/%a1-%a6,-(%a0)
	movm.l %d1-%d7/%a1-%a6,-(%a0)
	dbra %d0,.L01

	movm.l %d1-%d7/%a1-%a6,-(%a0)
	movm.l %d1-%d7/%a1-%a6,-(%a0)
	movm.l %d1-%d7/%a1-%a6,-(%a0)
	movm.l %d1-%d7/%a1-%a4,-(%a0)

	movm.l (%sp)+,%d2-%d7/%a2-%a6
	rts


    | internal use only
    | -----------------
    | IN:
    | d2 = x1
    | d3 = y1
    | d4 = x2
    | d5 = y2
    |
    | OUT:
    | d0 = ZFLAG = 0 if outside screen
    |              1 if inside screen
    | d1 = BMP_HEIGHT - 1
    | d2 = x1
    | d3 = y1
    | d4 = x2
    | d5 = y2
    | d6-d7 = ??
clipLine:

	move.w #255,%d0             | d0 = BMP_WIDTH - 1
	move.w #159,%d1             | d1 = BMP_HEIGHT - 1

	cmp.w %d0,%d2               | if (((u16) x1 < BMP_WIDTH) &&
	jhi .L50
	cmp.w %d0,%d4               |     ((u16) x2 < BMP_WIDTH) &&
	jhi .L50
	cmp.w %d1,%d3               |     ((u16) y1 < BMP_HEIGHT) &&
	jhi .L50
	cmp.w %d1,%d5               |     ((u16) y2 < BMP_HEIGHT))
	jhi .L50

	moveq #1,%d0                |   return 1;
	rts

.L60:
	move.w %d4,%d6
	sub.w %d2,%d6               | d6 = dx = x2 - x1;
	move.w %d5,%d7
	sub.w %d3,%d7               | d7 = dy = y2 - y1;

	tst.w %d2                   | if (x1 < 0)
	jge .L38                    | {

	muls.w %d7,%d2              |   y1 -= (x1 * dy) / dx;
	divs.w %d6,%d2
	sub.w %d2,%d3
	moveq #0,%d2                |   x1 = 0;
	jra .L39                    | }

.L38:
	cmp.w %d0,%d2               | else if (x1 >= BMP_WIDTH)
	jle .L39                    | {

    sub.w %d0,%d2
	muls.w %d7,%d2              |   y1 -= ((x1 - (BMP_WIDTH - 1)) * dy) / dx;
	divs.w %d6,%d2
	sub.w %d2,%d3
	move.w %d0,%d2              |   x1 = BMP_WIDTH - 1;
                                | }
.L39:
	tst.w %d4                   | if (x2 < 0)
	jge .L41                    | {

	muls.w %d7,%d4              |   y2 -= (x2 * dy) / dx;
	divs.w %d6,%d4
	sub.w %d4,%d5
	moveq #0,%d4                |   x2 = 0;
	jra .L42                    | }

.L41:
	cmp.w %d0,%d4               | else if (x2 >= BMP_WIDTH)
	jle .L42                    | {

    sub.w %d0,%d4
	muls.w %d7,%d4              |   y2 -= ((x2 - (BMP_WIDTH - 1)) * dy) / dx;
	divs.w %d6,%d4
	sub.w %d4,%d5
	move.w %d0,%d4              |   x2 = BMP_WIDTH - 1;
                                | }
.L42:
	tst.w %d3                   | if (y1 < 0)
	jge .L44                    | {

	muls.w %d6,%d3              |   x1 -= (y1 * dx) / dy;
	divs.w %d7,%d3
	sub.w %d3,%d2
	moveq #0,%d3                |   y1 = 0;
	jra .L45                    | }

.L44:
	cmp.w %d1,%d3               | else if (y1 >= BMP_HEIGHT)
	jle .L45                    | {

    sub.w %d1,%d3
	muls.w %d6,%d3              |   x1 -= ((y1 - (BMP_HEIGHT - 1)) * dx) / dy;
	divs.w %d7,%d3
	sub.w %d3,%d2
	move.w %d1,%d3              |   y1 = BMP_HEIGHT - 1;

.L45:	                        | }
	tst.w %d5                   | if (y2 < 0)
	jge .L47                    | {

	muls.w %d6,%d5              |   x2 -= (y2 * dx) / dy;
	divs.w %d7,%d5
	sub.w %d5,%d4
	moveq #0,%d5                |   y2 = 0;
	jra .L48                    | }

.L47:
	cmp.w %d1,%d5               | else if (y2 >= BMP_HEIGHT)
	jle .L48                    | {

    sub.w %d1,%d5
	muls.w %d6,%d5              |   x2 -= ((y2 - (BMP_HEIGHT - 1)) * dx) / dy;
	divs.w %d7,%d5
	sub.w %d5,%d4
	move.w %d1,%d5              |   y2 = BMP_HEIGHT - 1;
                                | }
.L48:
	cmp.w %d0,%d2               | if (((u16) x1 < BMP_WIDTH) &&
	jhi .L50
	cmp.w %d0,%d4               |     ((u16) x2 < BMP_WIDTH) &&
	jhi .L50
	cmp.w %d1,%d3               |     ((u16) y1 < BMP_HEIGHT) &&
	jhi .L50
	cmp.w %d1,%d5               |     ((u16) y2 < BMP_HEIGHT))
	jhi .L50                    | {

	moveq #1,%d0
	rts                         | }

.L50:
	tst.w %d2                   | if (((x1 < 0) && (x2 < 0)) ||
	jge .L53
	tst.w %d4
	jlt .L52

.L53:
	cmp.w %d0,%d2               |     ((x1 >= BMP_WIDTH) && (x2 >= BMP_WIDTH)) ||
	jle .L54
	cmp.w %d0,%d4
	jgt .L52

.L54:
	tst.w %d3                   |     ((y1 < 0) && (y2 < 0)) ||
	jge .L55
	tst.w %d5
	jlt .L52

.L55:
	cmp.w %d1,%d3               |     ((y1 >= BMP_HEIGHT) && (y2 >= BMP_HEIGHT)))
	jle .L60
	cmp.w %d1,%d5
	jle .L60

.L52:
	moveq #0,%d0                |   return 0;
	rts


	.globl	BMP_clipLine
	.type	BMP_clipLine, @function
BMP_clipLine:
	movm.l %d2-%d7,-(%sp)

	move.l 28(%sp),%a0          | a0 = &line
	movm.w (%a0),%d2-%d5        | d2 = x1, d3 = y1, d4 = x2, d5 = y2

    jsr clipLine
	jeq .L10

	movm.w %d2-%d5,(%a0)        |   update line

.L10:
	movm.l (%sp)+,%d2-%d7
	rts


	.globl	BMP_drawLine
	.type	BMP_drawLine, @function
BMP_drawLine:
	movm.l %d2-%d7,-(%sp)

	move.l 28(%sp),%a0      | a0 = &line
	movem.w (%a0)+,%d2-%d5  | d2 = x1, d3 = y1, d4 = x2, d5 = y2

	jsr clipLine
	jeq .L105

	move.b (%a0),%d6        | d6 = col

	asr.w #1,%d2            | d2 = x1 adjusted
	asr.w #1,%d4            | d4 = x2 adjusted

	sub.w %d2,%d4           | d4 = deltax
	sub.w %d3,%d5           | d5 = deltay

	lsl.w #7,%d3
	add.w %d2,%d3               | d3.l = offset = (y1 * BMP_PITCH) + x1
    ext.l %d3
	add.l bmp_buffer_write,%d3  | d3 = &bmp_buffer_write[offset];
	move.l %d3,%a0              | a0 = *dst = &bmp_buffer_write[offset];

	moveq #1,%d0            | d0 = stepx = 1

	tst.w %d4               | if (deltax < 0)
	jge .L100               | {

	neg.w %d4               |     deltax = -deltax;
	neg.w %d0               |     stepx = -stepx;
                            | }
.L100:
	move.w #128,%d1         | d1 = stepy = BMP_PITCH;

	tst.w %d5               | if (deltay < 0)
	jge .L101               | {

	neg.w %d5               |     deltay = -deltay;
	neg.w %d1               |     stepy = -stepy;
                            | }
.L101:                      |
	cmp.w %d4,%d5           | if (deltax < deltay)
	jle .L102               | {

    exg %d4,%d5             |     swap(deltax, deltay);
    exg %d0,%d1             |     swap(stepx, stey);
                            | }
.L102:                      |
	move.w %d4,%d2
	asr.w #1,%d2            | d2 = delta = dx >> 1
	move.w %d4,%d3          | d3 = cnt
	add.w %d0,%d1           | d1 = stepx + stepy

.L103:                      | while(cnt--)
                            | {
	move.b %d6,(%a0)        |     *dst = col;

	sub.w %d5,%d2           |     if ((delta -= dy) < 0)
	jpl .L104               |     {

	add.w %d1,%a0           |         dst += stepx + stepy; (can be 16 bits as dst is in RAM)
	add.w %d4,%d2           |         delta += dx;
                            |     }
	dbra %d3,.L103          |     else

	movm.l (%sp)+,%d2-%d7
	rts

.L104:                      |
	add.w %d0,%a0           |         dst += stepx; (can be 16 bits as dst is in RAM)
	dbra %d3,.L103          | }

.L105:
	movm.l (%sp)+,%d2-%d7
	rts


	.globl	BMP_isPolygonCulled
	.type	BMP_isPolygonCulled, @function
BMP_isPolygonCulled:
	movm.l %d2-%d5,-(%sp)

	move.l 20(%sp),%a1          | a1 = pts
    movm.w (%a1),%d0-%d5        | d0 = pts[0].x, d1 = pts[0].y, d2 = pts[1].x, d3 = pts[1].y, d4 = pts[2].x, d5 = pts[2].y

    sub.w %d0,%d4               | d4 = x2 - x0
    sub.w %d1,%d3               | d3 = y1 - y0
    muls.w %d4,%d3

    sub.w %d0,%d2               | d2 = x1 - x0
    sub.w %d1,%d5               | d5 = y2 - y0
    muls.w %d5,%d2

    cmp.l %d3,%d2               | culling test
	jge .L11                    | if (d4 * d3 < d5 * d2)

	moveq #1,%d0                |     return 1;
	movm.l (%sp)+,%d2-%d5
	rts

.L11:
	moveq #0,%d0                |     return 0;
	movm.l (%sp)+,%d2-%d5
	rts


    | internal use only
    | -----------------
    | a1 = pts
    | a2 = &minYL
    | a3 = &minYR
    | a4 = LeftPoly
    | a5 = RightPoly
calculatePolyEdge:

	movem.w (%a1),%d2-%d5       | d2 = x1, d3 = y1, d4 = x2, d5 = y2

	move.w %d5,%d7              | d7 = dy = y2 - y1;
	sub.w %d3,%d7
	jeq .L61                    | if (dy == 0) return;

	tst.w %d3                   | if (y1 < 0)
	jpl .L68                    | {

	tst.w %d5                   |     if (y2 < 0)
	jmi .L61                    |         return;

	move.w %d4,%d6
	sub.w %d2,%d6               |     d6 = dx = x2 - x1;
	move.w %d6,%d1              |     d1 = dx (save)

    ext.l %d6
    asl.l #6,%d6
    divs.w %d7,%d6              |     d6 = stepx;           // fix16 format

	move.w #159,%d0             |     d0 = BMP_HEIGHT - 1

	muls.w %d6,%d3
    asr.l #6,%d3
	sub.w %d3,%d2               |     x1 -= y1 * stepx;
	moveq #0,%d3                |     y1 = 0;
	jra .L71                    |     goto L71;
                                | }
.L68:
	move.w %d4,%d6
	sub.w %d2,%d6               | d6 = dx = x2 - x1;
	move.w %d6,%d1              | d1 = dx (save)

    ext.l %d6
    asl.l #6,%d6
    divs.w %d7,%d6              | d6 = stepx;           // fix16 format

	move.w #159,%d0             | d0 = BMP_HEIGHT - 1

	cmp.w %d0,%d3               | if (y1 >= BMP_HEIGHT)
	jle .L69                    | {

	cmp.w %d0,%d5               |     if (y2 >= BMP_HEIGHT)
	jgt .L61                    |         return;

    sub.w %d0,%d3
	muls.w %d6,%d3
    asr.l #6,%d3
	sub.w %d3,%d2               |     x1 -= (y1 - (BMP_HEIGHT - 1)) * stepx;
	move.w %d0,%d3              |     y1 = BMP_HEIGHT - 1;

	tst.w %d5                   |     if (y2 < 0)
	jpl .L72                    |     {

	muls.w %d6,%d5
    asr.l #6,%d5
	sub.w %d5,%d4               |         x2 -= y2 * stepx;
	moveq #0,%d5                |         y2 = 0;
	jra .L72                    |     }
                                |     goto L72;
.L69:	                        | }
	tst.w %d5                   | if (y2 < 0)
	jpl .L71                    | {

	muls.w %d6,%d5
    asr.l #6,%d5
	sub.w %d5,%d4               |     x2 -= y2 * stepx;
	moveq #0,%d5                |     y2 = 0;
	jra .L72                    |     goto L72;
                                | }
.L71:
	cmp.w %d0,%d5               | if (y2 >= BMP_HEIGHT)
	jle .L72                    | {

    sub.w %d0,%d5
	muls.w %d6,%d5
    asr.l #6,%d5
	sub.w %d5,%d4               |     x2 -= (y2 - (BMP_HEIGHT - 1)) * stepx;
	move.w %d0,%d5              |     y2 = BMP_HEIGHT - 1;
                                | }
.L72:
	cmp.w %d5,%d3               | if (y1 == y2) return;
	jeq .L61

    jgt .L80                    | if (y2 > y1)     // right edge (clockwise order)
.L82:                           | {
	tst.w %d2                   |     if (x1 < 0)
	jpl .L77_0                  |     {

	tst.w %d4                   |         if (x2 < 0)
	jmi .L61                    |             return;

    ext.l %d7
    asl.l #6,%d7
    divs.w %d1,%d7              |         d7 = stepy;   // fix16 format

	muls.w %d7,%d2
    asr.l #6,%d2                |
    sub.w %d2,%d3               |         y1 -= x1 * stepy
    moveq #0,%d0                |         d0 = pre fill len = 0
    moveq #0,%d2                |         x1 = 0

    move.w #255,%d1             |         d1 = BMP_WIDTH - 1

	cmp.w %d1,%d4               |         if (x2 >= BMP_WIDTH)
	jle .L77_4                  |         {

    sub.w %d1,%d4
	muls.w %d7,%d4
    asr.l #6,%d4                |             d1 = post fill len = (x2 - (BMP_WIDTH - 1)) * stepy
    exg %d1,%d4                 |             x2 = BMP_WIDTH - 1
    jra .L77                    |         }

.L77_0:                         |     }
    move.w #255,%d0

	cmp.w %d0,%d2               |     else if (x1 >= BMP_WIDTH)
	jle .L77_1                  |     {

	cmp.w %d0,%d4               |         if (x2 >= BMP_WIDTH)
	jle .L77_0_1                |         {

	move.w %d0,%d2              |             x1 = BMP_WIDTH - 1
	move.w %d5,%d0
	sub.w %d3,%d0               |             d0 = pre fill len = y2 - y1
	moveq #0,%d1                |             d1 = post fill len = 0
	jra .L77                    |         }
                                |         else
.L77_0_1:                       |         {
    ext.l %d7
    asl.l #6,%d7
    divs.w %d1,%d7              |             d7 = stepy;   // fix16 format

    sub.w %d0,%d2
	muls.w %d7,%d2
    asr.l #6,%d2                |             d0 = pre fill len = (x1 - (BMP_WIDTH - 1)) * stepy
    exg %d0,%d2                 |             x1 = BMP_WIDTH - 1

	tst.w %d4                   |             if  (x2 < 0)
	jpl .L77_4                  |             {

	muls.w %d7,%d4
    asr.l #6,%d4
    add.w %d4,%d5               |                 y2 += x2 * stepy
    moveq #0,%d1                |                 d1 = post fill len = 0
    moveq #0,%d4                |                 x2 = 0
    jra .L77                    |             }
                                |         }
.L77_1:                         |     }
	tst.w %d4                   |     else if (x2 < 0)
	jpl .L77_2                  |     {

    ext.l %d7
    asl.l #6,%d7
    divs.w %d1,%d7              |         d7 = stepy;   // fix16 format

	muls.w %d7,%d4
    asr.l #6,%d4
    add.w %d4,%d5               |         y2 += x2 * stepy
    moveq #0,%d0                |         d0 = pre fill len = 0
    moveq #0,%d1                |         d1 = post fill len = 0
    moveq #0,%d4                |         x2 = 0
    jra .L77                    |     }

.L77_2:
	cmp.w %d0,%d4               |     else if (x2 >= BMP_WIDTH)
	jle .L77_3                  |     {

    ext.l %d7
    asl.l #6,%d7
    divs.w %d1,%d7              |         d7 = stepy;   // fix16 format

    sub.w %d0,%d4
	muls.w %d7,%d4
    asr.l #6,%d4                |         d4 = post fill len = (x2 - (BMP_WIDTH - 1)) * stepy
    move.w %d4,%d1              |         d1 = post fill len
    move.w %d0,%d4              |         x2 = BMP_WIDTH - 1
    moveq #0,%d0                |         d0 = pre fill len = 0
    jra .L77                    |     }
                                |     else
.L77_3:                         |     {
    moveq #0,%d0                |         d0 = pre fill len = 0
.L77_4:
    moveq #0,%d1                |         d1 = post fill len = 0
                                |     }
.L77:
	move.w %d5,%d7
	sub.w %d3,%d7               |     d7 = len = y2 - y1;

	cmp.w (%a3),%d3             |     if (y1 < minYR)
	jge .L83

	move.w %d3,(%a3)            |         minYR = y1;

.L83:
	cmp.w 2(%a3),%d5            |     if (y2 > maxYR)
	jle .L84

	move.w %d5,2(%a3)           |         maxYR = y2;

.L84:
    add.w %d3,%d3
    lea (%a5,%d3.w),%a0         |     a0 = src = &RightPoly[y1];
    move.w %d2,%d0              |     d0 = x = x1
	jra .L85                    | }
                                | else      // left edge
.L80:                           | {
    move.w #255,%d0             |     d0 = BMP_WIDTH - 1

	cmp.w %d0,%d2               |     if (x1 >= BMP_WIDTH)
	jle .L76_0                  |     {

	cmp.w %d0,%d4               |         if (x2 >= BMP_WIDTH))
	jgt .L61                    |             return;

    ext.l %d7
    asl.l #6,%d7
    divs.w %d1,%d7              |         d7 = stepy;   // fix16 format

    sub.w %d0,%d2
	muls.w %d7,%d2
    asr.l #6,%d2                |         d2 = (x1 - (BMP_WIDTH - 1)) * stepy
    add.w %d2,%d3               |         y1 += (x1 - (BMP_WIDTH - 1)) * stepy
    move.w %d0,%d2              |         x1 = BMP_WIDTH - 1
    moveq #0,%d1                |         d1 = post fill len
    moveq #0,%d0                |         d0 = pre fill len = 0

	tst.w %d4                   |         if  (x2 < 0)
	jpl .L76                    |         {

	muls.w %d7,%d4
    asr.l #6,%d4                |             d0 = pre fill len = x2 * stepy
    exg %d4,%d0                 |             x2 = 0
    jra .L76                    |         }

.L76_0:                         |     }
	tst.w %d2                   |     else if (x1 < 0)
	jpl .L76_1                  |     {

	tst.w %d4                   |         if  (x2 < 0)
	jpl .L76_0_1                |         {

	move.w %d3,%d0
	sub.w %d5,%d0               |             d0 = pre fill len = y1 - y2
	moveq #0,%d1                |             d1 = post fill len = 0
	moveq #0,%d4                |             x2 = 0
	jra .L76                    |         }
                                |         else
.L76_0_1:                       |         {
    ext.l %d7
    asl.l #6,%d7
    divs.w %d1,%d7              |             d7 = stepy;   // fix16 format

	muls.w %d7,%d2
    asr.l #6,%d2                |             d2 = post fill len = x1 * stepy
    move.w %d2,%d1              |             d1 = post fill len
    moveq #0,%d2                |             x1 = 0

	cmp.w %d0,%d4               |             if (x2 >= BMP_WIDTH)
	jle .L76_4                  |             {

    sub.w %d0,%d4
	muls.w %d7,%d4
    asr.l #6,%d4                |                 d4 = (x2 - (BMP_WIDTH - 1)) * stepy
    sub.w %d4,%d5               |                 y2 -= (x2 - (BMP_WIDTH - 1)) * stepy
    move.w %d0,%d4              |                 x2 = BMP_WIDTH - 1
    moveq #0,%d0                |                 d0 = pre fill len = 0
    jra .L76                    |             }
                                |         }
.L76_1:                         |     }
	tst.w %d4                   |     else if (x2 < 0)
	jpl .L76_2                  |     {

    ext.l %d7
    asl.l #6,%d7
    divs.w %d1,%d7              |         d7 = stepy;   // fix16 format

	muls.w %d7,%d4
    asr.l #6,%d4                |         d4 = pre fill len = x2 * stepy
    move.w %d4,%d0              |         d0 = pre fill len
    moveq #0,%d1                |         d1 = post fill len = 0
    moveq #0,%d4                |         x2 = 0
    jra .L76                    |     }

.L76_2:
	cmp.w %d0,%d4               |     else if (x2 >= BMP_WIDTH)
	jle .L76_3                  |     {

    ext.l %d7
    asl.l #6,%d7
    divs.w %d1,%d7              |         d7 = stepy;   // fix16 format

    sub.w %d0,%d4
	muls.w %d7,%d4
    asr.l #6,%d4                |         d4 = (x2 - (BMP_WIDTH - 1)) * stepy
    sub.w %d4,%d5               |         y2 -= (x2 - (BMP_WIDTH - 1)) * stepy
    move.w %d0,%d4              |         x2 = BMP_WIDTH - 1
    moveq #0,%d0                |         d0 = pre fill len = 0
    moveq #0,%d1                |         d1 = post fill len = 0
    jra .L76                    |     }
                                |     else
.L76_3:                         |     {
    moveq #0,%d1                |         d1 = post fill len = 0
.L76_4:
    moveq #0,%d0                |         d0 = pre fill len = 0
                                |     }
.L76:
	cmp.w (%a2),%d5             |     if (y2 < minYL)
	jge .L86

	move.w %d5,(%a2)            |         minYL = y2;

.L86:
	cmp.w 2(%a2),%d3            |     if (y1 > maxYL)
	jle .L87

	move.w %d3,2(%a2)           |         maxYL = y1;

.L87:
	move.w %d3,%d7
	sub.w %d5,%d7               |     d7 = len = y1 - y2;

    add.w %d5,%d5
    lea (%a4,%d5.w),%a0         |     a0 = src = &LeftPoly[y2];
    move.w %d4,%d0              |     d0 = x = x2
                                | }
.L85:
	ext.l %d6
    add.l %d6,%d6               | d6 = step << 7
	ror.l #8,%d6                | d6 = step >> 1 (32 bits fixed point)
	ext.l %d0                   | d0 = x (32 bits fixed point)
	add.l %d6,%d0               | d0 = x + (step >> 1)
	rol.l #1,%d6                | d6 = step (32 bits fixed point)
	movq #0,%d2                 | d2 = 0

    move.w %d7,%d1
	asr.w #3,%d1                | d1 = len8 = len >> 3
	and.w #7,%d7                | d7 = len = len & 7
	subq.w #1,%d1
	jmi .L95

	move %d2,%ccr               | clear X flag

.L90:                           | while(len8--)
	move.w %d0,(%a0)+           | {
	addx.l %d6,%d0
	move.w %d0,(%a0)+           |     *src++ = fix16ToInt(x);
	addx.l %d6,%d0              |     x += step;
	move.w %d0,(%a0)+           |     ...
	addx.l %d6,%d0              |
	move.w %d0,(%a0)+
	addx.l %d6,%d0
	move.w %d0,(%a0)+
	addx.l %d6,%d0
	move.w %d0,(%a0)+
	addx.l %d6,%d0
	move.w %d0,(%a0)+
	addx.l %d6,%d0
	move.w %d0,(%a0)+
	addx.l %d6,%d0

	dbra.w %d1,.L90             | }

	move %sr,%d2                | save X flag

.L95:
	subq.w #1,%d7
	jmi .L61

	move %d2,%ccr               | restore X flag

.L93:                           | while(len--)
	move.w %d0,(%a0)+           | {
	addx.l %d6,%d0              |     *src++ = fix16ToInt(x);
	dbra.W %d7,.L93             |     x += step;
                                | }
.L61:
	rts


	.globl	BMP_drawPolygon
	.type	BMP_drawPolygon, @function
BMP_drawPolygon:
	movm.l %d2-%d5,-(%sp)

	move.l 20(%sp),%a1          | a1 = pts
    movm.w (%a1),%d0-%d5        | d0 = pts[0].x, d1 = pts[0].y, d2 = pts[1].x, d3 = pts[1].y, d4 = pts[2].x, d5 = pts[2].y

    sub.w %d0,%d4               | d4 = pts[2].x - pts[0].x
    sub.w %d1,%d3               | d3 = pts[1].y - pts[0].y
    muls.w %d4,%d3

    sub.w %d0,%d2               | d2 = pts[1].x - pts[0].x
    sub.w %d1,%d5               | d5 = pts[2].y - pts[0].y
    muls.w %d5,%d2

    cmp.l %d3,%d2               | culling test
	jge .L110                   | if (d4 * d3 < d5 * d2)

	moveq #1,%d0                |     return 1;
	movm.l (%sp)+,%d2-%d5
	rts

.L110:
    movm.l %d6-%d7/%a2-%a6,-(%sp)

    move.l #0x00A00000,%d0
    move.l %d0,-(%sp)           | minYL = BMP_HEIGHT; maxYL = 0
    move %sp,%a2                | a2 = &(minYL/maxYL)
    move.l %d0,-(%sp)           | minYR = BMP_HEIGHT; maxYR = 0
    move %sp,%a3                | a3 = &(minYR/maxYR)

.loop4:
  |  jmp .loop4

    move.l LeftPoly,%a4         | a4 = LeftPoly
    move.l RightPoly,%a5        | a5 = RightPoly

	move.l (%a1),-(%sp)         | save first point
	move.l %d0,-(%sp)           | reserve space for last point

	move.w 70(%sp),%d0          | d0 = num vertex
	add.w  %d0,%d0
	add.w  %d0,%d0              | d0 = num vertex * 4

.vertex_base:
    move.l (.vertex_table-.vertex_base)-2(%pc,%d0.w),%a0
    jmp (%a0)

    .align 4
.vertex_table:
    .long .vertex_0
    .long .vertex_1
    .long .vertex_2
    .long .vertex_3
    .long .vertex_4
    .long .vertex_5
    .long .vertex_6
    .long .vertex_7
    .long .vertex_8

.vertex_8:
	jsr calculatePolyEdge
	addq.l #4,%a1

.vertex_7:
	jsr calculatePolyEdge
	addq.l #4,%a1

.vertex_6:
	jsr calculatePolyEdge
	addq.l #4,%a1

.vertex_5:
	jsr calculatePolyEdge
	addq.l #4,%a1

.vertex_4:
	jsr calculatePolyEdge
	addq.l #4,%a1

.vertex_3:
.vertex_2:          | cannot have less than 3 vertex
.vertex_1:
.vertex_0:
	jsr calculatePolyEdge
	addq.l #4,%a1
	jsr calculatePolyEdge

	move.l 4(%a1),(%sp)        | save last point
	move.l %sp,%a1
	jsr calculatePolyEdge

.loo2:
 |    jmp .loo2

	move.w (%a2)+,%d1           | d1 = minYL
	move.w (%a3)+,%d0           | d0 = minYR
	cmp.w %d0,%d1
	jle .L112

	move.w %d1,%d0              | d0 = minY

.L112:
	move.w (%a2),%d1            | d1 = maxYL
	move.w (%a3),%d2            | d2 = maxYR
	cmp.w %d1,%d2
	jge .L113

	move.w %d2,%d1              | d1 = maxY

.L113:
    lea 16(%sp),%sp             | free reserved space
	move.b 59(%sp),%d5          | d5 = col

	sub.w %d0,%d1               | d1 = len = maxY - minY
	jge .L169

    moveq #0,%d0
	movm.l (%sp)+,%d6-%d7/%a2-%a6
	movm.l (%sp)+,%d2-%d5
	rts

.L169:
    moveq  #15,%d6              | d6 = col low nibble mask
    moveq  #-16,%d7             | d7 = col high nibble mask

    move.w %d5,%d4
    lsl.w  #8,%d4
    move.b %d5,%d4
    move.w %d4,%d5
    swap   %d5
    move.w %d4,%d5              | d5 = col extended to 32 bits

    move.l %d5,%a3              | a3 = col extended to 32 bits (save)
    rol.l  #4,%d5               | d5 = col with exchanged nibble
    btst   #0,%d0               | odd line ?
    jeq    .L168

    exg    %d5,%a3              | use according color scheme

.L168:
    add.w  %d0,%d0              | d0 = 2 * minY

	add.w  %d0,%a4              | a4 = left = &LeftPoly[minY]       (16 bits is ok as LeftPoly is in ram)
	add.w  %d0,%a5              | a5 = right = &RightPoly[minY]     (16 bits is ok as RightPoly is in ram)
	move.l bmp_buffer_write,%a2
	lsl.w  #6,%d0
	add.w  %d0,%a2              | a2 = buf = &bmp_buffer_write[minY * BMP_PITCH]    (16 bits is ok as bmp_buffer_write is in ram)

	move.w #255,%a6             | a6 = BMP_WIDTH - 1
	subq.w #1,%d1               | d1 = remaining y

.L180:
	move.w (%a4)+,%d2           | d2 = x1 = *left++
	move.w (%a5)+,%d3           | d3 = x2 = *rigth++

	cmp.w  %d2,%d3              | if ((x1 <= x2) && (x1 < BMP_WIDTH) && (x2 >= 0))
	jlt    .L175                | {
	cmp.w  %a6,%d2
	jgt    .L175
	tst.w  %d3
	jlt    .L175

    tst.w  %d2                  |   if (x1 < 0)
    jpl    .L162

    movq   #0,%d2               |     x1 = 0

.L162:
	cmp.w  %a6,%d3              |   if (x2 >= BMP_WIDTH)
	jle    .L177

	move.w %a6,%d3              |     x2 = BMP_WIDTH - 1;

.L177:
	asr.w  #1,%d3               |   if (!(x2 & 1))
	jcs    .L179                |   {

	move.b (%a2,%d3.w),%d0
	and.b  %d6,%d0
	move.b %d5,%d4
	and.b  %d7,%d4
	or.b   %d4,%d0
	move.b %d0,(%a2,%d3.w)      |     buf[x2 >> 1] = (buf[x2 >> 1] & 0x0F) | (color & 0xF0)
	subq.w #1,%d3               |     x2--;
                                |   }
.L179:
	asr.w  #1,%d2               |   a1 = dst = &buf[x1 >> 1]
    lea    (%a2,%d2.w),%a1      |   if (x1 & 1)
	jcc    .L178                |   {

	move.b (%a1),%d0
	and.b  %d7,%d0
	move.b %d5,%d4
	and.b  %d6,%d4
	or.b   %d4,%d0
	move.b %d0,(%a1)+           |     buf[x1 >> 1] = (buf[x1 >> 1] & 0xF0) | (color & 0x0F)
	addq.w #1,%d2               |     x1++;
                                |   }
.L178:
	sub.w  %d2,%d3              |   d3 = width - 1 = (x2 - x1) >> 1
	addq.w #1,%d3               |   d3 = width
	jeq    .L175

	move.l %a1,%d0
	btst   #0,%d0               |   dst is byte aligned ?
	jeq    .L163

	move.b %d5,(%a1)+           |   align to word
	subq.w #1,%d3

.L163:
	add.w %d3,%d3
	add.w %d3,%d3               |   d3 = (width & 0x1F) << 2

.fill_base:
    move.l (.fill_table-.fill_base)-2(%pc,%d3.w),%a0
    jmp (%a0)

    .align 4
.fill_table:
    .long .L175
    .long .fill_01
    .long .fill_02
    .long .fill_03
    .long .fill_04
    .long .fill_05
    .long .fill_06
    .long .fill_07
    .long .fill_08
    .long .fill_09
    .long .fill_10
    .long .fill_11
    .long .fill_12
    .long .fill_13
    .long .fill_14
    .long .fill_15
    .long .fill_16
    .long .fill_17
    .long .fill_18
    .long .fill_19
    .long .fill_20
    .long .fill_21
    .long .fill_22
    .long .fill_23
    .long .fill_24
    .long .fill_25
    .long .fill_26
    .long .fill_27
    .long .fill_28
    .long .fill_29
    .long .fill_30
    .long .fill_31
    .long .fill_32
    .long .fill_33
    .long .fill_34
    .long .fill_35
    .long .fill_36
    .long .fill_37
    .long .fill_38
    .long .fill_39
    .long .fill_40
    .long .fill_41
    .long .fill_42
    .long .fill_43
    .long .fill_44
    .long .fill_45
    .long .fill_46
    .long .fill_47
    .long .fill_48
    .long .fill_49
    .long .fill_50
    .long .fill_51
    .long .fill_52
    .long .fill_53
    .long .fill_54
    .long .fill_55
    .long .fill_56
    .long .fill_57
    .long .fill_58
    .long .fill_59
    .long .fill_60
    .long .fill_61
    .long .fill_62
    .long .fill_63
    .long .fill_64
    .long .fill_65
    .long .fill_66
    .long .fill_67
    .long .fill_68
    .long .fill_69
    .long .fill_70
    .long .fill_71
    .long .fill_72
    .long .fill_73
    .long .fill_74
    .long .fill_75
    .long .fill_76
    .long .fill_77
    .long .fill_78
    .long .fill_79
    .long .fill_80
    .long .fill_81
    .long .fill_82
    .long .fill_83
    .long .fill_84
    .long .fill_85
    .long .fill_86
    .long .fill_87
    .long .fill_88
    .long .fill_89
    .long .fill_90
    .long .fill_91
    .long .fill_92
    .long .fill_93
    .long .fill_94
    .long .fill_95
    .long .fill_96
    .long .fill_97
    .long .fill_98
    .long .fill_99
    .long .fill_100
    .long .fill_101
    .long .fill_102
    .long .fill_103
    .long .fill_104
    .long .fill_105
    .long .fill_106
    .long .fill_107
    .long .fill_108
    .long .fill_109
    .long .fill_110
    .long .fill_111
    .long .fill_112
    .long .fill_113
    .long .fill_114
    .long .fill_115
    .long .fill_116
    .long .fill_117
    .long .fill_118
    .long .fill_119
    .long .fill_120
    .long .fill_121
    .long .fill_122
    .long .fill_123
    .long .fill_124
    .long .fill_125
    .long .fill_126
    .long .fill_127
    .long .fill_128

.fill_128:
	move.l %d5,(%a1)+
.fill_124:
	move.l %d5,(%a1)+
.fill_120:
	move.l %d5,(%a1)+
.fill_116:
	move.l %d5,(%a1)+
.fill_112:
	move.l %d5,(%a1)+
.fill_108:
	move.l %d5,(%a1)+
.fill_104:
	move.l %d5,(%a1)+
.fill_100:
	move.l %d5,(%a1)+
.fill_96:
	move.l %d5,(%a1)+
.fill_92:
	move.l %d5,(%a1)+
.fill_88:
	move.l %d5,(%a1)+
.fill_84:
	move.l %d5,(%a1)+
.fill_80:
	move.l %d5,(%a1)+
.fill_76:
	move.l %d5,(%a1)+
.fill_72:
	move.l %d5,(%a1)+
.fill_68:
	move.l %d5,(%a1)+
.fill_64:
	move.l %d5,(%a1)+
.fill_60:
	move.l %d5,(%a1)+
.fill_56:
	move.l %d5,(%a1)+
.fill_52:
	move.l %d5,(%a1)+
.fill_48:
	move.l %d5,(%a1)+
.fill_44:
	move.l %d5,(%a1)+
.fill_40:
	move.l %d5,(%a1)+
.fill_36:
	move.l %d5,(%a1)+
.fill_32:
	move.l %d5,(%a1)+
.fill_28:
	move.l %d5,(%a1)+
.fill_24:
	move.l %d5,(%a1)+
.fill_20:
	move.l %d5,(%a1)+
.fill_16:
	move.l %d5,(%a1)+
.fill_12:
	move.l %d5,(%a1)+
.fill_08:
	move.l %d5,(%a1)+
.fill_04:
	move.l %d5,(%a1)

	lea 128(%a2),%a2
	exg %d5,%a3                 | exchange color nibble
	dbra %d1,.L180

    moveq #0,%d0
	movm.l (%sp)+,%d6-%d7/%a2-%a6
	movm.l (%sp)+,%d2-%d5
	rts

.fill_126:
	move.l %d5,(%a1)+
.fill_122:
	move.l %d5,(%a1)+
.fill_118:
	move.l %d5,(%a1)+
.fill_114:
	move.l %d5,(%a1)+
.fill_110:
	move.l %d5,(%a1)+
.fill_106:
	move.l %d5,(%a1)+
.fill_102:
	move.l %d5,(%a1)+
.fill_98:
	move.l %d5,(%a1)+
.fill_94:
	move.l %d5,(%a1)+
.fill_90:
	move.l %d5,(%a1)+
.fill_86:
	move.l %d5,(%a1)+
.fill_82:
	move.l %d5,(%a1)+
.fill_78:
	move.l %d5,(%a1)+
.fill_74:
	move.l %d5,(%a1)+
.fill_70:
	move.l %d5,(%a1)+
.fill_66:
	move.l %d5,(%a1)+
.fill_62:
	move.l %d5,(%a1)+
.fill_58:
	move.l %d5,(%a1)+
.fill_54:
	move.l %d5,(%a1)+
.fill_50:
	move.l %d5,(%a1)+
.fill_46:
	move.l %d5,(%a1)+
.fill_42:
	move.l %d5,(%a1)+
.fill_38:
	move.l %d5,(%a1)+
.fill_34:
	move.l %d5,(%a1)+
.fill_30:
	move.l %d5,(%a1)+
.fill_26:
	move.l %d5,(%a1)+
.fill_22:
	move.l %d5,(%a1)+
.fill_18:
	move.l %d5,(%a1)+
.fill_14:
	move.l %d5,(%a1)+
.fill_10:
	move.l %d5,(%a1)+
.fill_06:
	move.l %d5,(%a1)+
.fill_02:
	move.w %d5,(%a1)

	lea 128(%a2),%a2
	exg %d5,%a3                 | exchange color nibble
	dbra %d1,.L180

    moveq #0,%d0
	movm.l (%sp)+,%d6-%d7/%a2-%a6
	movm.l (%sp)+,%d2-%d5
	rts

.fill_125:
	move.l %d5,(%a1)+
.fill_121:
	move.l %d5,(%a1)+
.fill_117:
	move.l %d5,(%a1)+
.fill_113:
	move.l %d5,(%a1)+
.fill_109:
	move.l %d5,(%a1)+
.fill_105:
	move.l %d5,(%a1)+
.fill_101:
	move.l %d5,(%a1)+
.fill_97:
	move.l %d5,(%a1)+
.fill_93:
	move.l %d5,(%a1)+
.fill_89:
	move.l %d5,(%a1)+
.fill_85:
	move.l %d5,(%a1)+
.fill_81:
	move.l %d5,(%a1)+
.fill_77:
	move.l %d5,(%a1)+
.fill_73:
	move.l %d5,(%a1)+
.fill_69:
	move.l %d5,(%a1)+
.fill_65:
	move.l %d5,(%a1)+
.fill_61:
	move.l %d5,(%a1)+
.fill_57:
	move.l %d5,(%a1)+
.fill_53:
	move.l %d5,(%a1)+
.fill_49:
	move.l %d5,(%a1)+
.fill_45:
	move.l %d5,(%a1)+
.fill_41:
	move.l %d5,(%a1)+
.fill_37:
	move.l %d5,(%a1)+
.fill_33:
	move.l %d5,(%a1)+
.fill_29:
	move.l %d5,(%a1)+
.fill_25:
	move.l %d5,(%a1)+
.fill_21:
	move.l %d5,(%a1)+
.fill_17:
	move.l %d5,(%a1)+
.fill_13:
	move.l %d5,(%a1)+
.fill_09:
	move.l %d5,(%a1)+
.fill_05:
	move.l %d5,(%a1)+
	move.b %d5,(%a1)

	lea 128(%a2),%a2
	exg %d5,%a3                 | exchange color nibble
	dbra %d1,.L180

    moveq #0,%d0
	movm.l (%sp)+,%d6-%d7/%a2-%a6
	movm.l (%sp)+,%d2-%d5
	rts

.fill_127:
	move.l %d5,(%a1)+
.fill_123:
	move.l %d5,(%a1)+
.fill_119:
	move.l %d5,(%a1)+
.fill_115:
	move.l %d5,(%a1)+
.fill_111:
	move.l %d5,(%a1)+
.fill_107:
	move.l %d5,(%a1)+
.fill_103:
	move.l %d5,(%a1)+
.fill_99:
	move.l %d5,(%a1)+
.fill_95:
	move.l %d5,(%a1)+
.fill_91:
	move.l %d5,(%a1)+
.fill_87:
	move.l %d5,(%a1)+
.fill_83:
	move.l %d5,(%a1)+
.fill_79:
	move.l %d5,(%a1)+
.fill_75:
	move.l %d5,(%a1)+
.fill_71:
	move.l %d5,(%a1)+
.fill_67:
	move.l %d5,(%a1)+
.fill_63:
	move.l %d5,(%a1)+
.fill_59:
	move.l %d5,(%a1)+
.fill_55:
	move.l %d5,(%a1)+
.fill_51:
	move.l %d5,(%a1)+
.fill_47:
	move.l %d5,(%a1)+
.fill_43:
	move.l %d5,(%a1)+
.fill_39:
	move.l %d5,(%a1)+
.fill_35:
	move.l %d5,(%a1)+
.fill_31:
	move.l %d5,(%a1)+
.fill_27:
	move.l %d5,(%a1)+
.fill_23:
	move.l %d5,(%a1)+
.fill_19:
	move.l %d5,(%a1)+
.fill_15:
	move.l %d5,(%a1)+
.fill_11:
	move.l %d5,(%a1)+
.fill_07:
	move.l %d5,(%a1)+
.fill_03:
	move.w %d5,(%a1)+
.fill_01:
	move.b %d5,(%a1)

.L175:
	lea 128(%a2),%a2
	exg %d5,%a3                 | exchange color nibble
	dbra %d1,.L180

    moveq #0,%d0
	movm.l (%sp)+,%d6-%d7/%a2-%a6
	movm.l (%sp)+,%d2-%d5
	rts
