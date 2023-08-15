ARG ALPINE_VERSION=3.18.3
ARG BASEDIR=/tmp/work

FROM alpine:$ALPINE_VERSION as build
ARG BASEDIR
ARG _binu_ver=2.37
ARG _gcc_ver=6.3.0
ARG _mpfrver=4.1.0
ARG _mpcver=1.2.1
ARG _gmpver=6.2.1
ARG _newlib_ver=4.1.0

ENV TARGET=m68k-elf
ENV TARGET_CPU=m68000

ENV BASEDIR=$BASEDIR
ENV BINUTILS=binutils-${_binu_ver}
ENV GCC=gcc-${_gcc_ver}
ENV MPFR=mpfr-${_mpfrver}
ENV MPC=mpc-${_mpcver}
ENV GMP=gmp-${_gmpver}
ENV NEWLIB=newlib-${_newlib_ver}

ENV SRCDIR="$BASEDIR/src"
ENV BUILDDIR="$BASEDIR/build"
ENV INSTALLDIR="$BASEDIR/install"
ENV STAGINGDIR="$BASEDIR/staging"

RUN mkdir -p "$SRCDIR"
RUN mkdir -p "$BUILDDIR"
RUN mkdir -p "$INSTALLDIR"
RUN mkdir -p "$STAGINGDIR"

WORKDIR $SRCDIR
RUN wget http://ftp.gnu.org/gnu/binutils/${BINUTILS}.tar.xz -O - | tar -xJ
RUN wget http://ftp.gnu.org/gnu/gcc/${GCC}/${GCC}.tar.bz2  -O - | tar -xj
RUN wget http://ftp.gnu.org/gnu/mpfr/${MPFR}.tar.xz  -O - | tar -xJ
RUN wget http://ftp.gnu.org/gnu/mpc/${MPC}.tar.gz  -O - | tar -xz
RUN wget http://ftp.gnu.org/gnu/gmp/${GMP}.tar.xz  -O - | tar -xJ
RUN wget ftp://sourceware.org/pub/newlib/${NEWLIB}.tar.gz  -O - | tar -xz

RUN apk add --no-cache bash build-base zlib-dev

#################################################################################
##########################    Building binutils    ##############################
#################################################################################
WORKDIR $BUILDDIR/$BINUTILS
RUN $SRCDIR/$BINUTILS/configure \
	--prefix=/usr \
	--target=$TARGET \
	--disable-multilib \
	--with-cpu=$TARGET_CPU \
	--disable-nls

RUN make -j"$(nproc)"
RUN make prefix=$INSTALLDIR/$BINUTILS/usr install
RUN rm -rf $INSTALLDIR/$BINUTILS/usr/share
RUN rm $INSTALLDIR/$BINUTILS/usr/lib/bfd-plugins/libdep.so
RUN cp -r $INSTALLDIR/$BINUTILS/usr/ $STAGINGDIR
RUN cp -r $INSTALLDIR/$BINUTILS/usr/* /usr/
#################################################################################
#########################    Building GCC bootstrap    ##########################
#################################################################################
WORKDIR $SRCDIR/$GCC
RUN ln -fs ../$MPFR mpfr
RUN ln -fs ../$MPC mpc
RUN ln -fs ../$GMP gmp

WORKDIR $BUILDDIR/$GCC-bootstrap
# These flags are required to build older versions of GCC
ENV CFLAGS="$CFLAGS -Wno-implicit-fallthrough -Wno-cast-function-type -fpermissive"
ENV CXXFLAGS="$CXXFLAGS -Wno-implicit-fallthrough -Wno-cast-function-type -fpermissive"
RUN $SRCDIR/$GCC/configure \
	--prefix=/usr \
	--target=$TARGET \
	--enable-languages="c" \
	--disable-multilib \
	--with-cpu=$TARGET_CPU \
	--with-system-zlib \
	--with-libgloss \
	--without-headers \
	--disable-shared \
	--disable-nls

RUN make -j"$(nproc)" all-gcc
RUN make DESTDIR="$INSTALLDIR/$GCC-bootstrap" install-strip-gcc
RUN rm -rf "$INSTALLDIR/$GCC-bootstrap/usr/share"
RUN cp -r "$INSTALLDIR/$GCC-bootstrap/usr/"* /usr/
#################################################################################
#############################    Building NewLib    #############################
#################################################################################
WORKDIR $BUILDDIR/$NEWLIB
ENV CFLAGS_FOR_TARGET="-Os -g -ffunction-sections -fdata-sections -fomit-frame-pointer -ffast-math"

RUN "$SRCDIR/$NEWLIB/configure" \
	--prefix=/usr \
	--target=$TARGET \
	--enable-languages="c" \
	--disable-newlib-supplied-syscalls \
	--disable-multilib \
	--with-cpu=$TARGET_CPU \
	--disable-nls

RUN make -j"$(nproc)"
RUN DESTDIR=$INSTALLDIR/$NEWLIB make install
RUN cp -r "$INSTALLDIR/$NEWLIB/usr/" "$STAGINGDIR"
RUN cp -r "$INSTALLDIR/$NEWLIB/usr/"* /usr/
ENV CFLAGS_FOR_TARGET=""
#################################################################################
############################    Building final GCC    ###########################
#################################################################################
WORKDIR $BUILDDIR/$GCC

RUN "$SRCDIR/$GCC/configure" \
	--prefix=/usr \
	--target=$TARGET \
	--enable-languages="c,c++" \
	--disable-multilib \
	--with-cpu=$TARGET_CPU \
	--with-system-zlib \
	--with-newlib \
	--with-libgloss \
	--disable-shared \
	--disable-nls

RUN make -j"$(nproc)"
RUN make DESTDIR="$INSTALLDIR/$GCC" install-strip
RUN rm -rf "$INSTALLDIR/$GCC/usr/share"
RUN rm "$INSTALLDIR/$GCC/usr/lib/libcc1.so"
RUN rm "$INSTALLDIR/$GCC/usr/lib/libcc1.so.0"
RUN rm "$INSTALLDIR/$GCC/usr/lib/libcc1.so.0.0.0"

RUN cp -ar "$INSTALLDIR/$GCC/usr/" "$STAGINGDIR"


# Second stage, just  copy the built files
FROM scratch
ARG BASEDIR
WORKDIR /m68k
COPY --from=build $BASEDIR/staging/usr/ ./
