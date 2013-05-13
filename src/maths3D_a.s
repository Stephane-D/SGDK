	.extern	viewport

	.globl	M3D_transform
	.type	M3D_transform, @function
M3D_transform:
	movm.l #0x3f30,-(%sp)

	move.l 36(%sp),%a2              | a2 = transform
	move.w 50(%sp),%d5              | d5 = numv

	tst.w (%a2)
	jeq .L45

	move.l %a2,-(%sp)
	jbsr M3D_buildMat3D
	addq.l #4,%sp

.L45:
	move.l 2(%a2),%a0               | a0 = translation
	move.w (%a0),%a3                | a3 = translation.x
	move.w 2(%a0),%d7               | d7 = translation.y
	move.w 4(%a0),%d6               | d6 = translation.z

	move.l 40(%sp),%a1              | a1 = src
	move.l 44(%sp),%a0              | a0 = dest
	lea    10(%a2),%a2              | a2 = &(transform.mat)

	subq.w #1,%d5
	jmi    .L50

.L48:
	move.w (%a1)+,%d2               | d2 = sx
	move.w (%a1)+,%d3               | d3 = sy
	move.w (%a1)+,%d4               | d4 = sz

	move.w (%a2)+,%d1               | d1 = mat.a.x
	muls.w %d2,%d1                  | d1 = (mat.a.x * sx)
	move.w (%a2)+,%d0               | d0 = mat.a.y
	muls.w %d3,%d0                  | d0 = mat.a.y * sy
	add.l %d0,%d1                   | d1 = (mat.a.x * sx) + (mat.a.y * sy)
	move.w (%a2)+,%d0               | d0 = mat.a.z
	muls.w %d4,%d0                  | d0 = mat.a.z * sz
	add.l %d0,%d1                   | d1 = (mat.a.x * sx) + (mat.a.y * sy) + (mat.a.z * sz)
	asr.l #6,%d1
	add.w %a3,%d1                   | d1 = (mat.a.x * sx) + (mat.a.y * sy) + (mat.a.z * sz) + translation.x
	move.w %d1,(%a0)+               | dest++ = (mat.a.x * sx) + (mat.a.y * sy) + (mat.a.z * sz) + translation.x

	move.w (%a2)+,%d1               | d1 = mat.b.x
	muls.w %d2,%d1                  | d1 = (mat.b.x * sx)
	move.w (%a2)+,%d0               | d0 = mat.b.y
	muls.w %d3,%d0                  | d0 = mat.b.y * sy
	add.l %d0,%d1                   | d1 = (mat.b.x * sx) + (mat.b.y * sy)
	move.w (%a2)+,%d0               | d0 = mat.b.z
	muls.w %d4,%d0                  | d0 = mat.b.z * sz
	add.l %d0,%d1                   | d1 = (mat.b.x * sx) + (mat.b.y * sy) + (mat.b.z * sz)
	asr.l #6,%d1
	add.w %d7,%d1                   | d1 = (mat.b.x * sx) + (mat.b.y * sy) + (mat.b.z * sz) + translation.y
	move.w %d1,(%a0)+               | dest++ = (mat.b.x * sx) + (mat.b.y * sy) + (mat.b.z * sz) + translation.y

	muls.w (%a2)+,%d2               | d2 = mat.c.x * sx
	muls.w (%a2)+,%d3               | d3 = mat.c.y * sy
	add.l %d3,%d2                   | d2 = (mat.c.x * sx) + (mat.c.y * sy)
	muls.w (%a2),%d4                | d4 = mat.c.z * sz
	add.l %d4,%d2                   | d2 = (mat.c.x * sx) + (mat.c.y * sy) + (mat.c.z * sz)
	asr.l #6,%d2
	add.w %d6,%d2                   | d2 = (mat.c.x * sx) + (mat.c.y * sy) + (mat.c.z * sz) + translation.z
	move.w %d2,(%a0)+               | dest++ = (mat.c.x * sx) + (mat.c.y * sy) + (mat.c.z * sz) + translation.z

    lea -16(%a2),%a2                | a2 = &(transform.mat)
	dbra %d5,.L48

.L50:
	movm.l (%sp)+,#0xcfc
	rts


	.globl	M3D_project_f16
	.type	M3D_project_f16, @function
M3D_project_f16:
	movm.l #0x3e00,-(%sp)

	move.l 24(%sp),%a0                      | a0 = s = src
	move.l 28(%sp),%a1                      | a1 = d = dst
	move.w 34(%sp),%d2                      | d2 = i = numv

	move.w viewport_f16,%d4
	asr.w #1,%d4                            | d4 = wi = viewport_f16.x >> 1;
	move.w viewport_f16+2,%d3
	asr.w #1,%d3                            | d3 = hi = viewport_f16.y >> 1;
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
	muls.w %d1,%d0
	asr.l #6,%d0                            |       d0 = fix16Mul(s->x, zi)
	add.w %d4,%d0                           |
	move.w %d0,(%a1)+                       |       d->x = wi + fix16Mul(s->x, zi);

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


	.globl	M3D_project_s16
	.type	M3D_project_s16, @function
M3D_project_s16:
	movm.l %d2-%d5/%a2-%a4,-(%sp)

	move.l 32(%sp),%a0                      | a0 = s = src
	move.l 36(%sp),%a1                      | a1 = d = dst
	move.w 42(%sp),%d4                      | d4 = i = numv

	move.w camDist,%d0
	ext.l %d0
	lsl.l #6,%d0
	move.l %d0,%a2                          | a2 = camDist << 6
	move.w viewport,%d0
	lsr.w #1,%d0                            | a3 = wi = viewport.x >> 1;
	move.w %d0,%a3
	move.w viewport+2,%d0
	lsr.w #1,%d0                            | a4 = hi = viewport.y >> 1;
	move.w %d0,%a4
	moveq #-1,%d5                           | d5 = (-1 << 16) | -1

	subq.w #1,%d4
	jmi .L42

.L40:
	move.w (%a0)+,%d1                       |   d1 = s->x
	move.w (%a0)+,%d2                       |   d2 = s->y
	move.w (%a0)+,%d0                       |   if ((zi = s->z))
	jeq .L38                                |   {

    move.l %a2,%d3
    divs.w %d0,%d3                          |       d3 = zi = fix16Div(camDist, zi);

	muls.w %d3,%d1
	swap %d1
	rol.l #4,%d1                            |       d1 = fix16ToInt(fix16Mul(s->x, zi))
	add.w %a3,%d1                           |
	move.w %d1,(%a1)+                       |       d->x = wi + fix16ToInt(fix16Mul(s->x, zi))

	muls.w %d3,%d2
	swap %d2
	rol.l #4,%d2                            |       d2 = fix16ToInt(fix16Mul(s->y, zi))
	move.w %a4,%d0
	sub.w %d2,%d0
	move.w %d0,(%a1)+                       |       d->y = hi - fix16ToInt(fix16Mul(s->y, zi))

	dbra %d4,.L40

	movm.l (%sp)+,%d2-%d5/%a2-%a4
	rts                                     |   }
                                            |   else
.L38:                                       |   {
	move.l %d5,(%a1)+                       |       d->x = FIX16(-1);
                                            |       d->y = FIX16(-1);
                                            |   }
	dbra %d4,.L40                           | }

.L42:
	movm.l (%sp)+,%d2-%d5/%a2-%a4
	rts

