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
    | IN:
    | a1 = pts
    | a2 = &minYL
    | a3 = &minYR
    | a4 = leftEdge
    | a5 = rightEdge
    |
    | OUT:
    | a1 = pts
    | a2 = &minYL
    | a3 = &minYR
    | a4 = leftEdge
    | a5 = rightEdge
    | a6 = ???
calculatePolyEdge:
	movem.w (%a1),%d2-%d5       | d2 = x1, d3 = y1, d4 = x2, d5 = y2

	move.w %d5,%d7
	sub.w %d3,%d7               | d7 = dy = y2 - y1;

	jeq .L61                    | if (dy == 0) return;

	move.w %d4,%d6
	sub.w %d2,%d6               | d6 = dx = x2 - x1;

	move.w #159,%d0             | d0 = BMP_HEIGHT - 1

	tst.w %d3                   | if (y1 < 0)
	jpl .L68                    | {

	tst.w %d5                   |     if (y2 < 0)
	jmi .L61                    |         return;

    muls.w %d6,%d3
    divs.w %d7,%d3
	sub.w %d3,%d2               |     x1 -= (y1 * dx) / dy;
	moveq #0,%d3                |     y1 = 0;

	move.w %d4,%d6
	sub.w %d2,%d6               |     d6 = dx = x2 - x1;
	move.w %d5,%d7              |     d7 = dy = y2 - 0;

	jra .L71                    |     goto L71;
                                | }
.L68:
	cmp.w %d0,%d3               | if (y1 >= BMP_HEIGHT)
	jle .L69                    | {

	cmp.w %d0,%d5               |     if (y2 >= BMP_HEIGHT)
	jgt .L61                    |         return;

    sub.w %d0,%d3
	muls.w %d6,%d3
	divs.w %d7,%d3
	sub.w %d3,%d2               |     x1 -= ((y1 - (BMP_HEIGHT - 1)) * dx) / dy;
	move.w %d0,%d3              |     y1 = BMP_HEIGHT - 1;

	move.w %d4,%d6
	sub.w %d2,%d6               |     d6 = dx = x2 - x1;
	move.w %d5,%d7
	sub.w %d3,%d7               |     d7 = dy = y2 - y1;

	tst.w %d5                   |     if (y2 < 0)
	jpl .L72                    |     {

	muls.w %d6,%d5
	divs.w %d7,%d5
	sub.w %d5,%d4               |         x2 -= (y2 * dx) / dy;
	moveq #0,%d5                |         y2 = 0;

	move.w %d4,%d6
	sub.w %d2,%d6               |         d6 = dx = x2 - x1;
	move.w %d5,%d7
	sub.w %d3,%d7               |         d7 = dy = y2 - y1;

	jra .L72                    |     }
                                |     goto L72;
.L69:	                        | }
	tst.w %d5                   | if (y2 < 0)
	jpl .L71                    | {

	muls.w %d6,%d5
	divs.w %d7,%d5
	sub.w %d5,%d4               |     x2 -= (y2 * dx) / dy;
	moveq #0,%d5                |     y2 = 0;

	move.w %d4,%d6
	sub.w %d2,%d6               |     d6 = dx = x2 - x1;
	move.w %d5,%d7
	sub.w %d3,%d7               |     d7 = dy = y2 - y1;

	jra .L72                    |     goto L72;
                                | }
.L71:
	cmp.w %d0,%d5               | if (y2 >= BMP_HEIGHT)
	jle .L72                    | {

    sub.w %d0,%d5
	muls.w %d6,%d5
	divs.w %d7,%d5
	sub.w %d5,%d4               |     x2 -= (y2 - (BMP_HEIGHT - 1)) * stepx;
	move.w %d0,%d5              |     y2 = BMP_HEIGHT - 1;

	move.w %d4,%d6
	sub.w %d2,%d6               |     d6 = dx = x2 - x1;
	move.w %d5,%d7
	sub.w %d3,%d7               |     d7 = dy = y2 - y1;
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

	muls.w %d7,%d2
    divs.w %d6,%d2
    sub.w %d2,%d3               |         y1 -= (x1 * dy (>0)) / dx (>0);
    moveq #0,%d0                |         d0 = pre fill len = 0
    moveq #0,%d2                |         x1 = 0

	move.w %d4,%d6
	sub.w %d2,%d6               |         d6 = dx = x2 - x1;
	move.w %d5,%d7
	sub.w %d3,%d7               |         d7 = dy = y2 - y1;

    move.w #255,%d1             |         d1 = BMP_WIDTH - 1

	cmp.w %d1,%d4               |         if (x2 >= BMP_WIDTH)
	jle .L77_4                  |         {

    sub.w %d1,%d4
	muls.w %d7,%d4
	divs.w %d6,%d4              |             d1 = post fill len = ((x2 - (BMP_WIDTH - 1)) * dy (>0)) / dx (>0);
	sub.w %d4,%d5               |             y2 -= ((x2 - (BMP_WIDTH - 1)) * dy (>0)) / dx (>0);
    exg %d1,%d4                 |             x2 = BMP_WIDTH - 1

	move.w %d4,%d6
	sub.w %d2,%d6               |             d6 = dx = x2 - x1;
	move.w %d5,%d7
	sub.w %d3,%d7               |             d7 = dy = y2 - y1;

    jra .L77                    |         }

.L77_0:                         |     }
    move.w #255,%d0

	cmp.w %d0,%d2               |     else if (x1 >= BMP_WIDTH)
	jle .L77_1                  |     {

	cmp.w %d0,%d4               |         if (x2 >= BMP_WIDTH)
	jle .L77_0_1                |         {

	move.w %d0,%d2              |             x1 = BMP_WIDTH - 1
	move.w %d5,%d0
	sub.w %d3,%d0               |             d0 = pre fill len = dy = y2 - y1
	moveq #0,%d1                |             d1 = post fill len = 0
	jra .L77                    |         }
                                |         else
.L77_0_1:                       |         {
	neg.w %d6                   |             d6 = -dx (>0) = x1 - x2;

    sub.w %d0,%d2
	muls.w %d7,%d2
    divs.w %d6,%d2              |             d0 = pre fill len = ((x1 - (BMP_WIDTH - 1)) * dy (>0)) / -dx (>0);
    add.w %d2,%d3               |             y1 += pre fill len
    exg %d0,%d2                 |             x1 = BMP_WIDTH - 1

	move.w %d4,%d6
	sub.w %d2,%d6               |             d6 = dx = x2 - x1;
	move.w %d5,%d7
	sub.w %d3,%d7               |             d7 = dy = y2 - y1;

	tst.w %d4                   |             if  (x2 < 0)
	jpl .L77_4                  |             {

	neg.w %d6                   |                 d6 = -dx (>0) = x1 - x2;

	muls.w %d7,%d4
	divs.w %d6,%d4
    add.w %d4,%d5               |                 y2 += x2 * stepy (>0)
    moveq #0,%d1                |                 d1 = post fill len = 0
    moveq #0,%d4                |                 x2 = 0

	move.w %d4,%d6
	sub.w %d2,%d6               |                 d6 = dx = x2 - x1;
	move.w %d5,%d7
	sub.w %d3,%d7               |                 d7 = dy = y2 - y1;

    jra .L77                    |             }
                                |         }
.L77_1:                         |     }
	tst.w %d4                   |     else if (x2 < 0)
	jpl .L77_2                  |     {

	neg.w %d6                   |         d6 = -dx (>0) = x1 - x2;

	muls.w %d7,%d4
    divs.w %d6,%d4
    add.w %d4,%d5               |         y2 += (x2 * dy (>0)) / dx (>0);
    moveq #0,%d0                |         d0 = pre fill len = 0
    moveq #0,%d1                |         d1 = post fill len = 0
    moveq #0,%d4                |         x2 = 0

	move.w %d4,%d6
	sub.w %d2,%d6               |         d6 = dx = x2 - x1;
	move.w %d5,%d7
	sub.w %d3,%d7               |         d7 = dy = y2 - y1;

    jra .L77                    |     }

.L77_2:
	cmp.w %d0,%d4               |     else if (x2 >= BMP_WIDTH)
	jle .L77_3                  |     {

    sub.w %d0,%d4
	muls.w %d7,%d4
    divs.w %d6,%d4              |         d4 = post fill len = ((x2 - (BMP_WIDTH - 1)) * dy (>0)) / dx (>0)
    move.w %d4,%d1              |         d1 = d4 = post fill len
    sub.w %d4,%d5               |         y2 -= post fill len
    move.w %d0,%d4              |         x2 = BMP_WIDTH - 1
    moveq #0,%d0                |         d0 = pre fill len = 0

	move.w %d4,%d6
	sub.w %d2,%d6               |         d6 = dx = x2 - x1;
	move.w %d5,%d7
	sub.w %d3,%d7               |         d7 = dy = y2 - y1;

    jra .L77                    |     }
                                |     else
.L77_3:                         |     {
    moveq #0,%d0                |         d0 = pre fill len = 0
.L77_4:
    moveq #0,%d1                |         d1 = post fill len = 0
                                |     }
.L77:
	cmp.w (%a3),%d3             |     if (y1 < minYR)
	jge .L83

	move.w %d3,(%a3)            |         minYR = y1;

.L83:
	cmp.w 2(%a3),%d5            |     if (y2 > maxYR)
	jle .L84

	move.w %d5,2(%a3)           |         maxYR = y2;

.L84:
	ext.l %d6
    lsl.l #7,%d6                |     d6 = dx << 7
    divs.w %d7,%d6              |     d6 = (dx << 7) / dy

    sub.w %d0,%d3               |     y1 -= pre fill len
    add.w %d3,%d3
    lea (%a5,%d3.w),%a0         |     a0 = src = &rightEdge[y1];

    move.w %d2,%d3              |     d3 = x start = x1
    move.w %d4,%d5              |     d5 = x end = x2
	jra .L85                    | }
                                | else      // left edge
.L80:                           | {
    move.w #255,%d0             |     d0 = BMP_WIDTH - 1

	cmp.w %d0,%d2               |     if (x1 >= BMP_WIDTH)
	jle .L76_0                  |     {

	cmp.w %d0,%d4               |         if (x2 >= BMP_WIDTH))
	jgt .L61                    |             return;

	neg.w %d6                   |         d6 = -dx (>0) = x1 - x2;

    sub.w %d0,%d2
	muls.w %d7,%d2
    divs.w %d6,%d2
    add.w %d2,%d3               |         y1 += ((x1 - (BMP_WIDTH - 1)) * dy (<0)) / dx (>0);
    move.w %d0,%d2              |         x1 = BMP_WIDTH - 1
    moveq #0,%d1                |         d1 = post fill len = 0
    moveq #0,%d0                |         d0 = pre fill len = 0

	move.w %d4,%d6
	sub.w %d2,%d6               |         d6 = dx = x2 - x1;
    move.w %d5,%d7
	sub.w %d3,%d7               |         d7 = dy = y2 - y1;

	tst.w %d4                   |         if  (x2 < 0)
	jpl .L76                    |         {

	neg.w %d6                   |             d6 = -dx (>0) = x1 - x2;

	muls.w %d7,%d4
    divs.w %d6,%d4              |             d0 = pre fill len = (x2 * dy (<0)) / dx (>0);
    add.w %d4,%d5               |             y2 += pre fill len
    exg %d4,%d0                 |             x2 = 0

	move.w %d4,%d6
	sub.w %d2,%d6               |             d6 = dx = x2 - x1;
    move.w %d5,%d7
	sub.w %d3,%d7               |             d7 = dy = y2 - y1;

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
	muls.w %d7,%d2
    divs.w %d6,%d2
    move.w %d2,%d1              |             d1 = post fill len = (x1 * dy (<0)) / dx (>0);
    sub.w %d1,%d3               |             y1 -= post fill len
    moveq #0,%d2                |             x1 = 0

	move.w %d4,%d6
	sub.w %d2,%d6               |             d6 = dx = x2 - x1;
    move.w %d5,%d7
	sub.w %d3,%d7               |             d7 = dy = y2 - y1;

	cmp.w %d0,%d4               |             if (x2 >= BMP_WIDTH)
	jle .L76_4                  |             {

    sub.w %d0,%d4
	muls.w %d7,%d4
    divs.w %d6,%d4
    sub.w %d4,%d5               |                 y2 -= (x2 - (BMP_WIDTH - 1)) * dy (<0)) / dx (>0);
    move.w %d0,%d4              |                 x2 = BMP_WIDTH - 1
    moveq #0,%d0                |                 d0 = pre fill len = 0

	move.w %d4,%d6
	sub.w %d2,%d6               |                 d6 = dx = x2 - x1;
    move.w %d5,%d7
	sub.w %d3,%d7               |                 d7 = dy = y2 - y1;

    jra .L76                    |             }
                                |         }
.L76_1:                         |     }
	tst.w %d4                   |     else if (x2 < 0)
	jpl .L76_2                  |     {

    neg.w %d6                   |         d6 = -dx (>0)

	muls.w %d7,%d4
    divs.w %d6,%d4              |         d4 = pre fill len = (x2 * dy (<0)) / dx (>0);
    move.w %d4,%d0              |         d0 = pre fill len
    add.w %d0,%d5               |         y2 += pre fill len
    moveq #0,%d1                |         d1 = post fill len = 0
    moveq #0,%d4                |         x2 = 0

	move.w %d4,%d6
	sub.w %d2,%d6               |         d6 = dx = x2 - x1;
    move.w %d5,%d7
	sub.w %d3,%d7               |         d7 = dy = y2 - y1;

    jra .L76                    |     }

.L76_2:
	cmp.w %d0,%d4               |     else if (x2 >= BMP_WIDTH)
	jle .L76_3                  |     {

    sub.w %d0,%d4
	muls.w %d7,%d4              |         y2 -= ((x2 - (BMP_WIDTH - 1)) * dy (<0)) / dx (>0);
    divs.w %d6,%d4              |         x2 = BMP_WIDTH - 1
    sub.w %d4,%d5               |         d0 = pre fill len = 0
    move.w %d0,%d4              |         d1 = post fill len = 0

	move.w %d4,%d6
	sub.w %d2,%d6               |         d6 = dx = x2 - x1;
    move.w %d5,%d7
	sub.w %d3,%d7               |         d7 = dy = y2 - y1;
                                |     }
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
	ext.l %d6
    lsl.l #7,%d6                |     d6 = dx << 7
    divs.w %d7,%d6              |     d6 = (dx << 7) / dy
    neg.w %d7                   |     d7 = -dy (>0)

    sub.w %d0,%d5               |     y2 -= pre fill len
    add.w %d5,%d5
    lea (%a4,%d5.w),%a0         |     a0 = src = &leftEdge[y2];

    move.w %d4,%d3              |     d3 = x start = x2
    move.w %d2,%d5              |     d5 = x end = x1
                                | }
.L85:
    add.w %d0,%d0
    add.w %d0,%d0               | d0 = d0*4 (for jump table)
    add.w %d1,%d1
    add.w %d1,%d1               | d1 = d1*4 (for jump table)
    add.w %d7,%d7
    add.w %d7,%d7               | d7 = d7*4 (for jump table)

    move.w %d3,%d4
    swap %d3
    move.w %d4,%d3              | d3 = (x << 16) | (x << 0)

.prefill_base:
    move.l (.prefill_table-.prefill_base)-2(%pc,%d0.w),%a6
    jmp (%a6)

.L61:
    rts

    .align 4
.prefill_table:
    .long .prefill_0
    .long .prefill_1
    .long .prefill_2
    .long .prefill_3
    .long .prefill_4
    .long .prefill_5
    .long .prefill_6
    .long .prefill_7
    .long .prefill_8
    .long .prefill_9
    .long .prefill_10
    .long .prefill_11
    .long .prefill_12
    .long .prefill_13
    .long .prefill_14
    .long .prefill_15
    .long .prefill_16
    .long .prefill_17
    .long .prefill_18
    .long .prefill_19
    .long .prefill_20
    .long .prefill_21
    .long .prefill_22
    .long .prefill_23
    .long .prefill_24
    .long .prefill_25
    .long .prefill_26
    .long .prefill_27
    .long .prefill_28
    .long .prefill_29
    .long .prefill_30
    .long .prefill_31
    .long .prefill_32
    .long .prefill_33
    .long .prefill_34
    .long .prefill_35
    .long .prefill_36
    .long .prefill_37
    .long .prefill_38
    .long .prefill_39
    .long .prefill_40
    .long .prefill_41
    .long .prefill_42
    .long .prefill_43
    .long .prefill_44
    .long .prefill_45
    .long .prefill_46
    .long .prefill_47
    .long .prefill_48
    .long .prefill_49
    .long .prefill_50
    .long .prefill_51
    .long .prefill_52
    .long .prefill_53
    .long .prefill_54
    .long .prefill_55
    .long .prefill_56
    .long .prefill_57
    .long .prefill_58
    .long .prefill_59
    .long .prefill_60
    .long .prefill_61
    .long .prefill_62
    .long .prefill_63
    .long .prefill_64
    .long .prefill_65
    .long .prefill_66
    .long .prefill_67
    .long .prefill_68
    .long .prefill_69
    .long .prefill_70
    .long .prefill_71
    .long .prefill_72
    .long .prefill_73
    .long .prefill_74
    .long .prefill_75
    .long .prefill_76
    .long .prefill_77
    .long .prefill_78
    .long .prefill_79
    .long .prefill_80
    .long .prefill_81
    .long .prefill_82
    .long .prefill_83
    .long .prefill_84
    .long .prefill_85
    .long .prefill_86
    .long .prefill_87
    .long .prefill_88
    .long .prefill_89
    .long .prefill_90
    .long .prefill_91
    .long .prefill_92
    .long .prefill_93
    .long .prefill_94
    .long .prefill_95
    .long .prefill_96
    .long .prefill_97
    .long .prefill_98
    .long .prefill_99
    .long .prefill_100
    .long .prefill_101
    .long .prefill_102
    .long .prefill_103
    .long .prefill_104
    .long .prefill_105
    .long .prefill_106
    .long .prefill_107
    .long .prefill_108
    .long .prefill_109
    .long .prefill_110
    .long .prefill_111
    .long .prefill_112
    .long .prefill_113
    .long .prefill_114
    .long .prefill_115
    .long .prefill_116
    .long .prefill_117
    .long .prefill_118
    .long .prefill_119
    .long .prefill_120
    .long .prefill_121
    .long .prefill_122
    .long .prefill_123
    .long .prefill_124
    .long .prefill_125
    .long .prefill_126
    .long .prefill_127
    .long .prefill_128
    .long .prefill_129
    .long .prefill_130
    .long .prefill_131
    .long .prefill_132
    .long .prefill_133
    .long .prefill_134
    .long .prefill_135
    .long .prefill_136
    .long .prefill_137
    .long .prefill_138
    .long .prefill_139
    .long .prefill_140
    .long .prefill_141
    .long .prefill_142
    .long .prefill_143
    .long .prefill_144
    .long .prefill_145
    .long .prefill_146
    .long .prefill_147
    .long .prefill_148
    .long .prefill_149
    .long .prefill_150
    .long .prefill_151
    .long .prefill_152
    .long .prefill_153
    .long .prefill_154
    .long .prefill_155
    .long .prefill_156
    .long .prefill_157
    .long .prefill_158
    .long .prefill_159
    .long .prefill_160

.prefill_160:
	move.l %d3,(%a0)+
.prefill_158:
	move.l %d3,(%a0)+
.prefill_156:
	move.l %d3,(%a0)+
.prefill_154:
	move.l %d3,(%a0)+
.prefill_152:
	move.l %d3,(%a0)+
.prefill_150:
	move.l %d3,(%a0)+
.prefill_148:
	move.l %d3,(%a0)+
.prefill_146:
	move.l %d3,(%a0)+
.prefill_144:
	move.l %d3,(%a0)+
.prefill_142:
	move.l %d3,(%a0)+
.prefill_140:
	move.l %d3,(%a0)+
.prefill_138:
	move.l %d3,(%a0)+
.prefill_136:
	move.l %d3,(%a0)+
.prefill_134:
	move.l %d3,(%a0)+
.prefill_132:
	move.l %d3,(%a0)+
.prefill_130:
	move.l %d3,(%a0)+
.prefill_128:
	move.l %d3,(%a0)+
.prefill_126:
	move.l %d3,(%a0)+
.prefill_124:
	move.l %d3,(%a0)+
.prefill_122:
	move.l %d3,(%a0)+
.prefill_120:
	move.l %d3,(%a0)+
.prefill_118:
	move.l %d3,(%a0)+
.prefill_116:
	move.l %d3,(%a0)+
.prefill_114:
	move.l %d3,(%a0)+
.prefill_112:
	move.l %d3,(%a0)+
.prefill_110:
	move.l %d3,(%a0)+
.prefill_108:
	move.l %d3,(%a0)+
.prefill_106:
	move.l %d3,(%a0)+
.prefill_104:
	move.l %d3,(%a0)+
.prefill_102:
	move.l %d3,(%a0)+
.prefill_100:
	move.l %d3,(%a0)+
.prefill_98:
	move.l %d3,(%a0)+
.prefill_96:
	move.l %d3,(%a0)+
.prefill_94:
	move.l %d3,(%a0)+
.prefill_92:
	move.l %d3,(%a0)+
.prefill_90:
	move.l %d3,(%a0)+
.prefill_88:
	move.l %d3,(%a0)+
.prefill_86:
	move.l %d3,(%a0)+
.prefill_84:
	move.l %d3,(%a0)+
.prefill_82:
	move.l %d3,(%a0)+
.prefill_80:
	move.l %d3,(%a0)+
.prefill_78:
	move.l %d3,(%a0)+
.prefill_76:
	move.l %d3,(%a0)+
.prefill_74:
	move.l %d3,(%a0)+
.prefill_72:
	move.l %d3,(%a0)+
.prefill_70:
	move.l %d3,(%a0)+
.prefill_68:
	move.l %d3,(%a0)+
.prefill_66:
	move.l %d3,(%a0)+
.prefill_64:
	move.l %d3,(%a0)+
.prefill_62:
	move.l %d3,(%a0)+
.prefill_60:
	move.l %d3,(%a0)+
.prefill_58:
	move.l %d3,(%a0)+
.prefill_56:
	move.l %d3,(%a0)+
.prefill_54:
	move.l %d3,(%a0)+
.prefill_52:
	move.l %d3,(%a0)+
.prefill_50:
	move.l %d3,(%a0)+
.prefill_48:
	move.l %d3,(%a0)+
.prefill_46:
	move.l %d3,(%a0)+
.prefill_44:
	move.l %d3,(%a0)+
.prefill_42:
	move.l %d3,(%a0)+
.prefill_40:
	move.l %d3,(%a0)+
.prefill_38:
	move.l %d3,(%a0)+
.prefill_36:
	move.l %d3,(%a0)+
.prefill_34:
	move.l %d3,(%a0)+
.prefill_32:
	move.l %d3,(%a0)+
.prefill_30:
	move.l %d3,(%a0)+
.prefill_28:
	move.l %d3,(%a0)+
.prefill_26:
	move.l %d3,(%a0)+
.prefill_24:
	move.l %d3,(%a0)+
.prefill_22:
	move.l %d3,(%a0)+
.prefill_20:
	move.l %d3,(%a0)+
.prefill_18:
	move.l %d3,(%a0)+
.prefill_16:
	move.l %d3,(%a0)+
.prefill_14:
	move.l %d3,(%a0)+
.prefill_12:
	move.l %d3,(%a0)+
.prefill_10:
	move.l %d3,(%a0)+
.prefill_8:
	move.l %d3,(%a0)+
.prefill_6:
	move.l %d3,(%a0)+
.prefill_4:
	move.l %d3,(%a0)+
.prefill_2:
	move.l %d3,(%a0)+

    jmp .prefill_0


.prefill_159:
	move.l %d3,(%a0)+
.prefill_157:
	move.l %d3,(%a0)+
.prefill_155:
	move.l %d3,(%a0)+
.prefill_153:
	move.l %d3,(%a0)+
.prefill_151:
	move.l %d3,(%a0)+
.prefill_149:
	move.l %d3,(%a0)+
.prefill_147:
	move.l %d3,(%a0)+
.prefill_145:
	move.l %d3,(%a0)+
.prefill_143:
	move.l %d3,(%a0)+
.prefill_141:
	move.l %d3,(%a0)+
.prefill_139:
	move.l %d3,(%a0)+
.prefill_137:
	move.l %d3,(%a0)+
.prefill_135:
	move.l %d3,(%a0)+
.prefill_133:
	move.l %d3,(%a0)+
.prefill_131:
	move.l %d3,(%a0)+
.prefill_129:
	move.l %d3,(%a0)+
.prefill_127:
	move.l %d3,(%a0)+
.prefill_125:
	move.l %d3,(%a0)+
.prefill_123:
	move.l %d3,(%a0)+
.prefill_121:
	move.l %d3,(%a0)+
.prefill_119:
	move.l %d3,(%a0)+
.prefill_117:
	move.l %d3,(%a0)+
.prefill_115:
	move.l %d3,(%a0)+
.prefill_113:
	move.l %d3,(%a0)+
.prefill_111:
	move.l %d3,(%a0)+
.prefill_109:
	move.l %d3,(%a0)+
.prefill_107:
	move.l %d3,(%a0)+
.prefill_105:
	move.l %d3,(%a0)+
.prefill_103:
	move.l %d3,(%a0)+
.prefill_101:
	move.l %d3,(%a0)+
.prefill_99:
	move.l %d3,(%a0)+
.prefill_97:
	move.l %d3,(%a0)+
.prefill_95:
	move.l %d3,(%a0)+
.prefill_93:
	move.l %d3,(%a0)+
.prefill_91:
	move.l %d3,(%a0)+
.prefill_89:
	move.l %d3,(%a0)+
.prefill_87:
	move.l %d3,(%a0)+
.prefill_85:
	move.l %d3,(%a0)+
.prefill_83:
	move.l %d3,(%a0)+
.prefill_81:
	move.l %d3,(%a0)+
.prefill_79:
	move.l %d3,(%a0)+
.prefill_77:
	move.l %d3,(%a0)+
.prefill_75:
	move.l %d3,(%a0)+
.prefill_73:
	move.l %d3,(%a0)+
.prefill_71:
	move.l %d3,(%a0)+
.prefill_69:
	move.l %d3,(%a0)+
.prefill_67:
	move.l %d3,(%a0)+
.prefill_65:
	move.l %d3,(%a0)+
.prefill_63:
	move.l %d3,(%a0)+
.prefill_61:
	move.l %d3,(%a0)+
.prefill_59:
	move.l %d3,(%a0)+
.prefill_57:
	move.l %d3,(%a0)+
.prefill_55:
	move.l %d3,(%a0)+
.prefill_53:
	move.l %d3,(%a0)+
.prefill_51:
	move.l %d3,(%a0)+
.prefill_49:
	move.l %d3,(%a0)+
.prefill_47:
	move.l %d3,(%a0)+
.prefill_45:
	move.l %d3,(%a0)+
.prefill_43:
	move.l %d3,(%a0)+
.prefill_41:
	move.l %d3,(%a0)+
.prefill_39:
	move.l %d3,(%a0)+
.prefill_37:
	move.l %d3,(%a0)+
.prefill_35:
	move.l %d3,(%a0)+
.prefill_33:
	move.l %d3,(%a0)+
.prefill_31:
	move.l %d3,(%a0)+
.prefill_29:
	move.l %d3,(%a0)+
.prefill_27:
	move.l %d3,(%a0)+
.prefill_25:
	move.l %d3,(%a0)+
.prefill_23:
	move.l %d3,(%a0)+
.prefill_21:
	move.l %d3,(%a0)+
.prefill_19:
	move.l %d3,(%a0)+
.prefill_17:
	move.l %d3,(%a0)+
.prefill_15:
	move.l %d3,(%a0)+
.prefill_13:
	move.l %d3,(%a0)+
.prefill_11:
	move.l %d3,(%a0)+
.prefill_9:
	move.l %d3,(%a0)+
.prefill_7:
	move.l %d3,(%a0)+
.prefill_5:
	move.l %d3,(%a0)+
.prefill_3:
	move.l %d3,(%a0)+
.prefill_1:
	move.w %d3,(%a0)+

.prefill_0:
	ext.l %d6                   | d6 = (dx << 7) / dy
	ror.l #8,%d6                | d6 = step >> 1 (32 bits fixed point)
	ext.l %d3                   | d3 = x (32 bits fixed point)
	add.l %d6,%d3               | d3 = x + (step >> 1)
	rol.l #1,%d6                | d6 = step (32 bits fixed point)

	movq #0,%d2                 | d2 = 0
	move %d2,%ccr               | clear X flag

.normfill_base2:
    move.l (.normfill_table-.normfill_base2)-2(%pc,%d7.w),%a6
    jmp (%a6)

    .align 4
.normfill_table:
    .long .normfill_0
    .long .normfill_1
    .long .normfill_2
    .long .normfill_3
    .long .normfill_4
    .long .normfill_5
    .long .normfill_6
    .long .normfill_7
    .long .normfill_8
    .long .normfill_9
    .long .normfill_10
    .long .normfill_11
    .long .normfill_12
    .long .normfill_13
    .long .normfill_14
    .long .normfill_15
    .long .normfill_16
    .long .normfill_17
    .long .normfill_18
    .long .normfill_19
    .long .normfill_20
    .long .normfill_21
    .long .normfill_22
    .long .normfill_23
    .long .normfill_24
    .long .normfill_25
    .long .normfill_26
    .long .normfill_27
    .long .normfill_28
    .long .normfill_29
    .long .normfill_30
    .long .normfill_31
    .long .normfill_32
    .long .normfill_33
    .long .normfill_34
    .long .normfill_35
    .long .normfill_36
    .long .normfill_37
    .long .normfill_38
    .long .normfill_39
    .long .normfill_40
    .long .normfill_41
    .long .normfill_42
    .long .normfill_43
    .long .normfill_44
    .long .normfill_45
    .long .normfill_46
    .long .normfill_47
    .long .normfill_48
    .long .normfill_49
    .long .normfill_50
    .long .normfill_51
    .long .normfill_52
    .long .normfill_53
    .long .normfill_54
    .long .normfill_55
    .long .normfill_56
    .long .normfill_57
    .long .normfill_58
    .long .normfill_59
    .long .normfill_60
    .long .normfill_61
    .long .normfill_62
    .long .normfill_63
    .long .normfill_64
    .long .normfill_65
    .long .normfill_66
    .long .normfill_67
    .long .normfill_68
    .long .normfill_69
    .long .normfill_70
    .long .normfill_71
    .long .normfill_72
    .long .normfill_73
    .long .normfill_74
    .long .normfill_75
    .long .normfill_76
    .long .normfill_77
    .long .normfill_78
    .long .normfill_79
    .long .normfill_80
    .long .normfill_81
    .long .normfill_82
    .long .normfill_83
    .long .normfill_84
    .long .normfill_85
    .long .normfill_86
    .long .normfill_87
    .long .normfill_88
    .long .normfill_89
    .long .normfill_90
    .long .normfill_91
    .long .normfill_92
    .long .normfill_93
    .long .normfill_94
    .long .normfill_95
    .long .normfill_96
    .long .normfill_97
    .long .normfill_98
    .long .normfill_99
    .long .normfill_100
    .long .normfill_101
    .long .normfill_102
    .long .normfill_103
    .long .normfill_104
    .long .normfill_105
    .long .normfill_106
    .long .normfill_107
    .long .normfill_108
    .long .normfill_109
    .long .normfill_110
    .long .normfill_111
    .long .normfill_112
    .long .normfill_113
    .long .normfill_114
    .long .normfill_115
    .long .normfill_116
    .long .normfill_117
    .long .normfill_118
    .long .normfill_119
    .long .normfill_120
    .long .normfill_121
    .long .normfill_122
    .long .normfill_123
    .long .normfill_124
    .long .normfill_125
    .long .normfill_126
    .long .normfill_127
    .long .normfill_128
    .long .normfill_129
    .long .normfill_130
    .long .normfill_131
    .long .normfill_132
    .long .normfill_133
    .long .normfill_134
    .long .normfill_135
    .long .normfill_136
    .long .normfill_137
    .long .normfill_138
    .long .normfill_139
    .long .normfill_140
    .long .normfill_141
    .long .normfill_142
    .long .normfill_143
    .long .normfill_144
    .long .normfill_145
    .long .normfill_146
    .long .normfill_147
    .long .normfill_148
    .long .normfill_149
    .long .normfill_150
    .long .normfill_151
    .long .normfill_152
    .long .normfill_153
    .long .normfill_154
    .long .normfill_155
    .long .normfill_156
    .long .normfill_157
    .long .normfill_158
    .long .normfill_159
    .long .normfill_160

.normfill_160:
	move.w %d3,(%a0)+           | *src++ = fix16ToInt(x);
	addx.l %d6,%d3              |  x += step;
.normfill_159:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_158:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_157:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_156:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_155:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_154:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_153:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_152:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_151:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_150:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_149:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_148:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_147:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_146:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_145:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_144:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_143:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_142:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_141:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_140:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_139:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_138:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_137:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_136:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_135:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_134:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_133:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_132:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_131:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_130:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_129:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_128:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_127:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_126:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_125:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_124:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_123:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_122:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_121:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_120:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_119:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_118:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_117:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_116:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_115:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_114:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_113:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_112:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_111:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_110:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_109:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_108:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_107:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_106:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_105:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_104:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_103:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_102:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_101:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_100:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_99:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_98:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_97:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_96:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_95:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_94:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_93:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_92:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_91:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_90:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_89:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_88:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_87:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_86:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_85:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_84:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_83:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_82:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_81:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_80:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_79:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_78:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_77:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_76:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_75:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_74:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_73:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_72:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_71:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_70:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_69:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_68:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_67:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_66:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_65:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_64:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_63:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_62:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_61:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_60:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_59:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_58:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_57:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_56:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_55:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_54:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_53:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_52:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_51:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_50:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_49:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_48:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_47:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_46:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_45:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_44:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_43:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_42:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_41:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_40:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_39:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_38:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_37:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_36:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_35:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_34:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_33:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_32:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_31:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_30:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_29:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_28:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_27:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_26:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_25:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_24:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_23:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_22:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_21:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_20:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_19:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_18:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_17:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_16:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_15:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_14:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_13:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_12:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_11:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_10:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_9:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_8:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_7:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_6:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_5:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_4:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_3:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_2:
	move.w %d3,(%a0)+
	addx.l %d6,%d3
.normfill_1:
	move.w %d3,(%a0)+

.normfill_0:
    move.w %d5,%d4
    swap %d5
    move.w %d4,%d5              | d5 = (x_end << 16) | (x_end << 0)

.postfill_base:
    move.l (.postfill_table-.postfill_base)-2(%pc,%d1.w),%a6
    jmp (%a6)

    .align 4
.postfill_table:
    .long .postfill_0
    .long .postfill_1
    .long .postfill_2
    .long .postfill_3
    .long .postfill_4
    .long .postfill_5
    .long .postfill_6
    .long .postfill_7
    .long .postfill_8
    .long .postfill_9
    .long .postfill_10
    .long .postfill_11
    .long .postfill_12
    .long .postfill_13
    .long .postfill_14
    .long .postfill_15
    .long .postfill_16
    .long .postfill_17
    .long .postfill_18
    .long .postfill_19
    .long .postfill_20
    .long .postfill_21
    .long .postfill_22
    .long .postfill_23
    .long .postfill_24
    .long .postfill_25
    .long .postfill_26
    .long .postfill_27
    .long .postfill_28
    .long .postfill_29
    .long .postfill_30
    .long .postfill_31
    .long .postfill_32
    .long .postfill_33
    .long .postfill_34
    .long .postfill_35
    .long .postfill_36
    .long .postfill_37
    .long .postfill_38
    .long .postfill_39
    .long .postfill_40
    .long .postfill_41
    .long .postfill_42
    .long .postfill_43
    .long .postfill_44
    .long .postfill_45
    .long .postfill_46
    .long .postfill_47
    .long .postfill_48
    .long .postfill_49
    .long .postfill_50
    .long .postfill_51
    .long .postfill_52
    .long .postfill_53
    .long .postfill_54
    .long .postfill_55
    .long .postfill_56
    .long .postfill_57
    .long .postfill_58
    .long .postfill_59
    .long .postfill_60
    .long .postfill_61
    .long .postfill_62
    .long .postfill_63
    .long .postfill_64
    .long .postfill_65
    .long .postfill_66
    .long .postfill_67
    .long .postfill_68
    .long .postfill_69
    .long .postfill_70
    .long .postfill_71
    .long .postfill_72
    .long .postfill_73
    .long .postfill_74
    .long .postfill_75
    .long .postfill_76
    .long .postfill_77
    .long .postfill_78
    .long .postfill_79
    .long .postfill_80
    .long .postfill_81
    .long .postfill_82
    .long .postfill_83
    .long .postfill_84
    .long .postfill_85
    .long .postfill_86
    .long .postfill_87
    .long .postfill_88
    .long .postfill_89
    .long .postfill_90
    .long .postfill_91
    .long .postfill_92
    .long .postfill_93
    .long .postfill_94
    .long .postfill_95
    .long .postfill_96
    .long .postfill_97
    .long .postfill_98
    .long .postfill_99
    .long .postfill_100
    .long .postfill_101
    .long .postfill_102
    .long .postfill_103
    .long .postfill_104
    .long .postfill_105
    .long .postfill_106
    .long .postfill_107
    .long .postfill_108
    .long .postfill_109
    .long .postfill_110
    .long .postfill_111
    .long .postfill_112
    .long .postfill_113
    .long .postfill_114
    .long .postfill_115
    .long .postfill_116
    .long .postfill_117
    .long .postfill_118
    .long .postfill_119
    .long .postfill_120
    .long .postfill_121
    .long .postfill_122
    .long .postfill_123
    .long .postfill_124
    .long .postfill_125
    .long .postfill_126
    .long .postfill_127
    .long .postfill_128
    .long .postfill_129
    .long .postfill_130
    .long .postfill_131
    .long .postfill_132
    .long .postfill_133
    .long .postfill_134
    .long .postfill_135
    .long .postfill_136
    .long .postfill_137
    .long .postfill_138
    .long .postfill_139
    .long .postfill_140
    .long .postfill_141
    .long .postfill_142
    .long .postfill_143
    .long .postfill_144
    .long .postfill_145
    .long .postfill_146
    .long .postfill_147
    .long .postfill_148
    .long .postfill_149
    .long .postfill_150
    .long .postfill_151
    .long .postfill_152
    .long .postfill_153
    .long .postfill_154
    .long .postfill_155
    .long .postfill_156
    .long .postfill_157
    .long .postfill_158
    .long .postfill_159
    .long .postfill_160

.postfill_160:
	move.l %d5,(%a0)+
.postfill_158:
	move.l %d5,(%a0)+
.postfill_156:
	move.l %d5,(%a0)+
.postfill_154:
	move.l %d5,(%a0)+
.postfill_152:
	move.l %d5,(%a0)+
.postfill_150:
	move.l %d5,(%a0)+
.postfill_148:
	move.l %d5,(%a0)+
.postfill_146:
	move.l %d5,(%a0)+
.postfill_144:
	move.l %d5,(%a0)+
.postfill_142:
	move.l %d5,(%a0)+
.postfill_140:
	move.l %d5,(%a0)+
.postfill_138:
	move.l %d5,(%a0)+
.postfill_136:
	move.l %d5,(%a0)+
.postfill_134:
	move.l %d5,(%a0)+
.postfill_132:
	move.l %d5,(%a0)+
.postfill_130:
	move.l %d5,(%a0)+
.postfill_128:
	move.l %d5,(%a0)+
.postfill_126:
	move.l %d5,(%a0)+
.postfill_124:
	move.l %d5,(%a0)+
.postfill_122:
	move.l %d5,(%a0)+
.postfill_120:
	move.l %d5,(%a0)+
.postfill_118:
	move.l %d5,(%a0)+
.postfill_116:
	move.l %d5,(%a0)+
.postfill_114:
	move.l %d5,(%a0)+
.postfill_112:
	move.l %d5,(%a0)+
.postfill_110:
	move.l %d5,(%a0)+
.postfill_108:
	move.l %d5,(%a0)+
.postfill_106:
	move.l %d5,(%a0)+
.postfill_104:
	move.l %d5,(%a0)+
.postfill_102:
	move.l %d5,(%a0)+
.postfill_100:
	move.l %d5,(%a0)+
.postfill_98:
	move.l %d5,(%a0)+
.postfill_96:
	move.l %d5,(%a0)+
.postfill_94:
	move.l %d5,(%a0)+
.postfill_92:
	move.l %d5,(%a0)+
.postfill_90:
	move.l %d5,(%a0)+
.postfill_88:
	move.l %d5,(%a0)+
.postfill_86:
	move.l %d5,(%a0)+
.postfill_84:
	move.l %d5,(%a0)+
.postfill_82:
	move.l %d5,(%a0)+
.postfill_80:
	move.l %d5,(%a0)+
.postfill_78:
	move.l %d5,(%a0)+
.postfill_76:
	move.l %d5,(%a0)+
.postfill_74:
	move.l %d5,(%a0)+
.postfill_72:
	move.l %d5,(%a0)+
.postfill_70:
	move.l %d5,(%a0)+
.postfill_68:
	move.l %d5,(%a0)+
.postfill_66:
	move.l %d5,(%a0)+
.postfill_64:
	move.l %d5,(%a0)+
.postfill_62:
	move.l %d5,(%a0)+
.postfill_60:
	move.l %d5,(%a0)+
.postfill_58:
	move.l %d5,(%a0)+
.postfill_56:
	move.l %d5,(%a0)+
.postfill_54:
	move.l %d5,(%a0)+
.postfill_52:
	move.l %d5,(%a0)+
.postfill_50:
	move.l %d5,(%a0)+
.postfill_48:
	move.l %d5,(%a0)+
.postfill_46:
	move.l %d5,(%a0)+
.postfill_44:
	move.l %d5,(%a0)+
.postfill_42:
	move.l %d5,(%a0)+
.postfill_40:
	move.l %d5,(%a0)+
.postfill_38:
	move.l %d5,(%a0)+
.postfill_36:
	move.l %d5,(%a0)+
.postfill_34:
	move.l %d5,(%a0)+
.postfill_32:
	move.l %d5,(%a0)+
.postfill_30:
	move.l %d5,(%a0)+
.postfill_28:
	move.l %d5,(%a0)+
.postfill_26:
	move.l %d5,(%a0)+
.postfill_24:
	move.l %d5,(%a0)+
.postfill_22:
	move.l %d5,(%a0)+
.postfill_20:
	move.l %d5,(%a0)+
.postfill_18:
	move.l %d5,(%a0)+
.postfill_16:
	move.l %d5,(%a0)+
.postfill_14:
	move.l %d5,(%a0)+
.postfill_12:
	move.l %d5,(%a0)+
.postfill_10:
	move.l %d5,(%a0)+
.postfill_8:
	move.l %d5,(%a0)+
.postfill_6:
	move.l %d5,(%a0)+
.postfill_4:
	move.l %d5,(%a0)+
.postfill_2:
	move.l %d5,(%a0)
	rts

.postfill_159:
	move.l %d5,(%a0)+
.postfill_157:
	move.l %d5,(%a0)+
.postfill_155:
	move.l %d5,(%a0)+
.postfill_153:
	move.l %d5,(%a0)+
.postfill_151:
	move.l %d5,(%a0)+
.postfill_149:
	move.l %d5,(%a0)+
.postfill_147:
	move.l %d5,(%a0)+
.postfill_145:
	move.l %d5,(%a0)+
.postfill_143:
	move.l %d5,(%a0)+
.postfill_141:
	move.l %d5,(%a0)+
.postfill_139:
	move.l %d5,(%a0)+
.postfill_137:
	move.l %d5,(%a0)+
.postfill_135:
	move.l %d5,(%a0)+
.postfill_133:
	move.l %d5,(%a0)+
.postfill_131:
	move.l %d5,(%a0)+
.postfill_129:
	move.l %d5,(%a0)+
.postfill_127:
	move.l %d5,(%a0)+
.postfill_125:
	move.l %d5,(%a0)+
.postfill_123:
	move.l %d5,(%a0)+
.postfill_121:
	move.l %d5,(%a0)+
.postfill_119:
	move.l %d5,(%a0)+
.postfill_117:
	move.l %d5,(%a0)+
.postfill_115:
	move.l %d5,(%a0)+
.postfill_113:
	move.l %d5,(%a0)+
.postfill_111:
	move.l %d5,(%a0)+
.postfill_109:
	move.l %d5,(%a0)+
.postfill_107:
	move.l %d5,(%a0)+
.postfill_105:
	move.l %d5,(%a0)+
.postfill_103:
	move.l %d5,(%a0)+
.postfill_101:
	move.l %d5,(%a0)+
.postfill_99:
	move.l %d5,(%a0)+
.postfill_97:
	move.l %d5,(%a0)+
.postfill_95:
	move.l %d5,(%a0)+
.postfill_93:
	move.l %d5,(%a0)+
.postfill_91:
	move.l %d5,(%a0)+
.postfill_89:
	move.l %d5,(%a0)+
.postfill_87:
	move.l %d5,(%a0)+
.postfill_85:
	move.l %d5,(%a0)+
.postfill_83:
	move.l %d5,(%a0)+
.postfill_81:
	move.l %d5,(%a0)+
.postfill_79:
	move.l %d5,(%a0)+
.postfill_77:
	move.l %d5,(%a0)+
.postfill_75:
	move.l %d5,(%a0)+
.postfill_73:
	move.l %d5,(%a0)+
.postfill_71:
	move.l %d5,(%a0)+
.postfill_69:
	move.l %d5,(%a0)+
.postfill_67:
	move.l %d5,(%a0)+
.postfill_65:
	move.l %d5,(%a0)+
.postfill_63:
	move.l %d5,(%a0)+
.postfill_61:
	move.l %d5,(%a0)+
.postfill_59:
	move.l %d5,(%a0)+
.postfill_57:
	move.l %d5,(%a0)+
.postfill_55:
	move.l %d5,(%a0)+
.postfill_53:
	move.l %d5,(%a0)+
.postfill_51:
	move.l %d5,(%a0)+
.postfill_49:
	move.l %d5,(%a0)+
.postfill_47:
	move.l %d5,(%a0)+
.postfill_45:
	move.l %d5,(%a0)+
.postfill_43:
	move.l %d5,(%a0)+
.postfill_41:
	move.l %d5,(%a0)+
.postfill_39:
	move.l %d5,(%a0)+
.postfill_37:
	move.l %d5,(%a0)+
.postfill_35:
	move.l %d5,(%a0)+
.postfill_33:
	move.l %d5,(%a0)+
.postfill_31:
	move.l %d5,(%a0)+
.postfill_29:
	move.l %d5,(%a0)+
.postfill_27:
	move.l %d5,(%a0)+
.postfill_25:
	move.l %d5,(%a0)+
.postfill_23:
	move.l %d5,(%a0)+
.postfill_21:
	move.l %d5,(%a0)+
.postfill_19:
	move.l %d5,(%a0)+
.postfill_17:
	move.l %d5,(%a0)+
.postfill_15:
	move.l %d5,(%a0)+
.postfill_13:
	move.l %d5,(%a0)+
.postfill_11:
	move.l %d5,(%a0)+
.postfill_9:
	move.l %d5,(%a0)+
.postfill_7:
	move.l %d5,(%a0)+
.postfill_5:
	move.l %d5,(%a0)+
.postfill_3:
	move.l %d5,(%a0)+
.postfill_1:
	move.w %d5,(%a0)
.postfill_0:
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

    move.l leftEdge,%a4         | a4 = leftEdge
    move.l rightEdge,%a5        | a5 = rightEdge

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

	add.w  %d0,%a4              | a4 = left = &leftEdge[minY]       (16 bits is ok as leftEdge is in ram)
	add.w  %d0,%a5              | a5 = right = &rightEdge[minY]     (16 bits is ok as rightEdge is in ram)
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
	add.w  %d3,%d3
	add.w  %d3,%d3              |   d3 = (width & 0x1F) << 2

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
	exg %d5,%a3             | exchange color nibble
	dbra %d1,.L180

    moveq #0,%d0
	movm.l (%sp)+,%d6-%d7/%a2-%a6
	movm.l (%sp)+,%d2-%d5
	rts


| %d2 = x (32 bits extended)
| %d4 = dx
| %d5 = dy
| %a1 = dst

fillEdge:
    ext.l %d4
    asl.l #7,%d4            | d4 = dx << 7
    divs.w %d5,%d4          | d4 = (dx << 7) / dy
	ext.l %d4

|	ror.l #7,%d4            | d4 = step = (dx << 16) / dy (swapped)

	ror.l #8,%d4            | d4 = step >> 1 (32 bits fixed point)
	add.l %d4,%d2           | d2 = x + (step >> 1)
	rol.l #1,%d4            | d4 = step (32 bits fixed point)

    add.w %d5,%d5
    add.w %d5,%d5           | for jump table
	andi #0,%ccr            | clear X flag

.fe_fill_base2:
    move.l (.fe_fill_table-.fe_fill_base2)-2(%pc,%d5.w),%a6
    jmp (%a6)

    .align 4
.fe_fill_table:
    .long .fe_fill_0
    .long .fe_fill_1
    .long .fe_fill_2
    .long .fe_fill_3
    .long .fe_fill_4
    .long .fe_fill_5
    .long .fe_fill_6
    .long .fe_fill_7
    .long .fe_fill_8
    .long .fe_fill_9
    .long .fe_fill_10
    .long .fe_fill_11
    .long .fe_fill_12
    .long .fe_fill_13
    .long .fe_fill_14
    .long .fe_fill_15
    .long .fe_fill_16
    .long .fe_fill_17
    .long .fe_fill_18
    .long .fe_fill_19
    .long .fe_fill_20
    .long .fe_fill_21
    .long .fe_fill_22
    .long .fe_fill_23
    .long .fe_fill_24
    .long .fe_fill_25
    .long .fe_fill_26
    .long .fe_fill_27
    .long .fe_fill_28
    .long .fe_fill_29
    .long .fe_fill_30
    .long .fe_fill_31
    .long .fe_fill_32
    .long .fe_fill_33
    .long .fe_fill_34
    .long .fe_fill_35
    .long .fe_fill_36
    .long .fe_fill_37
    .long .fe_fill_38
    .long .fe_fill_39
    .long .fe_fill_40
    .long .fe_fill_41
    .long .fe_fill_42
    .long .fe_fill_43
    .long .fe_fill_44
    .long .fe_fill_45
    .long .fe_fill_46
    .long .fe_fill_47
    .long .fe_fill_48
    .long .fe_fill_49
    .long .fe_fill_50
    .long .fe_fill_51
    .long .fe_fill_52
    .long .fe_fill_53
    .long .fe_fill_54
    .long .fe_fill_55
    .long .fe_fill_56
    .long .fe_fill_57
    .long .fe_fill_58
    .long .fe_fill_59
    .long .fe_fill_60
    .long .fe_fill_61
    .long .fe_fill_62
    .long .fe_fill_63
    .long .fe_fill_64
    .long .fe_fill_65
    .long .fe_fill_66
    .long .fe_fill_67
    .long .fe_fill_68
    .long .fe_fill_69
    .long .fe_fill_70
    .long .fe_fill_71
    .long .fe_fill_72
    .long .fe_fill_73
    .long .fe_fill_74
    .long .fe_fill_75
    .long .fe_fill_76
    .long .fe_fill_77
    .long .fe_fill_78
    .long .fe_fill_79
    .long .fe_fill_80
    .long .fe_fill_81
    .long .fe_fill_82
    .long .fe_fill_83
    .long .fe_fill_84
    .long .fe_fill_85
    .long .fe_fill_86
    .long .fe_fill_87
    .long .fe_fill_88
    .long .fe_fill_89
    .long .fe_fill_90
    .long .fe_fill_91
    .long .fe_fill_92
    .long .fe_fill_93
    .long .fe_fill_94
    .long .fe_fill_95
    .long .fe_fill_96
    .long .fe_fill_97
    .long .fe_fill_98
    .long .fe_fill_99
    .long .fe_fill_100
    .long .fe_fill_101
    .long .fe_fill_102
    .long .fe_fill_103
    .long .fe_fill_104
    .long .fe_fill_105
    .long .fe_fill_106
    .long .fe_fill_107
    .long .fe_fill_108
    .long .fe_fill_109
    .long .fe_fill_110
    .long .fe_fill_111
    .long .fe_fill_112
    .long .fe_fill_113
    .long .fe_fill_114
    .long .fe_fill_115
    .long .fe_fill_116
    .long .fe_fill_117
    .long .fe_fill_118
    .long .fe_fill_119
    .long .fe_fill_120
    .long .fe_fill_121
    .long .fe_fill_122
    .long .fe_fill_123
    .long .fe_fill_124
    .long .fe_fill_125
    .long .fe_fill_126
    .long .fe_fill_127
    .long .fe_fill_128
    .long .fe_fill_129
    .long .fe_fill_130
    .long .fe_fill_131
    .long .fe_fill_132
    .long .fe_fill_133
    .long .fe_fill_134
    .long .fe_fill_135
    .long .fe_fill_136
    .long .fe_fill_137
    .long .fe_fill_138
    .long .fe_fill_139
    .long .fe_fill_140
    .long .fe_fill_141
    .long .fe_fill_142
    .long .fe_fill_143
    .long .fe_fill_144
    .long .fe_fill_145
    .long .fe_fill_146
    .long .fe_fill_147
    .long .fe_fill_148
    .long .fe_fill_149
    .long .fe_fill_150
    .long .fe_fill_151
    .long .fe_fill_152
    .long .fe_fill_153
    .long .fe_fill_154
    .long .fe_fill_155
    .long .fe_fill_156
    .long .fe_fill_157
    .long .fe_fill_158
    .long .fe_fill_159
    .long .fe_fill_160

.fe_fill_160:
	move.w %d2,(%a1)+           | *src++ = fix16ToInt(x);
	addx.l %d4,%d2              |  x += step;
.fe_fill_159:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_158:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_157:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_156:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_155:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_154:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_153:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_152:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_151:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_150:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_149:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_148:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_147:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_146:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_145:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_144:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_143:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_142:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_141:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_140:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_139:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_138:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_137:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_136:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_135:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_134:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_133:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_132:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_131:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_130:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_129:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_128:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_127:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_126:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_125:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_124:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_123:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_122:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_121:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_120:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_119:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_118:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_117:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_116:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_115:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_114:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_113:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_112:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_111:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_110:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_109:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_108:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_107:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_106:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_105:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_104:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_103:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_102:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_101:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_100:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_99:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_98:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_97:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_96:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_95:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_94:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_93:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_92:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_91:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_90:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_89:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_88:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_87:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_86:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_85:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_84:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_83:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_82:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_81:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_80:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_79:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_78:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_77:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_76:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_75:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_74:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_73:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_72:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_71:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_70:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_69:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_68:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_67:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_66:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_65:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_64:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_63:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_62:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_61:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_60:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_59:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_58:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_57:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_56:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_55:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_54:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_53:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_52:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_51:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_50:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_49:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_48:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_47:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_46:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_45:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_44:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_43:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_42:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_41:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_40:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_39:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_38:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_37:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_36:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_35:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_34:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_33:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_32:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_31:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_30:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_29:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_28:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_27:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_26:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_25:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_24:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_23:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_22:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_21:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_20:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_19:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_18:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_17:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_16:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_15:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_14:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_13:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_12:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_11:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_10:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_9:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_8:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_7:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_6:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_5:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_4:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_3:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_2:
	move.w %d2,(%a1)+
	addx.l %d4,%d2
.fe_fill_1:
	move.w %d2,(%a1)+
.fe_fill_0:
    rts


| %d2 = (x << 16) | (x << 0)
| %d1 = len
| %a1 = dst

fillEdgeFast:
    add.w %d1,%d1
    add.w %d1,%d1               | for jump table

.fef_fill_base:
    move.l (.fef_fill_table-.fef_fill_base)-2(%pc,%d1.w),%a6
    jmp (%a6)

    .align 4
.fef_fill_table:
    .long .fef_fill_0
    .long .fef_fill_1
    .long .fef_fill_2
    .long .fef_fill_3
    .long .fef_fill_4
    .long .fef_fill_5
    .long .fef_fill_6
    .long .fef_fill_7
    .long .fef_fill_8
    .long .fef_fill_9
    .long .fef_fill_10
    .long .fef_fill_11
    .long .fef_fill_12
    .long .fef_fill_13
    .long .fef_fill_14
    .long .fef_fill_15
    .long .fef_fill_16
    .long .fef_fill_17
    .long .fef_fill_18
    .long .fef_fill_19
    .long .fef_fill_20
    .long .fef_fill_21
    .long .fef_fill_22
    .long .fef_fill_23
    .long .fef_fill_24
    .long .fef_fill_25
    .long .fef_fill_26
    .long .fef_fill_27
    .long .fef_fill_28
    .long .fef_fill_29
    .long .fef_fill_30
    .long .fef_fill_31
    .long .fef_fill_32
    .long .fef_fill_33
    .long .fef_fill_34
    .long .fef_fill_35
    .long .fef_fill_36
    .long .fef_fill_37
    .long .fef_fill_38
    .long .fef_fill_39
    .long .fef_fill_40
    .long .fef_fill_41
    .long .fef_fill_42
    .long .fef_fill_43
    .long .fef_fill_44
    .long .fef_fill_45
    .long .fef_fill_46
    .long .fef_fill_47
    .long .fef_fill_48
    .long .fef_fill_49
    .long .fef_fill_50
    .long .fef_fill_51
    .long .fef_fill_52
    .long .fef_fill_53
    .long .fef_fill_54
    .long .fef_fill_55
    .long .fef_fill_56
    .long .fef_fill_57
    .long .fef_fill_58
    .long .fef_fill_59
    .long .fef_fill_60
    .long .fef_fill_61
    .long .fef_fill_62
    .long .fef_fill_63
    .long .fef_fill_64
    .long .fef_fill_65
    .long .fef_fill_66
    .long .fef_fill_67
    .long .fef_fill_68
    .long .fef_fill_69
    .long .fef_fill_70
    .long .fef_fill_71
    .long .fef_fill_72
    .long .fef_fill_73
    .long .fef_fill_74
    .long .fef_fill_75
    .long .fef_fill_76
    .long .fef_fill_77
    .long .fef_fill_78
    .long .fef_fill_79
    .long .fef_fill_80
    .long .fef_fill_81
    .long .fef_fill_82
    .long .fef_fill_83
    .long .fef_fill_84
    .long .fef_fill_85
    .long .fef_fill_86
    .long .fef_fill_87
    .long .fef_fill_88
    .long .fef_fill_89
    .long .fef_fill_90
    .long .fef_fill_91
    .long .fef_fill_92
    .long .fef_fill_93
    .long .fef_fill_94
    .long .fef_fill_95
    .long .fef_fill_96
    .long .fef_fill_97
    .long .fef_fill_98
    .long .fef_fill_99
    .long .fef_fill_100
    .long .fef_fill_101
    .long .fef_fill_102
    .long .fef_fill_103
    .long .fef_fill_104
    .long .fef_fill_105
    .long .fef_fill_106
    .long .fef_fill_107
    .long .fef_fill_108
    .long .fef_fill_109
    .long .fef_fill_110
    .long .fef_fill_111
    .long .fef_fill_112
    .long .fef_fill_113
    .long .fef_fill_114
    .long .fef_fill_115
    .long .fef_fill_116
    .long .fef_fill_117
    .long .fef_fill_118
    .long .fef_fill_119
    .long .fef_fill_120
    .long .fef_fill_121
    .long .fef_fill_122
    .long .fef_fill_123
    .long .fef_fill_124
    .long .fef_fill_125
    .long .fef_fill_126
    .long .fef_fill_127
    .long .fef_fill_128
    .long .fef_fill_129
    .long .fef_fill_130
    .long .fef_fill_131
    .long .fef_fill_132
    .long .fef_fill_133
    .long .fef_fill_134
    .long .fef_fill_135
    .long .fef_fill_136
    .long .fef_fill_137
    .long .fef_fill_138
    .long .fef_fill_139
    .long .fef_fill_140
    .long .fef_fill_141
    .long .fef_fill_142
    .long .fef_fill_143
    .long .fef_fill_144
    .long .fef_fill_145
    .long .fef_fill_146
    .long .fef_fill_147
    .long .fef_fill_148
    .long .fef_fill_149
    .long .fef_fill_150
    .long .fef_fill_151
    .long .fef_fill_152
    .long .fef_fill_153
    .long .fef_fill_154
    .long .fef_fill_155
    .long .fef_fill_156
    .long .fef_fill_157
    .long .fef_fill_158
    .long .fef_fill_159
    .long .fef_fill_160

.fef_fill_160:
	move.l %d2,(%a1)+
.fef_fill_158:
	move.l %d2,(%a1)+
.fef_fill_156:
	move.l %d2,(%a1)+
.fef_fill_154:
	move.l %d2,(%a1)+
.fef_fill_152:
	move.l %d2,(%a1)+
.fef_fill_150:
	move.l %d2,(%a1)+
.fef_fill_148:
	move.l %d2,(%a1)+
.fef_fill_146:
	move.l %d2,(%a1)+
.fef_fill_144:
	move.l %d2,(%a1)+
.fef_fill_142:
	move.l %d2,(%a1)+
.fef_fill_140:
	move.l %d2,(%a1)+
.fef_fill_138:
	move.l %d2,(%a1)+
.fef_fill_136:
	move.l %d2,(%a1)+
.fef_fill_134:
	move.l %d2,(%a1)+
.fef_fill_132:
	move.l %d2,(%a1)+
.fef_fill_130:
	move.l %d2,(%a1)+
.fef_fill_128:
	move.l %d2,(%a1)+
.fef_fill_126:
	move.l %d2,(%a1)+
.fef_fill_124:
	move.l %d2,(%a1)+
.fef_fill_122:
	move.l %d2,(%a1)+
.fef_fill_120:
	move.l %d2,(%a1)+
.fef_fill_118:
	move.l %d2,(%a1)+
.fef_fill_116:
	move.l %d2,(%a1)+
.fef_fill_114:
	move.l %d2,(%a1)+
.fef_fill_112:
	move.l %d2,(%a1)+
.fef_fill_110:
	move.l %d2,(%a1)+
.fef_fill_108:
	move.l %d2,(%a1)+
.fef_fill_106:
	move.l %d2,(%a1)+
.fef_fill_104:
	move.l %d2,(%a1)+
.fef_fill_102:
	move.l %d2,(%a1)+
.fef_fill_100:
	move.l %d2,(%a1)+
.fef_fill_98:
	move.l %d2,(%a1)+
.fef_fill_96:
	move.l %d2,(%a1)+
.fef_fill_94:
	move.l %d2,(%a1)+
.fef_fill_92:
	move.l %d2,(%a1)+
.fef_fill_90:
	move.l %d2,(%a1)+
.fef_fill_88:
	move.l %d2,(%a1)+
.fef_fill_86:
	move.l %d2,(%a1)+
.fef_fill_84:
	move.l %d2,(%a1)+
.fef_fill_82:
	move.l %d2,(%a1)+
.fef_fill_80:
	move.l %d2,(%a1)+
.fef_fill_78:
	move.l %d2,(%a1)+
.fef_fill_76:
	move.l %d2,(%a1)+
.fef_fill_74:
	move.l %d2,(%a1)+
.fef_fill_72:
	move.l %d2,(%a1)+
.fef_fill_70:
	move.l %d2,(%a1)+
.fef_fill_68:
	move.l %d2,(%a1)+
.fef_fill_66:
	move.l %d2,(%a1)+
.fef_fill_64:
	move.l %d2,(%a1)+
.fef_fill_62:
	move.l %d2,(%a1)+
.fef_fill_60:
	move.l %d2,(%a1)+
.fef_fill_58:
	move.l %d2,(%a1)+
.fef_fill_56:
	move.l %d2,(%a1)+
.fef_fill_54:
	move.l %d2,(%a1)+
.fef_fill_52:
	move.l %d2,(%a1)+
.fef_fill_50:
	move.l %d2,(%a1)+
.fef_fill_48:
	move.l %d2,(%a1)+
.fef_fill_46:
	move.l %d2,(%a1)+
.fef_fill_44:
	move.l %d2,(%a1)+
.fef_fill_42:
	move.l %d2,(%a1)+
.fef_fill_40:
	move.l %d2,(%a1)+
.fef_fill_38:
	move.l %d2,(%a1)+
.fef_fill_36:
	move.l %d2,(%a1)+
.fef_fill_34:
	move.l %d2,(%a1)+
.fef_fill_32:
	move.l %d2,(%a1)+
.fef_fill_30:
	move.l %d2,(%a1)+
.fef_fill_28:
	move.l %d2,(%a1)+
.fef_fill_26:
	move.l %d2,(%a1)+
.fef_fill_24:
	move.l %d2,(%a1)+
.fef_fill_22:
	move.l %d2,(%a1)+
.fef_fill_20:
	move.l %d2,(%a1)+
.fef_fill_18:
	move.l %d2,(%a1)+
.fef_fill_16:
	move.l %d2,(%a1)+
.fef_fill_14:
	move.l %d2,(%a1)+
.fef_fill_12:
	move.l %d2,(%a1)+
.fef_fill_10:
	move.l %d2,(%a1)+
.fef_fill_8:
	move.l %d2,(%a1)+
.fef_fill_6:
	move.l %d2,(%a1)+
.fef_fill_4:
	move.l %d2,(%a1)+
.fef_fill_2:
	move.l %d2,(%a1)+
.fef_fill_0:
    rts

.fef_fill_159:
	move.l %d2,(%a1)+
.fef_fill_157:
	move.l %d2,(%a1)+
.fef_fill_155:
	move.l %d2,(%a1)+
.fef_fill_153:
	move.l %d2,(%a1)+
.fef_fill_151:
	move.l %d2,(%a1)+
.fef_fill_149:
	move.l %d2,(%a1)+
.fef_fill_147:
	move.l %d2,(%a1)+
.fef_fill_145:
	move.l %d2,(%a1)+
.fef_fill_143:
	move.l %d2,(%a1)+
.fef_fill_141:
	move.l %d2,(%a1)+
.fef_fill_139:
	move.l %d2,(%a1)+
.fef_fill_137:
	move.l %d2,(%a1)+
.fef_fill_135:
	move.l %d2,(%a1)+
.fef_fill_133:
	move.l %d2,(%a1)+
.fef_fill_131:
	move.l %d2,(%a1)+
.fef_fill_129:
	move.l %d2,(%a1)+
.fef_fill_127:
	move.l %d2,(%a1)+
.fef_fill_125:
	move.l %d2,(%a1)+
.fef_fill_123:
	move.l %d2,(%a1)+
.fef_fill_121:
	move.l %d2,(%a1)+
.fef_fill_119:
	move.l %d2,(%a1)+
.fef_fill_117:
	move.l %d2,(%a1)+
.fef_fill_115:
	move.l %d2,(%a1)+
.fef_fill_113:
	move.l %d2,(%a1)+
.fef_fill_111:
	move.l %d2,(%a1)+
.fef_fill_109:
	move.l %d2,(%a1)+
.fef_fill_107:
	move.l %d2,(%a1)+
.fef_fill_105:
	move.l %d2,(%a1)+
.fef_fill_103:
	move.l %d2,(%a1)+
.fef_fill_101:
	move.l %d2,(%a1)+
.fef_fill_99:
	move.l %d2,(%a1)+
.fef_fill_97:
	move.l %d2,(%a1)+
.fef_fill_95:
	move.l %d2,(%a1)+
.fef_fill_93:
	move.l %d2,(%a1)+
.fef_fill_91:
	move.l %d2,(%a1)+
.fef_fill_89:
	move.l %d2,(%a1)+
.fef_fill_87:
	move.l %d2,(%a1)+
.fef_fill_85:
	move.l %d2,(%a1)+
.fef_fill_83:
	move.l %d2,(%a1)+
.fef_fill_81:
	move.l %d2,(%a1)+
.fef_fill_79:
	move.l %d2,(%a1)+
.fef_fill_77:
	move.l %d2,(%a1)+
.fef_fill_75:
	move.l %d2,(%a1)+
.fef_fill_73:
	move.l %d2,(%a1)+
.fef_fill_71:
	move.l %d2,(%a1)+
.fef_fill_69:
	move.l %d2,(%a1)+
.fef_fill_67:
	move.l %d2,(%a1)+
.fef_fill_65:
	move.l %d2,(%a1)+
.fef_fill_63:
	move.l %d2,(%a1)+
.fef_fill_61:
	move.l %d2,(%a1)+
.fef_fill_59:
	move.l %d2,(%a1)+
.fef_fill_57:
	move.l %d2,(%a1)+
.fef_fill_55:
	move.l %d2,(%a1)+
.fef_fill_53:
	move.l %d2,(%a1)+
.fef_fill_51:
	move.l %d2,(%a1)+
.fef_fill_49:
	move.l %d2,(%a1)+
.fef_fill_47:
	move.l %d2,(%a1)+
.fef_fill_45:
	move.l %d2,(%a1)+
.fef_fill_43:
	move.l %d2,(%a1)+
.fef_fill_41:
	move.l %d2,(%a1)+
.fef_fill_39:
	move.l %d2,(%a1)+
.fef_fill_37:
	move.l %d2,(%a1)+
.fef_fill_35:
	move.l %d2,(%a1)+
.fef_fill_33:
	move.l %d2,(%a1)+
.fef_fill_31:
	move.l %d2,(%a1)+
.fef_fill_29:
	move.l %d2,(%a1)+
.fef_fill_27:
	move.l %d2,(%a1)+
.fef_fill_25:
	move.l %d2,(%a1)+
.fef_fill_23:
	move.l %d2,(%a1)+
.fef_fill_21:
	move.l %d2,(%a1)+
.fef_fill_19:
	move.l %d2,(%a1)+
.fef_fill_17:
	move.l %d2,(%a1)+
.fef_fill_15:
	move.l %d2,(%a1)+
.fef_fill_13:
	move.l %d2,(%a1)+
.fef_fill_11:
	move.l %d2,(%a1)+
.fef_fill_9:
	move.l %d2,(%a1)+
.fef_fill_7:
	move.l %d2,(%a1)+
.fef_fill_5:
	move.l %d2,(%a1)+
.fef_fill_3:
	move.l %d2,(%a1)+
.fef_fill_1:
	move.w %d2,(%a1)+
    rts


| u16 calculateEdges_new(const Vect2D_s16 *pts, u16 num)
|
|a0 = pt / pt1
|a1 = edge / pt0
|a2 = ptYMin
|a3 = ptYMax
|a4 = ptFirst
|a5 = ptLast
|a6 = misc

|d2 = yMin
|d3 = yMax

|d0 = dx
|d1 = dy
|d2 = x0
|d3 = y0
|d4 = x1
|d5 = y1
|d6 = BMP_HEIGHT-1
|d7 = BMP_WIDTH-1

	.globl	calculateEdges_new
	.type	calculateEdges_new, @function
calculateEdges_new:
	movm.l %d2-%d7/%a2-%a6,-(%sp)

	move.l 48(%sp),%a0      | a0 = pt = &pts[0]
	move.w 54(%sp),%d1      | d1 = num

 .cen_loop:
|    jmp .cen_loop

	move.w %d1,%d0
	add.w %d0,%d0
	add.w %d0,%d0           | d0 = num * 4
	move.l %a0,%a4          | a4 = ptFirt
	lea -4(%a0,%d0.w),%a5   | a5 = ptLast

	move.l (%a0)+,%d0       | d0.w = y = pt->y; pt++;

	move.l %a0,%a2          | a2 = ptYMin = &pts[1]
	move.l %a0,%a3          | a3 = ptYMax = &pts[1]
	move.w %d0,%d2          | d2.w = yMin = y
	move.w %d0,%d3          | d3.W = yMax = y

	subq.w #2,%d1           | d1 = cnt = num - 2

.cen_157:                   | while (cnt--) {
	move.l (%a0)+,%d0       |   y = pt->y; py++;

	cmp.w %d2,%d0           |   if (y < yMin)
	jge .cen_154            |   {

    move.w %d0,%d2          |     yMin = y
    move.l %a0,%a2          |     ptYMin = pt + 1
	dbra %d1,.cen_157       |   }

	jra .cen_156            | done

.cen_154:
	cmp.w %d3,%d0           |   if (y > yMax)
	jle .cen_155            |   {

    move.w %d0,%d3          |     yMax = y
    move.l %a0,%a3          |     ptYMax = pt + 1

.cen_155:                   |   }
	dbra %d1,.cen_157       | }

.cen_156:
	subq.l #4,%a2           | fix ptYMin
	subq.l #4,%a3           | fix ptYMax

	tst.w %d3               | if (yMax < 0)
	jlt .cen_end0           |   return 0

    moveq #0,%d6
	move.w #159,%d6         | d6 = BMP_HEIGHT-1

	cmp.w %d6,%d2           | if (yMin > BMP_HEIGHT)
	jgt .cen_end0           |   return 0

    moveq #0,%d7
	move.w #255,%d7         | d7 = BMP_WIDTH-1

	cmp.w %d3,%d2           | if (yMin == yMax)
	jne .cen_160            | {

    move.w (%a2),%d4        |   d4 = x0 = ptYMin->x
    move.w (%a3),%d5        |   d5 = x1 = ptYMax->x

    cmp.w %d5,%d4           |   if (x0 > x1)
    jle .cen_161

	exg %d4,%d5             |      SWAP(x0, x1)

.cen_161:
	tst.w %d5               |   if (x1 < 0)
	jlt .cen_end0           |     return 0
	cmp.w %d7,%d4           |   if (x0 >= BMP_WIDTH)
	jgt .cen_end0           |     return 0

	tst.w %d4               |   if (x0 < 0)
	jge .cen_162

	moveq #0,%d4            |     x0 = 0

.cen_162:
	cmp.w %d7,%d5           |   if (x1 >= BMP_WIDTH)
	jle .cen_163

	move.w %d7,%d5          |     x1 = BMP_WIDTH - 1

.cen_163:
	move.w %d2,minY         |   minY = yMin = yMax
	addq.w #1,%d2
	move.w %d2,maxY         |   maxY = yMin + 1 = yMax

	add.w %d2,%d2

    move.l leftEdge,%a0
	move.w %d4,-2(%a0,%d2)  |   leftEdge[yMin] = x0
    move.l rightEdge,%a0
	move.w %d5,-2(%a0,%d2)  |   rightEdge[yMin] = x1

	jra .cen_end1           |   return 1
                            | }

    | LEFT
    | ----

.cen_160:
    move.l %a2,%a1          | a1 = pt0 = ptYMin
    lea -4(%a2),%a0         | a0 = pt1 = ptYMin-1
    cmp.l %a4,%a0           | if (pt1 < ptFirst)
    jcc .cen_166

    move.l %a5,%a0          |   pt1 = ptLast

.cen_166:
	tst.w 2(%a0)            | while (pt1->y <= 0)
	jgt .cen_250            | {

.cen_171:
	cmp.l %a3,%a0           |   if (pt1 == ptYMax)
	jeq .cen_end0           |     return 0

	move.l %a0,%a1          |   pt0 = pt1
	subq.l #4,%a0           |   pt1--
    cmp.l %a4,%a0           |   if (pt1 < ptFirst)
    jcc .cen_167

    move.l %a5,%a0          |     pt1 = ptLast

.cen_167:
	tst.w 2(%a0)            | }
	jle .cen_171

.cen_250:
	cmp.w (%a1),%d7         | while ((pt0->x >= BMP_WIDTH) && (pt1->x >= BMP_WIDTH))
	jge .cen_173            | {
	cmp.w (%a0),%d7
	jge .cen_173

.cen_177:
	move.w (%a1),%d0
	cmp.w (%a0),%d0         |   if (pt0->x <= pt1->x)
	jle .cen_end0           |     return 0;

	cmp.l %a3,%a0           |   if (pt1 == ptYMax)
	jeq .cen_end0           |     return 0

	move.l %a0,%a1          |   pt0 = pt1
	subq.l #4,%a0           |   pt1--
    cmp.l %a4,%a0           |   if (pt1 < ptFirst)
    jcc .cen_172

    move.l %a5,%a0          |     pt1 = ptLast

.cen_172:
	cmp.w (%a0),%d7         | }
	jlt .cen_177

.cen_173:
	move.w (%a0),%d4        | x1 = pt->x
	move.w 2(%a0),%d5       | y1 = pt->y
	move.w (%a1)+,%d2       | x0 = pt0->x
	move.w (%a1),%d3        | y0 = pt0->y

    | Clip Y0

	jge .cen_178            | if (y0 < 0)
                            | {
    move.w %d4,%d0
	sub.w %d2,%d0           |   d0 = dx
    move.w %d5,%d1
	sub.w %d3,%d1           |   d1 = dy

    muls.w %d3,%d0
    divs.w %d1,%d0          |   d0 = adj = (y0 * dx) / dy
    sub.w %d0,%d2           |   x0 -= adj
	moveq #0,%d3            |   y0 = 0
                            | }
.cen_178:
|    jmp .cen_178

	move.w %d3,minYL        | minYL = y0
	move.l leftEdge,%d0
	add.w %d3,%d0
	add.w %d3,%d0
	move.l %d0,%a1          | a1 = edge = &leftEdge[minYL]


.cen_loopL:

    | Clip Y1 against BMP_HEIGHT

	cmp.w %d6,%d5           | if (y1 >= BMP_HEIGHT)
	jle .cen_180            | {

    move.w %d4,%d0
	sub.w %d2,%d0           |   d0 = dx
    move.w %d5,%d1
	sub.w %d3,%d1           |   d1 = dy

	sub.w %d6,%d5
	muls.w %d5,%d0
	divs.w %d1,%d0          |   d0 = adj = ((y1 - (BMP_HEIGHT - 1)) * dx) / dy
    sub.w %d0,%d4           |   x1 -= adj
	move.w %d6,%d5          |   y1 = BMP_HEIGHT - 1
	move.l %a3,%a0          |   pt = ptYMax
                            | }

    | clip X0

.cen_180:
	cmp.w %d7,%d2           | if (x0 >= BMP_WIDTH)
	jls .cenl_x0_ok         | {
	jle .cenl_x0_neg

.cenl_x0_sup:
	cmp.w %d7,%d4           |   if (x1 >= BMP_WIDTH)
	jle .cen_182            |   {

	cmp.w %d4,%d2           |     if (x0 <= x1)
	jle .cen_endL           |       goto endL

	jra .cen_nextL          |     goto nextL
                            |   }
.cen_182:
    move.w %d4,%d0
	sub.w %d2,%d0           |   d0 = dx
    move.w %d5,%d1
	sub.w %d3,%d1           |   d1 = dy

	sub.w %d7,%d2
	muls.w %d2,%d1
	divs.w %d0,%d1          |   d1 = adj = ((x0 - (BMP_WIDTH - 1)) * dy) / dx
    sub.w %d1,%d3           |   y0 -= adj

    | from here we assume d2 = x0 = d7 = BMP_WIDTH - 1

	move.w %d3,minYL        |   minYL = y0
	move.l leftEdge,%d0
	add.w %d3,%d0
	add.w %d3,%d0
	move.l %d0,%a1          |   a1 = edge = &leftEdge[minYL]

    move.w %d4,%d0          |   if (x1 < 0)
	jge .cen_186            |   {

	sub.w %d7,%d0           |     d0 = dx = x1 - (BMP_WIDTH - 1)
    move.w %d5,%d1
	sub.w %d3,%d1           |     d1 = dy

    muls.w %d4,%d1
    divs.w %d0,%d1          |     d1 = adj = (x1 * dy) / dx
    sub.w %d1,%d5           |     y1 -= adj
    moveq #0,%d4            |     x1 = 0

	sub.w %d7,%d4           |     d4 = dx = (0 - (BMP_WIDTH - 1))
	sub.w %d3,%d5           |     d5 = dy

    jlt .cen_end0           |     if (dy < 0) return 0
    jeq .cen_187            |     if (dy)
                            |     {
	move.l %d7,%d2          |       d4 = dx; d5 = dy; d2 = x0 = BMP_WIDTH - 1 (32 bit extended)

	jsr fillEdge            |       fillEdge
                            |     }
.cen_187:
	tst.w %d1               |     if (adj < 0) return 0
    jlt .cen_end0           |
	jeq .cen_nextL          |     if (adj)
                            |     {
    moveq #0,%d2            |       d2 = (x << 16) | (x << 0)
    pea .cen_nextL          |       d1 = len
	jra fillEdgeFast        |       fillEdgeFast
                            |     }
                            |   }

.cen_186:
	sub.w %d7,%d4           |   d4 = dx = x1 - (BMP_WIDTH - 1)
	sub.w %d3,%d5           |   d5 = dy

    jlt .cen_end0           |   if (dy < 0) return 0
	jeq .cen_nextL          |   if (dy)
                            |   {
	move.l %d7,%d2          |     d4 = dx; d5 = dy; d2 = x0 = BMP_WIDTH - 1 (32 bit extended)

    pea .cen_nextL
	jra fillEdge            |     fillEdge
                            |   }
                            | }

.cenl_x0_neg:               | if (x0 < 0)
                            | {
    move.w %d4,%d0          |   if (x1 < 0)
	jge .cen_191            |   {

    sub.w %d3,%d5           |     d5 = dy

    jlt .cen_end0           |     if (dy < 0) return 0
	jeq .cen_nextL          |     if (dy)
                            |     {
    moveq #0,%d2            |       d2 = (x << 16) | (x << 0)
    move.w %d5,%d1          |       d1 = len
    pea .cen_nextL
	jra fillEdgeFast        |       fillEdgeFast
                            |     }
                            |   }
.cen_191:
	sub.w %d2,%d0           |   d0 = dx
    move.w %d5,%d1
	sub.w %d3,%d1           |   d1 = dy

	neg.w %d2
	muls.w %d2,%d1
    moveq #0,%d2            |   d1 = adj = (-x0 * dy) / dx
	divs.w %d0,%d1          |   x0 = 0

    jlt .cen_end0           |   if (adj < 0) return 0
	jeq .cen_193            |   if (adj)
                            |   {
    add.w %d1,%d3           |       y0 += adj

                            |       d2 = (x0 << 16) | (x0 << 0); d1 = len

	jsr fillEdgeFast        |       fillEdgeFast
                            |   }
.cen_193:
	cmp.w %d7,%d4           |   if (x1 >= BMP_WIDTH)
	jle .cen_194            |   {

    move.w %d4,%d0          |     d0 = dx = x1
    move.w %d5,%d1
	sub.w %d3,%d1           |     d1 = dy

    sub.w %d7,%d4
    muls.w %d1,%d4
    divs.w %d0,%d4          |     adj = ((x1 - (BMP_WIDTH - 1)) * dy) / dx
    sub.w %d4,%d5           |     y1 -= adj
    move.w %d7,%d4          |     x1 = BMP_WIDTH - 1
                            |   }
.cen_194:
	sub.w %d3,%d5           |   d4 = dx, d5 = dy

    jlt .cen_end0           |   if (dy < 0) return 0
	jeq .cen_nextL          |   if (dy)
                            |   {
                            |     d4 = dx; d5 = dy; d2 = x0 (32 bit extended)
    pea .cen_nextL
	jra fillEdge            |     fillEdge
                            |   }
                            | }

    | Clip X1

.cenl_x0_ok:
	cmp.w %d7,%d4           | if (x1 < 0)
	jls .cenl_x1_ok         | {
	jgt .cenl_x1_sup

.cenl_x1_neg:
	move.w %d4,%d0
	sub.w %d2,%d0           |   d0 = dx
    move.w %d5,%d1
	sub.w %d3,%d1           |   d1 = dy

	muls.w %d4,%d1
	divs.w %d0,%d1          |   adj = (x1 * dy) / dx
	sub.w %d1,%d5           |   y1 -= adj
	moveq #0,%d4            |   x1 = 0

	sub.w %d2,%d4           |   d4 = dx
	sub.w %d3,%d5           |   d5 = dy

    jlt .cen_end0           |   if (dy < 0) return 0
	jeq .cen_197            |   if (dy)
                            |   {
    ext.l %d2               |     d4 = dx; d5 = dy; d2 = x0 (32 bit extended)

	jsr fillEdge            |     fillEdge
                            |   }
.cen_197:
	tst.w %d1               |
    jlt .cen_end0           |   if (adj < 0) return 0
	jeq .cen_nextL          |   if (adj)
                            |   {
    moveq #0,%d2            |     d2 = (x1 << 16) | (x1 << 0); d1 = len
    pea .cen_nextL
	jra fillEdgeFast        |     fillEdgeFast
                            |   }
                            | }

    | clip x1 against BMP_WIDTH

.cenl_x1_sup:               | if (x1 >= BMP_WIDTH)
	move.w %d4,%d0          | {
	sub.w %d2,%d0           |   d0 = dx
    move.w %d5,%d1
	sub.w %d3,%d1           |   d1 = dy

    sub.w %d7,%d4
    muls.w %d4,%d1
    divs.w %d0,%d1          |   d1 = adj = ((x1 - (BMP_WIDTH - 1)) * dy) / dx
    sub.w %d1,%d5           |   y1 -= adj
    move.w %d7,%d4          |   d4 = x1 = BMP_WIDTH - 1;
                            | }

.cenl_x1_ok:
	sub.w %d2,%d4           | d4 = dx
	sub.w %d3,%d5           | d5 = dy

    jlt .cen_end0           | if (dy < 0) return 0
    jeq .cen_nextL          | if (dy)
                            | {
    ext.l %d2               |   d4 = dx; d5 = dy; d2 = x (32 bit extended)

	jsr fillEdge            |   fillEdge
                            | }

.cen_nextL:
	cmp.l %a3,%a0           | if (pt == ptYMax)
	jeq .cen_endL           |   goto endL

	move.w (%a0)+,%d2       | x0 = pt->x
	move.w (%a0),%d3        | y0 = pt->y

	subq.l #6,%a0           | pt--
	cmp.l %a4,%a0
	jcc .cen_202            | if (pt < ptFirst)

	move.l %a5,%a0          |   pt = ptLast

.cen_202:
	move.w (%a0),%d4        | x1 = pt->x
	move.w 2(%a0),%d5       | y1 = pt->y
	jra .cen_loopL

.cen_endL:
    sub.l leftEdge,%a1      | a1 = maxYL * 2
    move.w %a1,maxYL        | save


    | RIGHT
    | ----

    move.l %a2,%a1          | a1 = pt0 = ptYMin
    lea 4(%a2),%a0          | a0 = pt1 = ptYMin+1
    cmp.l %a5,%a0           | if (pt1 > ptLast)
    jls .cen_203

    move.l %a4,%a0          |   pt1 = ptFirst

.cen_203:
	tst.w 2(%a0)            | while (pt1->y <= 0)
	jgt .cen_253            | {

.cen_208:
	cmp.l %a3,%a0           |   if (pt1 == ptYMax)
	jeq .cen_end0           |     return 0

	move.l %a0,%a1          |   pt0 = pt1
	addq.l #4,%a0           |   pt1++
    cmp.l %a5,%a0           |   if (pt1 > ptLast)
    jls .cen_204

    move.l %a4,%a0          |   pt1 = ptFirst

.cen_204:
	tst.w 2(%a0)            | }
	jle .cen_208

.cen_253:
	tst.w (%a1)             | while ((pt0->x < 0) && (pt1->x < 0))
	jge .cen_210            | {
	tst.w (%a0)
	jge .cen_210

.cen_214:
	move.w (%a1),%d0
	cmp.w (%a0),%d0         |   if (pt0->x >= pt1->x)
	jge .cen_end0           |     return 0;

	cmp.l %a3,%a0           |   if (pt1 == ptYMax)
	jeq .cen_end0           |     return 0

	move.l %a0,%a1          |   pt0 = pt1
	addq.l #4,%a0           |   pt1++
    cmp.l %a5,%a0           |   if (pt1 > ptLast)
    jls .cen_209

    move.l %a4,%a0          |   pt1 = ptFirst

.cen_209:
	tst.w (%a0)             | }
	jlt .cen_214

.cen_210:
	move.w (%a0),%d4        | x1 = pt->x
	move.w 2(%a0),%d5       | y1 = pt->y
	move.w (%a1)+,%d2       | x0 = pt0->x
	move.w (%a1),%d3        | y0 = pt0->y

    | Clip Y0

	jge .cen_215            | if (y0 < 0)
                            | {
    move.w %d4,%d0
	sub.w %d2,%d0           |   d0 = dx
    move.w %d5,%d1
	sub.w %d3,%d1           |   d1 = dy

    muls.w %d3,%d0
    divs.w %d1,%d0          |   d0 = adj = (y0 * dx) / dy
    sub.w %d0,%d2           |   x0 -= adj
	moveq #0,%d3            |   y0 = 0
                            | }
.cen_215:
	move.w %d3,minYR        | minYR = y0
	move.l rightEdge,%d0
	add.w %d3,%d0
	add.w %d3,%d0
	move.l %d0,%a1          | a1 = edge = &rightEdge[minYR]

    | LoopR:

.cen_loopR:
	cmp.w %d6,%d5           | if (y1 >= BMP_HEIGHT)
	jle .cen_217            | {

    move.w %d4,%d0
	sub.w %d2,%d0           |   d0 = dx
    move.w %d5,%d1
	sub.w %d3,%d1           |   d1 = dy

	sub.w %d6,%d5
	muls.w %d5,%d0
	divs.w %d1,%d0          |   d0 = adj = ((y1 - (BMP_HEIGHT - 1)) * dx) / dy
    sub.w %d0,%d4           |   x1 -= adj
	move.w %d6,%d5          |   y1 = BMP_HEIGHT - 1
	move.l %a3,%a0          |   pt = ptYMax
                            | }

    | clip X0 against 0

.cen_217:
	cmp.w %d7,%d2           | if (x0 < 0)
	jls .cenr_x0_ok         | {
	jgt .cenr_x0_sup

.cenr_x0_neg:
	move.w %d4,%d0          |   if (x1 < 0)
	jge .cen_219            |   {

    cmp.w %d4,%d2           |     if (x0 >= x1) goto endR
    jge .cen_endR

    jra .cen_nextR          |     goto nextR
                            |   }
.cen_219:
	sub.w %d2,%d0           |   d0 = dx
    move.w %d5,%d1
	sub.w %d3,%d1           |   d1 = dy

    muls.w %d2,%d1
    divs.w %d0,%d1          |   d1 = adj = (x0 * dy) / dx
    sub.w %d1,%d3           |   y0 -= adj
	moveq #0,%d2            |   x0 = 0

	move.w %d3,minYR        |   minYR = y0
    move.l rightEdge,%d0
	add.w %d3,%d0
	add.w %d3,%d0
	move.l %d0,%a1          |   a1 = edge = &rightEdge[minYR]

	cmp.w %d7,%d4           |   if (x1 >= BMP_WIDTH)
	jle .cen_223            |   {

    move.w %d4,%d0          |     d0 = dx (don't forget x0 = 0 here)
    move.w %d5,%d1
	sub.w %d3,%d1           |     d1 = dy

    sub.w %d7,%d4
    muls.w %d4,%d1
    divs.w %d0,%d1          |     d1 = adj = ((x1 - (BMP_WIDTH - 1)) * dy) / dx
    sub.w %d1,%d5           |     y1 -= adj

    move.w %d7,%d4          |     d4 = dx = d7 = BMP_WIDTH - 1 (x0 = 0 and x1 = BMP_WIDTH - 1)
	sub.w %d3,%d5           |     d5 = dy

    jlt .cen_end0           |     if (dy < 0) return 0
	jeq .cen_224            |     if (dy)
                            |     {
                            |       d4 = dx; d5 = dy; d2 = x0 (32 bit extended)

	jsr fillEdge            |       fillEdge
                            |     }
.cen_224:
	tst.w %d1               |
    jlt .cen_end0           |     if (adj < 0) return 0
	jeq .cen_nextR          |     if (adj)
                            |     {
    move.w %d7,%d2
    swap %d2
    move.w %d7,%d2          |       d2 = (x1 << 16) | (x1 << 0); d1 = len

    pea .cen_nextR
	jra fillEdgeFast        |       fillEdgeFast
                            |     }
                            |   }

.cen_223:                   |   d4 = x1 = dx (x0 = 0 here)
	sub.w %d3,%d5           |   d5 = dy

    jlt .cen_end0           |   if (dy < 0) return 0
	jeq .cen_nextR          |   if (dy)
                            |   {
                            |     d4 = dx; d5 = dy; d2 = x0 (32 bit extended)
    pea .cen_nextR
	jra fillEdge            |     fillEdge
                            |   }
                            | }

    | clip x0

.cenr_x0_sup:               | if (x0 >= BMP_WIDTH)
                            | {
	cmp.w %d7,%d4           |   if (x1 >= BMP_WIDTH)
	jle .cen_228            |   {

	sub.w %d3,%d5           |     d5 = dy

    jlt .cen_end0           |     if (dy < 0) return 0
	jeq .cen_nextR          |     if (dy)
                            |     {
    move.w %d7,%d2
    swap %d2
    move.w %d7,%d2          |       d2 = (x0 << 16) | (x0 << 0)
    move.w %d5,%d1          |       d1 = len

    pea .cen_nextR
	jra fillEdgeFast        |       fillEdgeFast
                            |     }
                            |   }
.cen_228:
    move.w %d4,%d0
	sub.w %d2,%d0           |   d0 = dx
    move.w %d5,%d1
	sub.w %d3,%d1           |   d1 = dy

    sub.w %d7,%d2           |   d0 = x0 - (BMP_WIDTH - 1)
    neg.w %d2               |   d2 = (BMP_WIDTH - 1) - x0
    muls.w %d2,%d1
    divs.w %d0,%d1          |   d1 = adj = (((BMP_WIDTH - 1) - x0) * dy) / dx;

    jlt .cen_end0           |   if (adj < 0) return 0
    jeq .cen_230            |   if (adj)
                            |   {
    add.w %d1,%d3           |     y0 += adj

    move.w %d7,%d2
    swap %d2                |     d2 = (x0 << 16) | (x0 << 0)
    move.w %d7,%d2          |     d1 = len

	jsr fillEdgeFast        |     fillEdgeFast
                            |   }

.cen_230:
    move.w %d4,%d0          |   if (x1 < 0)
	jge .cen_231            |   {

	sub.w %d7,%d0           |     d0 = dx
    move.w %d5,%d1
	sub.w %d3,%d1           |     d1 = dy

	muls.w %d4,%d1
	divs.w %d0,%d1          |     adj = (x1 * dy) / dx
    sub.w %d1,%d5           |     y1 -= adj
    moveq #0,%d4            |     d4 = x1 = 0
                            |   }
.cen_231:
	sub.w %d7,%d4           |   d0 = dx
	sub.w %d3,%d5           |   d5 = dy

    jlt .cen_end0           |   if (dy < 0) return 0
	jeq .cen_nextR          |   if (dy)
                            |   {
    move.l %d7,%d2          |     d4 = dx; d5 = dy; d2 = x (32 bit extended);

    pea .cen_nextR
	jra fillEdge            |     fillEdge
                            |   }
                            | }

    | clip x1

.cenr_x0_ok:
	cmp.w %d7,%d4           | if (x1 >= BMP_WIDTH)
	jls .cenr_x1_ok         | {
	jle .cenr_x1_neg

    move.w %d4,%d0
	sub.w %d2,%d0           |   d0 = dx
    move.w %d5,%d1
	sub.w %d3,%d1           |   d1 = dy

    sub.w %d7,%d4
    muls.w %d4,%d1
    divs.w %d0,%d1          |   d1 = adj = ((x1 - (BMP_WIDTH - 1)) * dy) / dx
    sub.w %d1,%d5           |   y1 -= adj
    move.w %d7,%d4          |   d4 = x1 = BMP_WIDTH - 1

	sub.w %d2,%d4           |   d4 = dx
	sub.w %d3,%d5           |   d5 = dy

    jlt .cen_end0           |   if (dy < 0) return 0
	jeq .cen_234            |   if (dy)
                            |   {
	ext.l %d2               |     d4 = dx; d5 = dy; d2 = x (32 bit extended)

	jsr fillEdge            |     fillEdge
                            |   }
.cen_234:
	tst.w %d1               |
    jlt .cen_end0           |   if (adj < 0) return 0
	jeq .cen_nextR          |   if (adj)
                            |   {
    move.w %d7,%d2
    swap %d2
    move.w %d7,%d2          |     d2 = (x1 << 16) | (x1 << 0)
                            |     d1 = len
    pea .cen_nextR
	jra fillEdgeFast        |     fillEdgeFast
                            |   }
                            | }

.cenr_x1_neg:               | if (x1 < 0)
                            | {
    move.w %d4,%d0
	sub.w %d2,%d0           |   d0 = dx
    move.w %d5,%d1
	sub.w %d3,%d1           |   d1 = dy

    muls.w %d4,%d1
    divs.w %d0,%d1          |   d1 = adj = (x1 * dy) / dx
    sub.w %d1,%d5           |   y1 -= adj
    moveq #0,%d4            |   d4 = x1 = 0
                            | }

.cenr_x1_ok:
	sub.w %d2,%d4           | d4 = dx
	sub.w %d3,%d5           | d5 = dy

    jlt .cen_end0           | if (dy < 0) return 0
	jeq .cen_nextR          | if (dy)
                            | {
	ext.l %d2               |   d4 = dx; d5 = dy; d2 = x0 (32 bit extended)

    pea .cen_nextR
	jra fillEdge            |   fillEdge
                            | }
.cen_nextR:
	cmp.l %a3,%a0           | if (pt == ptYMax)
	jeq .cen_endR           |   goto endR

	move.w (%a0)+,%d2       | x0 = pt->x
	move.w (%a0)+,%d3       | y0 = pt->y

    cmp.l %a5,%a0           | if (pt1 > ptLast)
    jls .cen_239            |

	move.l %a4,%a0          |   pt = ptFirst

.cen_239:
	move.w (%a0),%d4        | x1 = pt->x
	move.w 2(%a0),%d5       | y1 = pt->y
	jra .cen_loopR

.cen_endR:
    sub.l rightEdge,%a1     | a1 = 2 * maxYR = edge - rightEdge
    move.w maxYL,%d1        | d1 = 2 * maxYL

	cmp.w %a1,%d1           | if (maxYL > maxYR)
	jle .cen_240

	move.w %a1,%d1          |   d1 = maxYR

.cen_240:
    lsr.w #1,%d1            | d1 = maxY

    move.w minYR,%d2        | d2 = minYR
    move.w minYL,%d3        | d3 = minYL

	cmp.w %d2,%d3           | if (minYL > minYR)
	jle .cen_241

	move.w %d3,%d2          |   d2 = minYL

.cen_241:
    cmp.w %d1,%d2           | if (minY >= maxY) return 0
    jge .cen_end0

    move.w %d1,maxY         | maxY = d1
    move.w %d2,minY         | minY = d0

.cen_end1:
	moveq #1,%d0
	movm.l (%sp)+,%d2-%d7/%a2-%a6
	rts

.cen_end0:
	moveq #0,%d0
	movm.l (%sp)+,%d2-%d7/%a2-%a6
	rts



| u16 drawPolygon_new(u8 col)
|
|a0 = buf = &bmp_buffer_write[y]
|a1 = &buf[x]

|a2 = leftEdge
|a3 = rightEdge

|a4 = free use

|d0 = current col (32 bits extended)
|d1 = other col (32 bits extended)

|d2 = xl
|d3 = xr

|d4 = free use
|d5 = free use

|d6 = len


	.globl	drawPolygon_new
	.type	drawPolygon_new, @function
drawPolygon_new:

	move.w 6(%sp),%d0           | d0.w = col
	add.w %d0,%d0
	add.w %d0,%d0
    lea cnv_8to32_tab,%a0
    move.l (%a0,%d0.w),%d0      | d0 = col extended to 32 bits

	movm.l %d2-%d6/%a2-%a4,-(%sp)

    move.w minY,%d2             | d2 = minY
    move.w maxY,%d6
    sub.w %d2,%d6               | d6 = len
    subq.w #1,%d6
    jlt .dp_end                 | nothing to draw --> exit

    move.l %d0,%d1              | d1 = odd line col extended to 32 bits
    rol.l  #4,%d0               | d0 = even line col exchanged to 32 bits
    btst   #0,%d2               | odd line ?
    jeq    .dp_1

    exg    %d0,%d1              | change to odd color scheme

.dp_1:
    move.l leftEdge,%a2         | a2 = &leftEdge
    move.l rightEdge,%a3        | a3 = &rightEdge
    add.w %d2,%d2               | d2 = minY * 2
    add.w %d2,%a2               | a2 = edgeL = &leftEdge[minY]
    add.w %d2,%a3               | a3 = edgeR = &rightEdge[minY]

	move.l bmp_buffer_write,%a0
	lsl.w  #6,%d2
	add.w  %d2,%a0              | a0 = buf = &bmp_buffer_write[minY * BMP_PITCH]

                                | while (len--)
.dp_293:                        | {
    move.w (%a2)+,%d2           |   xl = *edgeL
    jge .dp_291

    moveq #0,%d2

.dp_291:
    move.w (%a3)+,%d3           |   xr = *edgeR
|    cmp.w #255,%d3
 |   jle .dp_292

  |  move.w #255,%d3

.dp_292:
	cmp.w %d2,%d3               |   if (xr < xl) continue;
    jlt .dp_175

.dp_290:
	asr.w  #1,%d2               |   a1 = dst = &buf[xl >> 1]
    lea    (%a0,%d2.w),%a1      |   if (xl & 1)
	jcc    .dp_179              |   {

	moveq  #-16,%d4
	and.b  (%a1),%d4            |     d4 = buf[xl >> 1] & 0xF0
	moveq  #15,%d5
	and.b  %d0,%d5              |     d5 = color & 0x0F
	or.b   %d4,%d5
	move.b %d5,(%a1)+           |     buf[xl >> 1] = (buf[xl >> 1] & 0xF0) | (color & 0x0F)
	addq.w #1,%d2               |     xl++
                                |   }
.dp_179:
	asr.w  #1,%d3               |   if (!(xr & 1))
	jcs    .dp_180              |   {

	moveq  #15,%d4
	and.b  (%a0,%d3.w),%d4      |     d4 = buf[xr >> 1] & 0x0F
	moveq  #-16,%d5
	and.b  %d0,%d5              |     d5 = color & 0xF0
	or.b   %d4,%d5
	move.b %d5,(%a0,%d3.w)      |     buf[xr >> 1] = (buf[xr >> 1] & 0x0F) | (color & 0xF0)
	subq.w #1,%d3               |     xr--
                                |   }
.dp_180:
	sub.w  %d2,%d3              |   d3 = width
	jlt .dp_175

    btst   #0,%d2               |   dst is byte aligned ?
	jeq    .dp_163

	move.b %d0,(%a1)+           |   align dst to word
	subq.w #1,%d3

.dp_163:
	add.w  %d3,%d3
	add.w  %d3,%d3              |   d3 = width * 4

.dp_fill_base:
    move.l (.dp_fill_table-.dp_fill_base)-2(%pc,%d3.w),%a4
    jmp (%a4)

    .align 4
    .long .dp_175
    .long .dp_175
    .long .dp_175
.dp_fill_table:
    .long .dp_fill_01
    .long .dp_fill_02
    .long .dp_fill_03
    .long .dp_fill_04
    .long .dp_fill_05
    .long .dp_fill_06
    .long .dp_fill_07
    .long .dp_fill_08
    .long .dp_fill_09
    .long .dp_fill_10
    .long .dp_fill_11
    .long .dp_fill_12
    .long .dp_fill_13
    .long .dp_fill_14
    .long .dp_fill_15
    .long .dp_fill_16
    .long .dp_fill_17
    .long .dp_fill_18
    .long .dp_fill_19
    .long .dp_fill_20
    .long .dp_fill_21
    .long .dp_fill_22
    .long .dp_fill_23
    .long .dp_fill_24
    .long .dp_fill_25
    .long .dp_fill_26
    .long .dp_fill_27
    .long .dp_fill_28
    .long .dp_fill_29
    .long .dp_fill_30
    .long .dp_fill_31
    .long .dp_fill_32
    .long .dp_fill_33
    .long .dp_fill_34
    .long .dp_fill_35
    .long .dp_fill_36
    .long .dp_fill_37
    .long .dp_fill_38
    .long .dp_fill_39
    .long .dp_fill_40
    .long .dp_fill_41
    .long .dp_fill_42
    .long .dp_fill_43
    .long .dp_fill_44
    .long .dp_fill_45
    .long .dp_fill_46
    .long .dp_fill_47
    .long .dp_fill_48
    .long .dp_fill_49
    .long .dp_fill_50
    .long .dp_fill_51
    .long .dp_fill_52
    .long .dp_fill_53
    .long .dp_fill_54
    .long .dp_fill_55
    .long .dp_fill_56
    .long .dp_fill_57
    .long .dp_fill_58
    .long .dp_fill_59
    .long .dp_fill_60
    .long .dp_fill_61
    .long .dp_fill_62
    .long .dp_fill_63
    .long .dp_fill_64
    .long .dp_fill_65
    .long .dp_fill_66
    .long .dp_fill_67
    .long .dp_fill_68
    .long .dp_fill_69
    .long .dp_fill_70
    .long .dp_fill_71
    .long .dp_fill_72
    .long .dp_fill_73
    .long .dp_fill_74
    .long .dp_fill_75
    .long .dp_fill_76
    .long .dp_fill_77
    .long .dp_fill_78
    .long .dp_fill_79
    .long .dp_fill_80
    .long .dp_fill_81
    .long .dp_fill_82
    .long .dp_fill_83
    .long .dp_fill_84
    .long .dp_fill_85
    .long .dp_fill_86
    .long .dp_fill_87
    .long .dp_fill_88
    .long .dp_fill_89
    .long .dp_fill_90
    .long .dp_fill_91
    .long .dp_fill_92
    .long .dp_fill_93
    .long .dp_fill_94
    .long .dp_fill_95
    .long .dp_fill_96
    .long .dp_fill_97
    .long .dp_fill_98
    .long .dp_fill_99
    .long .dp_fill_100
    .long .dp_fill_101
    .long .dp_fill_102
    .long .dp_fill_103
    .long .dp_fill_104
    .long .dp_fill_105
    .long .dp_fill_106
    .long .dp_fill_107
    .long .dp_fill_108
    .long .dp_fill_109
    .long .dp_fill_110
    .long .dp_fill_111
    .long .dp_fill_112
    .long .dp_fill_113
    .long .dp_fill_114
    .long .dp_fill_115
    .long .dp_fill_116
    .long .dp_fill_117
    .long .dp_fill_118
    .long .dp_fill_119
    .long .dp_fill_120
    .long .dp_fill_121
    .long .dp_fill_122
    .long .dp_fill_123
    .long .dp_fill_124
    .long .dp_fill_125
    .long .dp_fill_126
    .long .dp_fill_127
    .long .dp_fill_128
    .long .dp_fill_129
    .long .dp_fill_129
    .long .dp_fill_129

.dp_fill_128:
	move.l %d0,(%a1)+
.dp_fill_124:
	move.l %d0,(%a1)+
.dp_fill_120:
	move.l %d0,(%a1)+
.dp_fill_116:
	move.l %d0,(%a1)+
.dp_fill_112:
	move.l %d0,(%a1)+
.dp_fill_108:
	move.l %d0,(%a1)+
.dp_fill_104:
	move.l %d0,(%a1)+
.dp_fill_100:
	move.l %d0,(%a1)+
.dp_fill_96:
	move.l %d0,(%a1)+
.dp_fill_92:
	move.l %d0,(%a1)+
.dp_fill_88:
	move.l %d0,(%a1)+
.dp_fill_84:
	move.l %d0,(%a1)+
.dp_fill_80:
	move.l %d0,(%a1)+
.dp_fill_76:
	move.l %d0,(%a1)+
.dp_fill_72:
	move.l %d0,(%a1)+
.dp_fill_68:
	move.l %d0,(%a1)+
.dp_fill_64:
	move.l %d0,(%a1)+
.dp_fill_60:
	move.l %d0,(%a1)+
.dp_fill_56:
	move.l %d0,(%a1)+
.dp_fill_52:
	move.l %d0,(%a1)+
.dp_fill_48:
	move.l %d0,(%a1)+
.dp_fill_44:
	move.l %d0,(%a1)+
.dp_fill_40:
	move.l %d0,(%a1)+
.dp_fill_36:
	move.l %d0,(%a1)+
.dp_fill_32:
	move.l %d0,(%a1)+
.dp_fill_28:
	move.l %d0,(%a1)+
.dp_fill_24:
	move.l %d0,(%a1)+
.dp_fill_20:
	move.l %d0,(%a1)+
.dp_fill_16:
	move.l %d0,(%a1)+
.dp_fill_12:
	move.l %d0,(%a1)+
.dp_fill_08:
	move.l %d0,(%a1)+
.dp_fill_04:
	move.l %d0,(%a1)

.dp_175:
	lea 128(%a0),%a0
	exg %d0,%d1                 |   exchange color scheme
	dbra %d6,.dp_293            | }

.dp_end:
	movm.l (%sp)+,%d2-%d6/%a2-%a4
	rts

.dp_fill_126:
	move.l %d0,(%a1)+
.dp_fill_122:
	move.l %d0,(%a1)+
.dp_fill_118:
	move.l %d0,(%a1)+
.dp_fill_114:
	move.l %d0,(%a1)+
.dp_fill_110:
	move.l %d0,(%a1)+
.dp_fill_106:
	move.l %d0,(%a1)+
.dp_fill_102:
	move.l %d0,(%a1)+
.dp_fill_98:
	move.l %d0,(%a1)+
.dp_fill_94:
	move.l %d0,(%a1)+
.dp_fill_90:
	move.l %d0,(%a1)+
.dp_fill_86:
	move.l %d0,(%a1)+
.dp_fill_82:
	move.l %d0,(%a1)+
.dp_fill_78:
	move.l %d0,(%a1)+
.dp_fill_74:
	move.l %d0,(%a1)+
.dp_fill_70:
	move.l %d0,(%a1)+
.dp_fill_66:
	move.l %d0,(%a1)+
.dp_fill_62:
	move.l %d0,(%a1)+
.dp_fill_58:
	move.l %d0,(%a1)+
.dp_fill_54:
	move.l %d0,(%a1)+
.dp_fill_50:
	move.l %d0,(%a1)+
.dp_fill_46:
	move.l %d0,(%a1)+
.dp_fill_42:
	move.l %d0,(%a1)+
.dp_fill_38:
	move.l %d0,(%a1)+
.dp_fill_34:
	move.l %d0,(%a1)+
.dp_fill_30:
	move.l %d0,(%a1)+
.dp_fill_26:
	move.l %d0,(%a1)+
.dp_fill_22:
	move.l %d0,(%a1)+
.dp_fill_18:
	move.l %d0,(%a1)+
.dp_fill_14:
	move.l %d0,(%a1)+
.dp_fill_10:
	move.l %d0,(%a1)+
.dp_fill_06:
	move.l %d0,(%a1)+
.dp_fill_02:
	move.w %d0,(%a1)

	lea 128(%a0),%a0
	exg %d0,%d1                 |   exchange color scheme
	dbra %d6,.dp_293            | }

	movm.l (%sp)+,%d2-%d6/%a2-%a4
	rts

.dp_fill_129:
	move.l %d0,(%a1)+
.dp_fill_125:
	move.l %d0,(%a1)+
.dp_fill_121:
	move.l %d0,(%a1)+
.dp_fill_117:
	move.l %d0,(%a1)+
.dp_fill_113:
	move.l %d0,(%a1)+
.dp_fill_109:
	move.l %d0,(%a1)+
.dp_fill_105:
	move.l %d0,(%a1)+
.dp_fill_101:
	move.l %d0,(%a1)+
.dp_fill_97:
	move.l %d0,(%a1)+
.dp_fill_93:
	move.l %d0,(%a1)+
.dp_fill_89:
	move.l %d0,(%a1)+
.dp_fill_85:
	move.l %d0,(%a1)+
.dp_fill_81:
	move.l %d0,(%a1)+
.dp_fill_77:
	move.l %d0,(%a1)+
.dp_fill_73:
	move.l %d0,(%a1)+
.dp_fill_69:
	move.l %d0,(%a1)+
.dp_fill_65:
	move.l %d0,(%a1)+
.dp_fill_61:
	move.l %d0,(%a1)+
.dp_fill_57:
	move.l %d0,(%a1)+
.dp_fill_53:
	move.l %d0,(%a1)+
.dp_fill_49:
	move.l %d0,(%a1)+
.dp_fill_45:
	move.l %d0,(%a1)+
.dp_fill_41:
	move.l %d0,(%a1)+
.dp_fill_37:
	move.l %d0,(%a1)+
.dp_fill_33:
	move.l %d0,(%a1)+
.dp_fill_29:
	move.l %d0,(%a1)+
.dp_fill_25:
	move.l %d0,(%a1)+
.dp_fill_21:
	move.l %d0,(%a1)+
.dp_fill_17:
	move.l %d0,(%a1)+
.dp_fill_13:
	move.l %d0,(%a1)+
.dp_fill_09:
	move.l %d0,(%a1)+
.dp_fill_05:
	move.l %d0,(%a1)+
	move.b %d0,(%a1)

	lea 128(%a0),%a0
	exg %d0,%d1                 |   exchange color scheme
	dbra %d6,.dp_293            | }

	movm.l (%sp)+,%d2-%d6/%a2-%a4
	rts

.dp_fill_127:
	move.l %d0,(%a1)+
.dp_fill_123:
	move.l %d0,(%a1)+
.dp_fill_119:
	move.l %d0,(%a1)+
.dp_fill_115:
	move.l %d0,(%a1)+
.dp_fill_111:
	move.l %d0,(%a1)+
.dp_fill_107:
	move.l %d0,(%a1)+
.dp_fill_103:
	move.l %d0,(%a1)+
.dp_fill_99:
	move.l %d0,(%a1)+
.dp_fill_95:
	move.l %d0,(%a1)+
.dp_fill_91:
	move.l %d0,(%a1)+
.dp_fill_87:
	move.l %d0,(%a1)+
.dp_fill_83:
	move.l %d0,(%a1)+
.dp_fill_79:
	move.l %d0,(%a1)+
.dp_fill_75:
	move.l %d0,(%a1)+
.dp_fill_71:
	move.l %d0,(%a1)+
.dp_fill_67:
	move.l %d0,(%a1)+
.dp_fill_63:
	move.l %d0,(%a1)+
.dp_fill_59:
	move.l %d0,(%a1)+
.dp_fill_55:
	move.l %d0,(%a1)+
.dp_fill_51:
	move.l %d0,(%a1)+
.dp_fill_47:
	move.l %d0,(%a1)+
.dp_fill_43:
	move.l %d0,(%a1)+
.dp_fill_39:
	move.l %d0,(%a1)+
.dp_fill_35:
	move.l %d0,(%a1)+
.dp_fill_31:
	move.l %d0,(%a1)+
.dp_fill_27:
	move.l %d0,(%a1)+
.dp_fill_23:
	move.l %d0,(%a1)+
.dp_fill_19:
	move.l %d0,(%a1)+
.dp_fill_15:
	move.l %d0,(%a1)+
.dp_fill_11:
	move.l %d0,(%a1)+
.dp_fill_07:
	move.l %d0,(%a1)+
.dp_fill_03:
	move.w %d0,(%a1)+
.dp_fill_01:
	move.b %d0,(%a1)

	lea 128(%a0),%a0
	exg %d0,%d1                 |   exchange color scheme
	dbra %d6,.dp_293            | }

	movm.l (%sp)+,%d2-%d6/%a2-%a4
	rts
