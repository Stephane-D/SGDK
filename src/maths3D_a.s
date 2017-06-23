    .extern    viewport

    .globl	M3D_transform
    .type	M3D_transform, @function
M3D_transform:
    movm.l %d2-%d5/%a2-%a5,-(%sp)

    move.l 36(%sp),%a2             	| a2 = &transform

    tst.w (%a2)
    jeq .L45

    move.l %a2,-(%sp)
    jsr M3D_buildMat3D
    addq.l #4,%sp

.L45:
    move.l 2(%a2),%a0               | a0 = &translation
    movem.w (%a0)+,%a3-%a5          | a3 = translation.x   a4 = translation.y   a5 = translation.z

    move.l 40(%sp),%a0              | a0 = src
    move.l 44(%sp),%a1              | a1 = dest
    lea    10(%a2),%a2              | a2 = &(transform.mat)
    move.w 50(%sp),%d5              | d5 = numv

    subq.w #1,%d5
    jmi    .L50

.L48:
    movem.w (%a0)+,%d2-%d4          | d2 = sx        d3 = sy        d4 = sz

    move.w (%a2)+,%d0               | d0 = mat.a.x
    muls.w %d2,%d0                  | d0 = mat.a.x * sx
    move.w (%a2)+,%d1               | d1 = mat.a.y
    muls.w %d3,%d1                  | d1 = mat.a.y * sy
    add.l %d1,%d0                   | d0 = (mat.a.x * sx) + (mat.a.y * sy)
    move.w (%a2)+,%d1               | d1 = mat.a.z
    muls.w %d4,%d1                  | d1 = mat.a.z * sz
    add.l %d1,%d0                   | d0 = (mat.a.x * sx) + (mat.a.y * sy) + (mat.a.z * sz)
    asr.l #6,%d0
    add.w %a3,%d0                   | d0 = (mat.a.x * sx) + (mat.a.y * sy) + (mat.a.z * sz) + translation.x
    move.w %d0,(%a1)+               | dest++ = (mat.a.x * sx) + (mat.a.y * sy) + (mat.a.z * sz) + translation.x

    move.w (%a2)+,%d0               | d0 = mat.b.x
    muls.w %d2,%d0                  | d0 = mat.b.x * sx
    move.w (%a2)+,%d1               | d1 = mat.b.y
    muls.w %d3,%d1                  | d1 = mat.b.y * sy
    add.l %d1,%d0                   | d0 = (mat.b.x * sx) + (mat.b.y * sy)
    move.w (%a2)+,%d1               | d1 = mat.b.z
    muls.w %d4,%d1                  | d1 = mat.b.z * sz
    add.l %d1,%d0                   | d0 = (mat.b.x * sx) + (mat.b.y * sy) + (mat.b.z * sz)
    asr.l #6,%d0
    add.w %a4,%d0                   | d0 = (mat.b.x * sx) + (mat.b.y * sy) + (mat.b.z * sz) + translation.y
    move.w %d0,(%a1)+               | dest++ = (mat.b.x * sx) + (mat.b.y * sy) + (mat.b.z * sz) + translation.y

    muls.w (%a2)+,%d2               | d2 = mat.c.x * sx
    muls.w (%a2)+,%d3               | d3 = mat.c.y * sy
    add.l %d3,%d2                   | d2 = (mat.c.x * sx) + (mat.c.y * sy)
    muls.w (%a2)+,%d4               | d4 = mat.c.z * sz
    add.l %d4,%d2                   | d2 = (mat.c.x * sx) + (mat.c.y * sy) + (mat.c.z * sz)
    asr.l #6,%d2
    add.w %a5,%d2                   | d2 = (mat.c.x * sx) + (mat.c.y * sy) + (mat.c.z * sz) + translation.z
    move.w %d2,(%a1)+               | dest++ = (mat.c.x * sx) + (mat.c.y * sy) + (mat.c.z * sz) + translation.z

    lea -18(%a2),%a2                | a2 = &(transform.mat)
    dbra %d5,.L48

.L50:
    movem.l (%sp)+,%d2-%d5/%a2-%a5
    rts


    .globl    M3D_project_f16
    .type    M3D_project_f16, @function
M3D_project_f16:
    movem.l %d2-%d7/%a2-%a3,-(%sp)

    move.l 36(%sp),%a0                      | a0 = s = src
    move.l 40(%sp),%a1                      | a1 = d = dst
    move.w 46(%sp),%d7                      | d7 = i = numv

    subq.w #1,%d7
    jmi .L34

    lea context3D,%a2

    move.w (%a2)+,%d5
    lsl.w #5,%d5                            | d5 = centerX = intToFix16(viewport.x / 2)
    move.w (%a2)+,%d6
    lsl.w #5,%d6                            | d6 = centerY = intToFix16(viewport.y / 2)
    moveq #0,%d4
    move.w (%a2),%d4                        | d4 = camDist

    move.w %d5,%d0
    swap %d0
    move.w %d6,%d0
    move.l %d0,%a2                          | a2 = (centerX << 16) | centerY

    move.w %d4,%d0
    ext.l %d0
    asl.l #6,%d0
    move.l %d0,%a3                          | a3 = camDist << 6

.L32:                                       | {
    movem.w (%a0)+,%d0-%d2                  |   d0 = x = s->x, d1 = y = s->y, d2 = z = s->z

    add.w %d4,%d2                           |   if ((zi = (camDist + z)) > 0)
    jle .L30                                |   {

    move.l %a3,%d3
    divs.w %d2,%d3                          |       d3 = scale = fix16Div(camDist, camDist + z)

    muls.w %d3,%d0
    asr.l #6,%d0
    add.w %d5,%d0
    move.w %d0,(%a1)+                       |       d->x = centerX + (scale * x)

    muls.w %d3,%d1
    asr.l #6,%d1
    move.w %d6,%d0
    sub.w %d1,%d0
    move.w %d0,(%a1)+                       |       d->y = centerY - (scale * y)

    dbra %d7,.L32

    movem.l (%sp)+,%d2-%d7/%a2-%a3
    rts                                     |   }
                                            |   else
.L30:                                       |   {
    move.l %a2,(%a1)+                       |       d->x = centerX
                                            |       d->y = centerY
                                            |   }
    dbra %d7,.L32                           | }

.L34:
    movem.l (%sp)+,%d2-%d7/%a2-%a3
    rts


    .globl    M3D_project_s16
    .type    M3D_project_s16, @function
M3D_project_s16:
    movem.l %d2-%d7/%a2-%a3,-(%sp)

    move.l 36(%sp),%a0                      | a0 = s = src
    move.l 40(%sp),%a1                      | a1 = d = dst
    move.w 46(%sp),%d7                      | d7 = i = numv

    subq.w #1,%d7
    jmi .L42

    lea context3D,%a2

    move.w (%a2)+,%d5
    lsr.w #1,%d5                            | d5 = centerX = viewport.x / 2
    move.w (%a2)+,%d6
    lsr.w #1,%d6                            | d6 = centerY = viewport.y / 2
    moveq #0,%d4
    move.w (%a2),%d4                        | d4 = camDist

    move.w %d5,%d0
    swap %d0
    move.w %d6,%d0
    move.l %d0,%a2                          | a2 = (centerX << 16) | centerY

    move.w %d4,%d0
    ext.l %d0
    swap %d0
    asr.l #6,%d0
    move.l %d0,%a3                          | a3 = camDist << (6 + 4)
                                            | 6 to prepare for division
                                            | 4 for *16 ratio

.L40:                                       | while(i--) {
    movem.w (%a0)+,%d0-%d2                  |   d0 = x = s->x, d1 = y = s->y, d2 = z = s->z

    add.w %d4,%d2                           |   if ((zi = (camDist + z)) > 0)
    jle .L38                                |   {

    move.l %a3,%d3
    divs.w %d2,%d3                          |       d3 = scale = fix16Div((camDist << (4 + 2)), camDist + z)

    muls.w %d3,%d0
    swap %d0
    rol.l #4,%d0
    add.w %d5,%d0
    move.w %d0,(%a1)+                       |       d->x = centerX + (scale * x)

    muls.w %d3,%d1
    swap %d1
    rol.l #4,%d1
    move.w %d6,%d0
    sub.w %d1,%d0
    move.w %d0,(%a1)+                       |       d->y = centerY - (scale * y)

    dbra %d7,.L40

    movem.l (%sp)+,%d2-%d7/%a2-%a3
    rts                                     |   }
                                            |   else
.L38:                                       |   {
    move.l %a2,(%a1)+                       |       d->x = centerX
                                            |       d->y = centerY
                                            |   }
    dbra %d7,.L40                           | }

.L42:
    movem.l (%sp)+,%d2-%d7/%a2-%a3
    rts
