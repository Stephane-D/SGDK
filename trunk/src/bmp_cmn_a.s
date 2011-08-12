	.align	2
	.globl	BMP_clipLine
	.type	BMP_clipLine, @function
BMP_clipLine:
	movm.l #0x3f00,-(%sp)

	move.l 28(%sp),%a0          | a0 = line
	move.w (%a0),%d2            | d2 = x1
	move.w 2(%a0),%d3           | d3 = y1
	move.w 4(%a0),%d4           | d4 = x2
	move.w 6(%a0),%d5           | d5 = y2

	moveq #127,%d0              | d0 = BMP_WIDTH - 1
	move.w #159,%d1             | d1 = BMP_HEIGHT - 1

	cmp.w %d0,%d2               | if (((u16) x1 < BMP_WIDTH) &&
	jbhi .L30
	cmp.w %d0,%d4               |     ((u16) x2 < BMP_WIDTH) &&
	jbhi .L30
	cmp.w %d1,%d3               |     ((u16) y1 < BMP_HEIGHT) &&
	jbhi .L30
	cmp.w %d1,%d5               |     ((u16) y2 < BMP_HEIGHT))
	jbhi .L30

	moveq #1,%d0                |   return 1;
	movm.l (%sp)+,#0x0fc
	rts

.L30:
	tst.w %d2                   | if (((x1 < 0) && (x2 < 0)) ||
	jbge .L33
	tst.w %d4
	jblt .L32

.L33:
	cmp.w %d0,%d2               |     ((x1 >= BMP_WIDTH) && (x2 >= BMP_WIDTH)) ||
	jble .L34
	cmp.w %d0,%d4
	jbgt .L32

.L34:
	tst.w %d3                   |     ((y1 < 0) && (y2 < 0)) ||
	jbge .L35
	tst.w %d5
	jblt .L32

.L35:
	cmp.w %d1,%d3               |     ((y1 >= BMP_HEIGHT) && (y2 >= BMP_HEIGHT)))
	jble .L60
	cmp.w %d1,%d5
	jble .L60

.L32:
	moveq #0,%d0                |   return 0;
	movm.l (%sp)+,#0x0fc
	rts

.L60:
	move.w %d4,%d6
	sub.w %d2,%d6               | d6 = dx = x2 - x1;
	move.w %d5,%d7
	sub.w %d3,%d7               | d7 = dy = y2 - y1;

	tst.w %d2                   | if (x1 < 0)
	jbge .L38                   | {

	muls.w %d7,%d2              |   y1 -= (x1 * dy) / dx;
	divs.w %d6,%d2
	sub.w %d2,%d3
	moveq #0,%d2                |   x1 = 0;
	jbra .L39                   | }

	.align	2
.L38:
	cmp.w %d0,%d2               | else if (x1 >= BMP_WIDTH)
	jble .L39                   | {

    sub.w %d2,%d0
	muls.w %d7,%d0              |   y1 += (((BMP_WIDTH - 1) - x1) * dy) / dx;
	divs.w %d6,%d0
	add.w %d0,%d3
	moveq #127,%d2              |   x1 = BMP_WIDTH - 1;
	move.w %d2,%d0              |   d0 = BMP_WIDTH - 1
                                | }
.L39:
	tst.w %d4                   | if (x2 < 0)
	jbge .L41                   | {

	muls.w %d7,%d4              |   y2 -= (x2 * dy) / dx;
	divs.w %d6,%d4
	sub.w %d4,%d5
	moveq #0,%d4                |   x2 = 0;
	jbra .L42                   | }

.L41:
	cmp.w %d0,%d4               | else if (x2 >= BMP_WIDTH)
	jble .L42                   | {

    sub.w %d4,%d0
	muls.w %d7,%d0              |   y2 += (((BMP_WIDTH - 1) - x2) * dy) / dx;
	divs.w %d6,%d0
	add.w %d0,%d5
	moveq #127,%d4              |   x2 = BMP_WIDTH - 1;
	move.w %d4,%d0              |   d0 = BMP_WIDTH - 1
                                | }
.L42:
	tst.w %d3                   | if (y1 < 0)
	jbge .L44                   | {

	muls.w %d6,%d3              |   x1 -= (y1 * dx) / dy;
	divs.w %d7,%d3
	sub.w %d3,%d2
	moveq #0,%d3                |   y1 = 0;
	jbra .L45                   | }

.L44:
	cmp.w %d1,%d3               | else if (y1 >= BMP_HEIGHT)
	jble .L45                   | {

    sub.w %d3,%d1
	muls.w %d6,%d1              |   x1 += (((BMP_HEIGHT - 1) - y1) * dx) / dy;
	divs.w %d7,%d1
	add.w %d1,%d2
	move.w #159,%d3             |   y1 = BMP_HEIGHT - 1;
	move.w %d3,%d1              |   d1 = BMP_HEIGHT - 1


.L45:	                        | }
	tst.w %d5                   | if (y2 < 0)
	jbge .L47                   | {

	muls.w %d6,%d5              |   x2 -= (y2 * dx) / dy;
	divs.w %d7,%d5
	sub.w %d5,%d4
	moveq #0,%d5                |   y2 = 0;
	jbra .L48                   | }

.L47:
	cmp.w %d1,%d5               | else if (y2 >= BMP_HEIGHT)
	jble .L48                   | {

    sub.w %d5,%d1
	muls.w %d6,%d1              |   x2 += (((BMP_HEIGHT - 1) - y2) * dx) / dy;
	divs.w %d7,%d1
	add.w %d1,%d4
	move.w #159,%d5             |   y2 = BMP_HEIGHT - 1;
	move.w %d5,%d1              |   d1 = BMP_HEIGHT - 1
                                | }

.L48:
	cmp.w %d0,%d2               | if (((u16) x1 < BMP_WIDTH) &&
	jbhi .L50
	cmp.w %d0,%d4               |     ((u16) x2 < BMP_WIDTH) &&
	jbhi .L50
	cmp.w %d1,%d3               |     ((u16) y1 < BMP_HEIGHT) &&
	jbhi .L50
	cmp.w %d1,%d5               |     ((u16) y2 < BMP_HEIGHT))
	jbhi .L50                   | {

	move.w %d2,(%a0)            |   l->pt1.x = x1;
	move.w %d3,2(%a0)           |   l->pt1.y = y1;
	move.w %d4,4(%a0)           |   l->pt2.x = x2;
	move.w %d5,6(%a0)           |   l->pt2.y = y2;
	moveq #1,%d0
	movm.l (%sp)+,#0x0fc        |   return 1;
	rts                         | }

.L50:
	tst.w %d2                   | if (((x1 < 0) && (x2 < 0)) ||
	jbge .L53
	tst.w %d4
	jblt .L52

.L53:
	cmp.w %d0,%d2               |     ((x1 >= BMP_WIDTH) && (x2 >= BMP_WIDTH)) ||
	jble .L54
	cmp.w %d0,%d4
	jbgt .L52

.L54:
	tst.w %d3                   |     ((y1 < 0) && (y2 < 0)) ||
	jbge .L55
	tst.w %d5
	jblt .L52

.L55:
	cmp.w %d1,%d3               |     ((y1 >= BMP_HEIGHT) && (y2 >= BMP_HEIGHT)))
	jble .L60
	cmp.w %d1,%d5
	jble .L60

.L52:
	moveq #0,%d0                |   return 0;
	movm.l (%sp)+,#0x0fc        | }
	rts


	.align	2
	.globl	calculatePolyEdge
	.type	calculatePolyEdge, @function
calculatePolyEdge:
	movm.l #0x3f00,-(%sp)

	move.l 28(%sp),%a0          | a0 = pt1
	move.l 32(%sp),%a1          | a1 = pt2

	move.w (%a0),%d2            | d2 = x1
	move.w 2(%a0),%d3           | d3 = y1
	move.w (%a1),%d4            | d4 = x2
	move.w 2(%a1),%d5           | d5 = y2

    moveq #127,%d0              | d0 = BMP_WIDTH - 1
	move.w #159,%d1             | d1 = BMP_HEIGHT - 1

	tst.w %d2                   | if (((x1 < 0) && (x2 < 0)) ||
	jbge .L64
	tst.w %d4
	jblt .L61

.L64:
	cmp.w %d0,%d2               |     ((x1 >= BMP_WIDTH) && (x2 >= BMP_WIDTH)) ||
	jble .L65
	cmp.w %d0,%d4
	jbgt .L61

.L65:
	tst.w %d3                   |     ((y1 < 0) && (y2 < 0)) ||
	jbge .L66
	tst.w %d5
	jblt .L61

.L66:
	cmp.w %d1,%d3               |     ((y1 >= BMP_HEIGHT) && (y2 >= BMP_HEIGHT)))
	jble .L62
	cmp.w %d1,%d5
	jbgt .L61                   |       return;

.L62:
	move.w %d5,%d7              | d7 = dy = y2 - y1;
	sub.w %d3,%d7
	jbeq .L61                   | if (dy == 0) return;

	move.w %d4,%d6
	sub.w %d2,%d6               | d6 = dx = x2 - x1;

	tst.w %d3                   | if (y1 < 0)
	jbge .L68                   | {

	muls.w %d6,%d3              |   x1 -= (y1 * dx) / dy;
	divs.w %d7,%d3
	sub.w %d3,%d2
	moveq #0,%d3                |   y1 = 0;
	jbra .L69                   | }

.L68:
	cmp.w %d1,%d3               | else if (y1 >= BMP_HEIGHT)
	jble .L69                   | {

    sub.w %d3,%d1
	muls.w %d6,%d1              |   x1 += (((BMP_HEIGHT - 1) - y1) * dx) / dy;
	divs.w %d7,%d1
	add.w %d1,%d2
	move.w #159,%d3             |   y1 = BMP_HEIGHT - 1;
	move.w %d3,%d1              |   d1 = BMP_HEIGHT - 1;

.L69:	                        | }
	tst.w %d5                   | if (y2 < 0)
	jbge .L71                   | {

	muls.w %d6,%d5              |   x2 -= (y2 * dx) / dy;
	divs.w %d7,%d5
	sub.w %d5,%d4
	moveq #0,%d5                |   y2 = 0;
	jbra .L72                   | }

.L71:
	cmp.w %d1,%d5               | else if (y2 >= BMP_HEIGHT)
	jble .L72                   | {

    sub.w %d5,%d1
	muls.w %d6,%d1              |   x2 += (((BMP_HEIGHT - 1) - y2) * dx) / dy;
	divs.w %d7,%d1
	add.w %d1,%d4
	move.w #159,%d5             |   y2 = BMP_HEIGHT - 1;
	move.w %d5,%d1              |   d1 = BMP_HEIGHT - 1
                                | }

.L72:
	tst.w %d2                   | if (((x1 < 0) && (x2 < 0)) ||
	jbge .L76
	tst.w %d4
	jblt .L61

.L76:
	cmp.w %d0,%d2               |     ((x1 >= BMP_WIDTH) && (x2 >= BMP_WIDTH)) ||
	jble .L77
	cmp.w %d0,%d4
	jbgt .L61

.L77:
	tst.w %d3                   |     ((y1 < 0) && (y2 < 0)) ||
	jbge .L78
	tst.w %d5
	jblt .L61

.L78:
	cmp.w %d1,%d3               |     ((y1 >= BMP_HEIGHT) && (y2 >= BMP_HEIGHT)))
	jble .L74
	cmp.w %d1,%d5
	jbgt .L61                   |       return;

.L74:
	cmp.w %d5,%d3               | if (y1 == y2) return;
	jbeq .L61

    jbge .L81                   | if ((y2 > y1) ^ (clockwise))
	tst.b 39(%sp)               | {
	jbeq .L82
	jbra .L80
.L81:
	tst.b 39(%sp)
	jbeq .L80

.L82:
	move.w %d4,%d6
	sub.w %d2,%d6               |   d6 = dx = x2 - x1;
	move.w %d5,%d7
	sub.w %d3,%d7               |   d7 = len = y2 - y1;

	cmp.w minY.l,%d3            |   if (y1 < minY)
	jbge .L83

	move.w %d3,minY             |       minY = y1;

.L83:
	cmp.w maxY.l,%d5            |   if (y2 > maxY)
	jble .L84

	move.w %d5,maxY             |       maxY = y2;

.L84:
    ext.l %d3
    add.w %d3,%d3
    add.l LeftPoly,%d3
    move.l %d3,%a0              |   a0 = src = &LeftPoly[y1];
    move.w %d2,%d0              |   d0 = x = x1
	jbra .L85                   | }
                                | else
.L80:                           | {
	move.w %d2,%d6
	sub.w %d4,%d6               |   d6 = dx = x1 - x2;
	move.w %d3,%d7
	sub.w %d5,%d7               |   d7 = len = y1 - y2;

	cmp.w minY.l,%d5            |   if (y2 < minY)
	jbge .L86

	move.w %d5,minY             |       minY = y2;

.L86:
	cmp.w maxY.l,%d3            |   if (y1 > maxY)
	jble .L87

	move.w %d3,maxY             |       maxY = y1;

.L87:
    ext.l %d5
    add.w %d5,%d5
    add.l RightPoly,%d5
    move.l %d5,%a0              |   a0 = src = &RightPoly[y2];
    move.w %d4,%d0              |   d0 = x = x2
                                | }

.L85:
	ext.l %d6
	lsl.l #7,%d6                | d6 = dx << 7
	divs.w %d7,%d6              | d6 = (dx << 7) / len
	ext.l %d6                   | d6 = step << 7
	ror.l #8,%d6                | d6 = step >> 1 (32 bits fixed point)
	ext.l %d0                   | d0 = x (32 bits fixed point)
	add.l %d6,%d0               | d0 = x + (step >> 1)
	rol.l #1,%d6                | d6 = step (32 bits fixed point)
	movq #0,%d2                 | d2 = 0

    move.w %d7,%d1
	asr.w #3,%d1                | d1 = len8 = len >> 3
	and.w #7,%d7                | d7 = len = len & 7
	subq.w #1,%d1
	jbmi .L95

	move %d2,%ccr               | restore X flag

.L90:                           | while(len8--)
	move.w %d0,(%a0)+           | {
	addx.l %d6,%d0
	move.w %d0,(%a0)+           |   *src++ = fix16ToInt(x);
	addx.l %d6,%d0              |   x += step;
	move.w %d0,(%a0)+           |   ...
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

	dbra.W %d1,.L90             | }

	move %sr,%d2                | save X flag

.L95:
	subq.w #1,%d7
	jbmi .L61

	move %d2,%ccr               | restore X flag

.L93:                           | while(len--)
	move.w %d0,(%a0)+           | {
	addx.l %d6,%d0              |   *src++ = fix16ToInt(x);
	dbra.W %d7,.L93             |   x += step;
                                | }

.L61:
	movm.l (%sp)+,#0x0fc
	rts

