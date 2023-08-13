ARG ALPINE_VERSION=3.18.3
ARG JDK_VER=11

ARG BASE_IMAGE=ghcr.io/Stephane-D/sgdk-m68k-gcc
FROM $BASE_IMAGE
ARG JDK_VER

RUN apk add --no-cache build-base git openjdk${JDK_VER}-jre-headless
WORKDIR /tmp 
RUN git clone https://github.com/istvan-v/sjasmep.git 
WORKDIR sjasmep 
RUN make

# Set-up SGDK
WORKDIR /sgdk
COPY . .
ENV SGDK_PATH=/sgdk

# Building bintos
RUN cd /sgdk/tools/bintos \
    && gcc -O2 -s src/bintos.c -o $SGDK_PATH/bin/bintos

# Building xgmtool
RUN cd /sgdk/tools/xgmtool \
    && gcc -fexpensive-optimizations -Os -s src/*.c -o $SGDK_PATH/bin/xgmtool

RUN mv /tmp/sjasmep/sjasm ./bin/

ENV PATH="/$SGDK_PATH/bin:${PATH}"

RUN mkdir lib
#build libmd.a
RUN make -f makelib.gen release
#build libmd_debug.a
RUN make -f makelib.gen debug


### Second Stage - clean image ###
FROM alpine:$ALPINE_VERSION

COPY --from=build /m68k/ /usr/

#RUN apt-get install -y build-essential openjdk-17-jre-headless
ARG JDK_VER
RUN apk add --no-cache build-base openjdk${JDK_VER}-jre-headless

# Set-up SGDK
ENV SGDK_PATH=/sgdk

# Create sgdk unprivileged user
RUN addgroup -S sgdk && adduser -S sgdk -G sgdk -h $SGDK_PATH

COPY --from=build --chown=sgdk:sgdk $SGDK_PATH $SGDK_PATH

ENV PATH="/$SGDK_PATH/bin:${PATH}"

# Set-up mount point and make command
VOLUME /src
WORKDIR /src
# Use sgdk user
USER sgdk
ENTRYPOINT make -f $SGDK_PATH/makefile.gen
