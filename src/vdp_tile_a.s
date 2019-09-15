    .align	2
    .globl  VDP_loadTileData
    .type   VDP_loadTileData, @function
VDP_loadTileData:
    movm.l %d2-%d4,-(%sp)

    move.l 16(%sp),%d2              | d2 = data
    move.w 26(%sp),%d3              | d3 = num
    jeq .L1

    movq  #0,%d4
    move.w 22(%sp),%d4              | d4 = ind
    lsl.w #5,%d4                    | d4 = ind * 32 = VRAM address

    move.b 31(%sp),%d0              | d0 = tm (TransferMethod)
    subq.b #1,%d0                   | 0 = CPU, 1 = DMA, 2 = DMA QUEUE
    jmi .transfer_CPU
    jeq .transfer_DMA

.DMA_queue:
    lsl.w #4,%d3                    | d3 = num * 16 (size of DMA in word)

    pea 2.w                         | prepare parameters for VDP_doDMA
    move.l %d3,-(%sp)
    move.l %d4,-(%sp)
    move.l %d2,-(%sp)
    clr.l -(%sp)
    jsr DMA_queueDma
    lea (20,%sp),%sp
    jra .L1

.transfer_DMA:
    lsl.w #4,%d3                    | d3 = num * 16 (size of DMA in word)

    pea 2.w                         | prepare parameters for VDP_doDMA
    move.l %d3,-(%sp)
    move.l %d4,-(%sp)
    move.l %d2,-(%sp)
    clr.l -(%sp)
    jsr DMA_doDma
    lea (20,%sp),%sp
    jra .L1

    .align  2
.transfer_CPU:
    pea 2.w
    jsr VDP_setAutoInc
    addq.l #4,%sp

    lsl.l #2,%d4
    lsr.w #2,%d4
    andi.w #0x3FFF,%d4
    ori.w #0x4000,%d4
    swap %d4                        | d4 = formated VRAM address for VDP command write

    move.l %d4,0xC00004             | set destination address in VDP Ctrl command

    move.l %d2,%a0                  | a0 = data
    move.l #0xC00000,%a1
    subq.w #1,%d3

.L6:
    move.l (%a0)+,(%a1)
    move.l (%a0)+,(%a1)
    move.l (%a0)+,(%a1)
    move.l (%a0)+,(%a1)
    move.l (%a0)+,(%a1)
    move.l (%a0)+,(%a1)
    move.l (%a0)+,(%a1)
    move.l (%a0)+,(%a1)
    dbra %d3,.L6

.L1:
    movm.l (%sp)+,%d2-%d4
    rts


    .align  2
    .globl  VDP_loadBMPTileData
    .type   VDP_loadBMPTileData, @function
VDP_loadBMPTileData:
    movm.l #0x3c00,-(%sp)

    move.w 34(%sp),%d3                  | d3 = h
    jeq .L17
    move.w 30(%sp),%d4                  | d4 = w
    jeq .L17

    pea 2.w
    jbsr VDP_setAutoInc
    addq.l #4,%sp

    move.l #0xC00000,%a1                | VDP data port

    movq  #0, %d0
    move.w 26(%sp),%d0                  | d0 = index

    lsl.l #7,%d0                        | adjust index to VRAM address (* 32)
    lsr.w #2,%d0
    andi.w #0x3FFF,%d0
    ori.w #0x4000,%d0
    swap %d0                            | d0 = formated VRAM address for VDP command write
    move.l %d0,0xC00004                 | set destination address in VDP Ctrl command

    moveq #0,%d5
    move.w 38(%sp),%d5                  | d5 = bmp_w
    lsl.l #2,%d5                        | d5 = bmp_w adjusted for u32*
    move.l %d5,%d0
    lsl.l #3,%d0
    sub.l %d5,%d0                       | d0 = (bmp_w * 7) adjusted for u32*

    move.l 20(%sp),%a0                  | a0 = src
    subq.w #1,%d3
    subq.w #1,%d4

.L15:
    move.w %d4,%d2

.L14:
    move.l (%a0),(%a1)
    add.l %d5,%a0
    move.l (%a0),(%a1)
    add.l %d5,%a0
    move.l (%a0),(%a1)
    add.l %d5,%a0
    move.l (%a0),(%a1)
    add.l %d5,%a0
    move.l (%a0),(%a1)
    add.l %d5,%a0
    move.l (%a0),(%a1)
    add.l %d5,%a0
    move.l (%a0),(%a1)
    add.l %d5,%a0
    move.l (%a0),(%a1)

    sub.l %d0,%a0
    addq.l #4,%a0
    dbra %d2,.L14

.L19:
    add.l %d0,%a0
    dbra %d3,.L15

.L17:
    movm.l (%sp)+,#0x3c
    rts
