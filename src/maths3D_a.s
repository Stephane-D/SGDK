	.extern	viewport
	.globl	M3D_project3D_f16
	.type	M3D_project3D_f16, @function
M3D_project3D_f16:
	movm.l #0x3e00,-(%sp)

	move.w viewport_f16,%d4
	asr.w #1,%d4                            | d4 = wi = viewport_f16.x >> 1;
	move.w viewport_f16+2,%d3
	asr.w #1,%d3                            | d3 = hi = viewport_f16.y >> 1;
	move.l 24(%sp),%a0                      | a0 = s = src
	move.l 28(%sp),%a1                      | a1 = d = dst
	move.w 34(%sp),%d2                      | d2 = i = numv

	move.w camDist,%d5
	ext.l %d5
	lsl.l #6,%d5                            | d5 = camDist << 6

	moveq #-64,%d6
	swap %d6
	move.w #-64,%d6                         | d6 = (FIX16(-1) << 16) | FIX16(-1)

	subq.w #1,%d2
	jmi .L34                                | while(i--)

.L32:                                       | {
	move.w 4(%a0),%d0                       |   if ((zi = s->z))
	jeq .L30                                |   {

    move.l %d5,%d1
    divs.w %d0,%d1                          |       d1 = zi = fix16Div(camDist, zi);

	move.w (%a0),%d0
	asr.w #1,%d0
	muls.w %d1,%d0
	asr.l #6,%d0                            |       d0 = fix16Mul(s->x >> 1, zi)
	add.w %d4,%d0                           |
	move.w %d0,(%a1)+                       |       d->x = wi + fix16Mul(s->x >> 1, zi);

	muls.w 2(%a0),%d1
	asr.l #6,%d1                            |       d1 = fix16Mul(s->y, zi)
	move.w %d3,%d0
	sub.w %d1,%d0
	move.w %d0,(%a1)+                       |       d->y = hi - fix16Mul(s->y, zi);

	addq.l #6,%a0
	dbra %d2,.L32

	movm.l (%sp)+,#0x07c
	rts                                     |   }
                                            |   else
.L30:                                       |   {
	move.l %d6,(%a1)+                       |       d->x = FIX16(-1);
                                            |       d->y = FIX16(-1);
	addq.l #6,%a0                           |   }
	dbra %d2,.L32                           | }

.L34:
	movm.l (%sp)+,#0x07c
	rts


	.globl	M3D_project3D_s16
	.type	M3D_project3D_s16, @function
M3D_project3D_s16:
	movm.l #0x3e00,-(%sp)

	move.w viewport,%d4
	lsr.w #1,%d4                            | d4 = wi = viewport.x >> 1;
	move.w viewport+2,%d3
	lsr.w #1,%d3                            | d3 = hi = viewport.y >> 1;
	move.l 24(%sp),%a0                      | a0 = s = src
	move.l 28(%sp),%a1                      | a1 = d = dst
	move.w 34(%sp),%d2                      | d2 = i = numv

	move.w camDist,%d5
	ext.l %d5
	lsl.l #6,%d5                            | d5 = camDist << 6

	moveq #-1,%d6
	swap %d6
	move.w #-1,%d6                          | d6 = (-1 << 16) | -1

	subq.w #1,%d2
	jmi .L42

.L40:
	move.w 4(%a0),%d0                       |   if ((zi = s->z))
	jeq .L38

    move.l %d5,%d1
    divs.w %d0,%d1                          |       d1 = zi = fix16Div(camDist, zi);

	move.w (%a0),%d0
	asr.w #1,%d0
	muls.w %d1,%d0
	swap %d0
	rol.l #4,%d0                            |       d0 = fix16ToInt(fix16Mul(s->x >> 1, zi))
	add.w %d4,%d0                           |
	move.w %d0,(%a1)+                       |       d->x = wi + fix16ToInt(fix16Mul(s->x >> 1, zi))

	muls.w 2(%a0),%d1
	swap %d1
	rol.l #4,%d1                            |       d1 = fix16ToInt(fix16Mul(s->y, zi))
	move.w %d3,%d0
	sub.w %d1,%d0
	move.w %d0,(%a1)+                       |       d->y = hi - fix16ToInt(fix16Mul(s->y, zi))

	addq.l #6,%a0
	dbra %d2,.L40

	movm.l (%sp)+,#0x07c
	rts                                     |   }
                                            |   else
.L38:
	move.l %d6,(%a1)+                       |       d->x = FIX16(-1);
                                            |       d->y = FIX16(-1);
	addq.l #6,%a0                           |   }
	dbra %d2,.L40                           | }

.L42:
	movm.l (%sp)+,#0x07c
	rts

