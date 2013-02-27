	.align	2
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

	move.w #255,%d0             | d0 = BMP_WIDTH - 1
	move.w #159,%d1             | d1 = BMP_HEIGHT - 1

	cmp.w %d0,%d2               | if (((u16) x1 < BMP_WIDTH) &&
	jhi .L30
	cmp.w %d0,%d4               |     ((u16) x2 < BMP_WIDTH) &&
	jhi .L30
	cmp.w %d1,%d3               |     ((u16) y1 < BMP_HEIGHT) &&
	jhi .L30
	cmp.w %d1,%d5               |     ((u16) y2 < BMP_HEIGHT))
	jhi .L30

	moveq #1,%d0                |   return 1;
	movm.l (%sp)+,#0x0fc
	rts

.L30:
	tst.w %d2                   | if (((x1 < 0) && (x2 < 0)) ||
	jge .L33
	tst.w %d4
	jlt .L32

.L33:
	cmp.w %d0,%d2               |     ((x1 >= BMP_WIDTH) && (x2 >= BMP_WIDTH)) ||
	jle .L34
	cmp.w %d0,%d4
	jgt .L32

.L34:
	tst.w %d3                   |     ((y1 < 0) && (y2 < 0)) ||
	jge .L35
	tst.w %d5
	jlt .L32

.L35:
	cmp.w %d1,%d3               |     ((y1 >= BMP_HEIGHT) && (y2 >= BMP_HEIGHT)))
	jle .L60
	cmp.w %d1,%d5
	jle .L60

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
	jge .L38                    | {

	muls.w %d7,%d2              |   y1 -= (x1 * dy) / dx;
	divs.w %d6,%d2
	sub.w %d2,%d3
	moveq #0,%d2                |   x1 = 0;
	jra .L39                    | }

	.align	2
.L38:
	cmp.w %d0,%d2               | else if (x1 >= BMP_WIDTH)
	jle .L39                    | {

    sub.w %d2,%d0
	muls.w %d7,%d0              |   y1 += (((BMP_WIDTH - 1) - x1) * dy) / dx;
	divs.w %d6,%d0
	add.w %d0,%d3
	move.w #255,%d2             |   x1 = BMP_WIDTH - 1;
	move.w %d2,%d0              |   d0 = BMP_WIDTH - 1
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

    sub.w %d4,%d0
	muls.w %d7,%d0              |   y2 += (((BMP_WIDTH - 1) - x2) * dy) / dx;
	divs.w %d6,%d0
	add.w %d0,%d5
	move.w #255,%d4             |   x2 = BMP_WIDTH - 1;
	move.w %d4,%d0              |   d0 = BMP_WIDTH - 1
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

    sub.w %d3,%d1
	muls.w %d6,%d1              |   x1 += (((BMP_HEIGHT - 1) - y1) * dx) / dy;
	divs.w %d7,%d1
	add.w %d1,%d2
	move.w #159,%d3             |   y1 = BMP_HEIGHT - 1;
	move.w %d3,%d1              |   d1 = BMP_HEIGHT - 1


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

    sub.w %d5,%d1
	muls.w %d6,%d1              |   x2 += (((BMP_HEIGHT - 1) - y2) * dx) / dy;
	divs.w %d7,%d1
	add.w %d1,%d4
	move.w #159,%d5             |   y2 = BMP_HEIGHT - 1;
	move.w %d5,%d1              |   d1 = BMP_HEIGHT - 1
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

	move.w %d2,(%a0)            |   l->pt1.x = x1;
	move.w %d3,2(%a0)           |   l->pt1.y = y1;
	move.w %d4,4(%a0)           |   l->pt2.x = x2;
	move.w %d5,6(%a0)           |   l->pt2.y = y2;
	moveq #1,%d0
	movm.l (%sp)+,#0x0fc        |   return 1;
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

    move.w #255,%d0             | d0 = BMP_WIDTH - 1
	move.w #159,%d1             | d1 = BMP_HEIGHT - 1

	move.w %d5,%d7              | d7 = dy = y2 - y1;
	sub.w %d3,%d7
|	jeq .L61                    | if (dy == 0) return;

	move.w %d4,%d6
	sub.w %d2,%d6               | d6 = dx = x2 - x1;

.L62:
	tst.w %d3                   | if (y1 < 0)
	jge .L68                    | {
	tst.w %d5                   |     if (y2 < 0)
	jlt .L61                    |         return;

	muls.w %d6,%d3              |     x1 -= (y1 * dx) / dy;
	divs.w %d7,%d3
	sub.w %d3,%d2
	moveq #0,%d3                |     y1 = 0;
	jra .L71                    |     goto L71;
                                | }
.L68:
	cmp.w %d1,%d3               | if (y1 >= BMP_HEIGHT)
	jle .L69                    | {
	cmp.w %d1,%d5               |     if (y2 >= BMP_HEIGHT)
	jgt .L61                    |         return;

    sub.w %d3,%d1
	muls.w %d6,%d1              |     x1 += (((BMP_HEIGHT - 1) - y1) * dx) / dy;
	divs.w %d7,%d1
	add.w %d1,%d2
	move.w #159,%d3             |     y1 = BMP_HEIGHT - 1;
	move.w %d3,%d1              |     d1 = BMP_HEIGHT - 1;

	tst.w %d5                   |     if (y2 < 0)
	jge .L72                    |     {

	muls.w %d6,%d5              |         x2 -= (y2 * dx) / dy;
	divs.w %d7,%d5
	sub.w %d5,%d4
	moveq #0,%d5                |         y2 = 0;
	jra .L72                    |     }
                                |     goto L72;
.L69:	                        | }
	tst.w %d5                   | if (y2 < 0)
	jge .L71                    | {

	muls.w %d6,%d5              |     x2 -= (y2 * dx) / dy;
	divs.w %d7,%d5
	sub.w %d5,%d4
	moveq #0,%d5                |     y2 = 0;
	jra .L72                    |     goto L72;
                                | }
.L71:
	cmp.w %d1,%d5               | if (y2 >= BMP_HEIGHT)
	jle .L72                    | {

    sub.w %d5,%d1
	muls.w %d6,%d1              |     x2 += (((BMP_HEIGHT - 1) - y2) * dx) / dy;
	divs.w %d7,%d1
	add.w %d1,%d4
	move.w #159,%d5             |     y2 = BMP_HEIGHT - 1;
	move.w %d5,%d1              |     d1 = BMP_HEIGHT - 1
                                | }

.L72:
	cmp.w %d5,%d3               | if (y1 == y2) return;
	jeq .L61

    jge .L80                    | if (y2 > y1)      // right edge

.L82:                           | {
	tst.w %d2                   |     if (((x1 < 0) && (x2 < 0))
	jpl .L77                    |         return;
	tst.w %d4
	jmi .L61

.L77:
	move.w %d4,%d6
	sub.w %d2,%d6               |     d6 = dx = x2 - x1;
	move.w %d5,%d7
	sub.w %d3,%d7               |     d7 = len = y2 - y1;

	cmp.w minYR.l,%d3           |     if (y1 < minYR)
	jge .L83

	move.w %d3,minYR            |         minYR = y1;

.L83:
	cmp.w maxYR.l,%d5           |     if (y2 > maxYR)
	jle .L84

	move.w %d5,maxYR            |         maxYR = y2;

.L84:
    ext.l %d3
    add.w %d3,%d3
    add.l RightPoly,%d3
    move.l %d3,%a0              |     a0 = src = &RightPoly[y1];
    move.w %d2,%d0              |     d0 = x = x1
	jra .L85                    | }
                                | else      // left edge
.L80:                           | {
	cmp.w %d0,%d2               |     ((x1 >= BMP_WIDTH) && (x2 >= BMP_WIDTH))
	jle .L76                    |         return;
	cmp.w %d0,%d4
	jgt .L61

.L76:
	move.w %d2,%d6
	sub.w %d4,%d6               |     d6 = dx = x1 - x2;
	move.w %d3,%d7
	sub.w %d5,%d7               |     d7 = len = y1 - y2;

	cmp.w minYL.l,%d5           |     if (y2 < minYL)
	jge .L86

	move.w %d5,minYL            |         minYL = y2;

.L86:
	cmp.w maxYL.l,%d3           |     if (y1 > maxYL)
	jle .L87

	move.w %d3,maxYL            |         maxYL = y1;

.L87:
    ext.l %d5
    add.w %d5,%d5
    add.l LeftPoly,%d5
    move.l %d5,%a0              |     a0 = src = &LeftPoly[y2];
    move.w %d4,%d0              |     d0 = x = x2
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

	dbra.W %d1,.L90             | }

	move %sr,%d2                | save X flag

.L95:
	subq.w #1,%d7
	jbmi .L61

	move %d2,%ccr               | restore X flag

.L93:                           | while(len--)
	move.w %d0,(%a0)+           | {
	addx.l %d6,%d0              |     *src++ = fix16ToInt(x);
	dbra.W %d7,.L93             |     x += step;
                                | }

.L61:
	movm.l (%sp)+,#0x0fc
	rts


	.globl	drawPolygon
	.type	drawPolygon, @function
drawPolygon:
	move.w maxY,%d1
	move.w minY,%d0             | d0 = minY
	sub.w %d0,%d1               | d1 = len
	jgt .L169

	rts

.L169:
	movm.l %d2-%d7/%a2-%a6,-(%sp)
	move.b 51(%sp),%d5          | d5 = col

    moveq  #15,%d6              | d6 = col low nibble mask
    moveq  #-16,%d7             | d7 = col high nibble mask

    move.w %d5,%d4
    lsl.w  #8,%d4
    move.b %d5,%d4
    move.w %d4,%d5
    swap   %d5
    move.w %d4,%d5              | d5 = col extended to 32 bits

    move.l %d5,%a5              | a5 = col extended to 32 bits (save)
    rol.l  #4,%d5               | d5 = col with exchanged nibble
    btst   #0,%d0               | odd line ?
    jeq    .L168

    exg    %d5,%a5              | use according color scheme

.L168:
    add.w  %d0,%d0
    ext.l  %d0                  | d0.l = 2 * minY

	move.l LeftPoly,%a2
	add.l  %d0,%a2              | a2 = left = &LeftPoly[minY]
	move.l RightPoly,%a3
	add.l  %d0,%a3              | a3 = right = &RightPoly[minY]
	move.l bmp_buffer_write,%a4
	lsl.l  #6,%d0
	add.l  %d0,%a4              | a4 = buf = &bmp_buffer_write[minY * BMP_PITCH]

	move.w #255,%a6             | a6 = BMP_WIDTH - 1
	subq.w #1,%d1               | d1 = remaining y

.L180:
	move.w (%a2)+,%d2           | d2 = x1 = *left++
	move.w (%a3)+,%d3           | d3 = x2 = *rigth++

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

	move.b (%a4,%d3.w),%d0
	and.b  %d6,%d0
	move.b %d5,%d4
	and.b  %d7,%d4
	or.b   %d4,%d0
	move.b %d0,(%a4,%d3.w)      |     buf[x2 >> 1] = (buf[x2 >> 1] & 0x0F) | (color & 0xF0)
	subq.w #1,%d3               |     x2--;
                                |   }
.L179:
	asr.w  #1,%d2               |   a1 = dst = &buf[x1 >> 1]
    lea    (%a4,%d2.w),%a1      |   if (x1 & 1)
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

	lea 128(%a4),%a4
	exg %d5,%a5                 | exchange color nibble
	dbra %d1,.L180
	jra .L171

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

	lea 128(%a4),%a4
	exg %d5,%a5                 | exchange color nibble
	dbra %d1,.L180
	jra .L171

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

	lea 128(%a4),%a4
	exg %d5,%a5                 | exchange color nibble
	dbra %d1,.L180
	jra .L171

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
	lea 128(%a4),%a4
	exg %d5,%a5                 | exchange color nibble
	dbra %d1,.L180

.L171:
	movm.l (%sp)+,%d2-%d7/%a2-%a6

.L170:
	rts
