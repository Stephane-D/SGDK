ARG ALPINE_VERSION=3.18.3
ARG CROSSTOOL_VERSION=1.26.0
ARG BASEDIR=/tmp/work

FROM alpine:$ALPINE_VERSION as build
ARG BASEDIR
ARG CROSSTOOL_VERSION

#################################################################################
##########################    Install required packages    ######################
#################################################################################
RUN apk update && apk upgrade && apk add cmake alpine-sdk wget xz git bash autoconf automake bison flex texinfo help2man gawk libtool ncurses-dev gettext-dev python3-dev rsync openjdk11 ninja

#################################################################################
############################    Building crosstool-ng    ########################
#################################################################################
WORKDIR /root
RUN git clone --branch crosstool-ng-$CROSSTOOL_VERSION --depth=1 https://github.com/crosstool-ng/crosstool-ng.git
WORKDIR /root/crosstool-ng
RUN ./bootstrap && ./configure --enable-local
RUN make
ENV PATH=/root/crosstool-ng:$PATH
COPY samples/m68k-elf ./samples/m68k-elf

#################################################################################
###############################    Building gcc   ###############################
#################################################################################
WORKDIR $BASEDIR
ENV CT_EXPERIMENTAL=y
ENV CT_ALLOW_BUILD_AS_ROOT=y
ENV CT_ALLOW_BUILD_AS_ROOT_SURE=y
# Configure and build the tools
RUN ct-ng m68k-elf
RUN ct-ng build

# Second stage, just copy the built files
FROM scratch
WORKDIR /m68k
COPY --from=build /root/x-tools/m68k-elf/ .
