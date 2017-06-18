    .globl	bench_add8reg
bench_add8reg:
    move.w  6(%sp),%d0         | d0 = len

    subq.w  #1,%d0

.L11:
    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1

    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1

    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1

    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1
    add.b   %d1,%d1

    dbra    %d0,.L11

    rts


    .globl	bench_add16reg
bench_add16reg:
    move.w  6(%sp),%d0         | d0 = len

    subq.w  #1,%d0

.L12:
    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1

    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1

    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1

    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1
    add.w   %d1,%d1

    dbra    %d0,.L12

    rts


    .globl	bench_add32reg
bench_add32reg:
    move.w  6(%sp),%d0         | d0 = len

    subq.w  #1,%d0

.L14:
    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1

    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1

    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1

    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1
    add.l   %d1,%d1

    dbra    %d0,.L14

    rts


    .globl	bench_add8mem
bench_add8mem:
    move.l  4(%sp),%a0          | a0 = src
    move.l  8(%sp),%a1          | a1 = dst
    move.w  14(%sp),%d0         | d0 = len

    subq.w  #1,%d0

.L01:
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+

    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+

    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+

    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+
    move.b  (%a0)+,%d1
    add.b   %d1,(%a1)+

    sub.w   #32,%a0
    sub.w   #32,%a1

    dbra    %d0,.L01

    rts


    .globl	bench_add16mem
bench_add16mem:
    move.l  4(%sp),%a0          | a0 = src
    move.l  8(%sp),%a1          | a1 = dst
    move.w  14(%sp),%d0         | d0 = len

    subq.w  #1,%d0

.L02:
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+

    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+

    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+

    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+
    move.w  (%a0)+,%d1
    add.w   %d1,(%a1)+

    sub.w   #64,%a0
    sub.w   #64,%a1

    dbra    %d0,.L02

    rts


    .globl	bench_add32mem
bench_add32mem:
    move.l  4(%sp),%a0          | a0 = src
    move.l  8(%sp),%a1          | a1 = dst
    move.w  14(%sp),%d0         | d0 = len

    subq.w  #1,%d0

.L03:
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+

    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+

    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+

    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+
    move.l  (%a0)+,%d1
    add.l   %d1,(%a1)+

    sub.w   #128,%a0
    sub.w   #128,%a1

    dbra    %d0,.L03

    rts


    .globl	bench_mulu
bench_mulu:
    move.w  %d2,-(%sp)
    move.w  8(%sp),%d0          | d0 = src
    move.w  12(%sp),%d1         | d1 = dst
    move.w  16(%sp),%d2         | d2 = len

    move.w  %d0,%a0
    move.w  %d1,%a1             | save them

    subq.w  #1,%d2

.L04:
    move.w  %a0,%d1
    mulu.w  %d0,%d1
    mulu.w  %d0,%d1
    mulu.w  %d0,%d1
    mulu.w  %d0,%d1
    move.w  %a1,%d1
    mulu.w  %d0,%d1
    mulu.w  %d0,%d1
    mulu.w  %d0,%d1
    mulu.w  %d0,%d1

    move.w  %a0,%d1
    mulu.w  %d0,%d1
    mulu.w  %d0,%d1
    mulu.w  %d0,%d1
    mulu.w  %d0,%d1
    move.w  %a1,%d1
    mulu.w  %d0,%d1
    mulu.w  %d0,%d1
    mulu.w  %d0,%d1
    mulu.w  %d0,%d1

    dbra    %d2,.L04

    move.w  (%sp)+,%d2
    rts


    .globl	bench_muls
bench_muls:
    move.w  %d2,-(%sp)
    move.w  8(%sp),%d0          | d0 = src
    move.w  12(%sp),%d1         | d1 = dst
    move.w  16(%sp),%d2         | d2 = len

    move.w  %d0,%a0
    move.w  %d1,%a1             | save them

    subq.w  #1,%d2

.L05:
    move.w  %a0,%d1
    muls.w  %d0,%d1
    muls.w  %d0,%d1
    muls.w  %d0,%d1
    muls.w  %d0,%d1
    move.w  %a1,%d1
    muls.w  %d0,%d1
    muls.w  %d0,%d1
    muls.w  %d0,%d1
    muls.w  %d0,%d1

    move.w  %a0,%d1
    muls.w  %d0,%d1
    muls.w  %d0,%d1
    muls.w  %d0,%d1
    muls.w  %d0,%d1
    move.w  %a1,%d1
    muls.w  %d0,%d1
    muls.w  %d0,%d1
    muls.w  %d0,%d1
    muls.w  %d0,%d1

    dbra    %d2,.L05

    move.w  (%sp)+,%d2
    rts


    .globl	bench_divu
bench_divu:
    move.w  %d2,-(%sp)
    move.l  6(%sp),%d1          | d1 = src
    move.l  10(%sp),%d0         | d0 = dst
    move.w  16(%sp),%d2         | d2 = len

    move.l  %d1,%a0
    swap    %d1
    move.l  %d1,%a1             | save them

    subq.w  #1,%d2

.L06:
    move.l  %a0,%d1
    divu.w  %d0,%d1
    divu.w  %d0,%d1
    divu.w  %d0,%d1
    divu.w  %d0,%d1
    move.l  %a1,%d1
    divu.w  %d0,%d1
    divu.w  %d0,%d1
    divu.w  %d0,%d1
    divu.w  %d0,%d1

    move.l  %a0,%d1
    divu.w  %d0,%d1
    divu.w  %d0,%d1
    divu.w  %d0,%d1
    divu.w  %d0,%d1
    move.l  %a1,%d1
    divu.w  %d0,%d1
    divu.w  %d0,%d1
    divu.w  %d0,%d1
    divu.w  %d0,%d1

    dbra    %d2,.L06

    move.w  (%sp)+,%d2
    rts


    .globl	bench_divs
bench_divs:
    move.w  %d2,-(%sp)
    move.l  6(%sp),%d1          | d1 = src
    move.l  10(%sp),%d0         | d0 = dst
    move.w  16(%sp),%d2         | d2 = len

    move.l  %d1,%a0
    swap    %d1
    move.l  %d1,%a1             | save them

    subq.w  #1,%d2

.L07:
    move.l  %a0,%d1
    divs.w  %d0,%d1
    divs.w  %d0,%d1
    divs.w  %d0,%d1
    divs.w  %d0,%d1
    move.l  %a1,%d1
    divs.w  %d0,%d1
    divs.w  %d0,%d1
    divs.w  %d0,%d1
    divs.w  %d0,%d1

    move.l  %a0,%d1
    divs.w  %d0,%d1
    divs.w  %d0,%d1
    divs.w  %d0,%d1
    divs.w  %d0,%d1
    move.l  %a1,%d1
    divs.w  %d0,%d1
    divs.w  %d0,%d1
    divs.w  %d0,%d1
    divs.w  %d0,%d1

    dbra    %d2,.L07

    move.w  (%sp)+,%d2
    rts


