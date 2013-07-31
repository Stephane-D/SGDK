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
    | d2 = x (32 bits extended)
    | d4 = dx
    | d5 = dy
    | a1 = dst
    |
    | OUT:
    | d2 = x updated
    | d4 = ?
    | d5 = dy * 4
    | a1 = dst (updated)
    | a6 = ?
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


    | internal use only
    | -----------------
    | IN:
    | d1 = len
    | d2 = (x << 16) | (x << 0)
    | a1 = dst
    |
    | OUT:
    | d1 = len * 4
    | d2 = (x << 16) | (x << 0)
    | a1 = dst (updated)
    | a6 = ?
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


    | edges calculation
    | -----------------
    |
    | d0 = dx
    | d1 = dy
    | d2 = yMin & x0
    | d3 = yMax & y0
    | d4 = x1
    | d5 = y1
    | d6 = BMP_HEIGHT-1
    | d7 = BMP_WIDTH-1
    | a0 = pt / pt1
    | a1 = edge / pt0
    | a2 = ptYMin
    | a3 = ptYMax
    | a4 = ptFirst
    | a5 = ptLast
    | a6 = free use
    |
    | polygon drawing
    | ---------------
    |
    | d0 = current col (32 bits extended)
    | d1 = other col (32 bits extended)
    | d2 = xl
    | d3 = xr
    | d4 = free use
    | d5 = free use
    | d6 = len
    | a0 = buf = &bmp_buffer_write[y]
    | a1 = &buf[x]
    | a2 = leftEdge
    | a3 = rightEdge
    | a4 = free use

	.globl	BMP_drawPolygon
	.type	BMP_drawPolygon, @function
BMP_drawPolygon:
	movm.l %d2-%d7/%a2-%a6,-(%sp)

	move.l 48(%sp),%a0      | a0 = pt = &pts[0]
	move.w 54(%sp),%d1      | d1 = num

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
    move.w %d3,%d6          |   d6 = maxY

	add.w %d3,%d3

    move.l leftEdge,%a0
	move.w %d4,(%a0,%d3)    |   leftEdge[yMin] = x0
    move.l rightEdge,%a0
	move.w %d5,(%a0,%d3)    |   rightEdge[yMin] = x1

	jra .drawPolygon        |   draw polygon now (d2 = minY and d6 = maxY)
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
    move.w maxYL,%d6        | d6 = 2 * maxYL

	cmp.w %a1,%d6           | if (maxYL > maxYR)
	jle .cen_240

	move.w %a1,%d6          |   d6 = maxYR

.cen_240:
    lsr.w #1,%d6
    subq.w #1,%d6           | d6 = maxY

    move.w minYR,%d2        | d2 = minYR
    move.w minYL,%d3        | d3 = minYL

	cmp.w %d2,%d3           | if (minYL > minYR)
	jle .drawPolygon

	move.w %d3,%d2          |   d2 = minYL

    | polygon drawing
    | ---------------

.drawPolygon:
    sub.w %d2,%d6               | d6 = len = maxY - minY
    jlt .cen_end0               | < 0 = nothing to draw --> exit

	move.w 58(%sp),%d0          | d0.w = col
	add.w %d0,%d0
	add.w %d0,%d0
    lea cnv_8to32_tab,%a0
    move.l (%a0,%d0.w),%d0      | d0 = col extended to 32 bits

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
|    jle .dp_292

|    move.w #255,%d3

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
	jlt    .dp_175

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

	moveq #1,%d0
	movm.l (%sp)+,%d2-%d7/%a2-%a6
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

	moveq #1,%d0
	movm.l (%sp)+,%d2-%d7/%a2-%a6
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

	moveq #1,%d0
	movm.l (%sp)+,%d2-%d7/%a2-%a6
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

	moveq #1,%d0
	movm.l (%sp)+,%d2-%d7/%a2-%a6
	rts

.cen_end0:
	moveq #0,%d0
	movm.l (%sp)+,%d2-%d7/%a2-%a6
	rts
