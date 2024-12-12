ARG ALPINE_VERSION=3.18.3
ARG JDK_VER=11

ARG BASE_IMAGE=ghcr.io/stephane-d/sgdk-m68k-gcc
ARG BASE_IMAGE_VERSION=latest


# Stage Zero - Base images for m68k compiler and JRE
FROM $BASE_IMAGE:$BASE_IMAGE_VERSION as m68k-files

FROM alpine:$ALPINE_VERSION as jre-minimal
ARG JDK_VER
RUN apk add --no-cache openjdk${JDK_VER}
ENV JRE_MODULES="java.base,java.desktop,java.xml"
RUN jlink \
    --module-path "$JAVA_HOME/jmods" \
    --add-modules $JRE_MODULES \
    --verbose \
    --strip-debug \
    --compress 2 \
    --no-header-files \
    --no-man-pages \
    --output /opt/jre-minimal


# Stage One - build all tools and libs for SGDK
FROM jre-minimal as build
RUN apk add --no-cache build-base git cmake ninja

# Set-up environment and folders
ENV SGDK_PATH=/sgdk
RUN mkdir -p $SGDK_PATH/bin

# Get SGDK sources
COPY . $SGDK_PATH

# Building apj.jar
WORKDIR $SGDK_PATH/tools/apj/src
RUN javac sgdk/aplib/*.java
RUN jar cfe $SGDK_PATH/bin/apj.jar sgdk.aplib.Launcher sgdk/aplib/*.class

# Building lz4w.jar
WORKDIR $SGDK_PATH/tools/lz4w/src
RUN javac sgdk/lz4w/*.java
RUN jar cfe $SGDK_PATH/bin/lz4w.jar sgdk.lz4w.Launcher sgdk/lz4w/*.class

# Building sizebnd.jar
WORKDIR $SGDK_PATH/tools/sizebnd/src
RUN javac sgdk/sizebnd/*.java
RUN jar cfe $SGDK_PATH/bin/sizebnd.jar sgdk.sizebnd.Launcher sgdk/sizebnd/*.class

# Building rescomp.jar
WORKDIR $SGDK_PATH/tools/rescomp/src
ENV CLASSPATH="$SGDK_PATH/bin/apj.jar:$SGDK_PATH/bin/lz4w.jar:$SGDK_PATH/tools/rescomp/src"
RUN cp -r $SGDK_PATH/tools/commons/src/sgdk .
RUN find . -name "*.java" | xargs javac
RUN echo -e "Main-Class: sgdk.rescomp.Launcher\nClass-Path: apj.jar lz4w.jar" > Manifest.txt
RUN jar cfm $SGDK_PATH/bin/rescomp.jar Manifest.txt  .

# Building xgm2tool.jar
WORKDIR $SGDK_PATH/tools/xgm2tool/src
ENV CLASSPATH="$SGDK_PATH/bin/apj.jar:$SGDK_PATH/bin/lz4w.jar:$SGDK_PATH/tools/xgm2tool/src"
RUN cp -r $SGDK_PATH/tools/commons/src/sgdk .
RUN find . -name "*.java" | xargs javac
RUN echo -e "Main-Class: sgdk.xgm2tool.Launcher\nClass-Path: apj.jar lz4w.jar" > Manifest.txt
RUN jar cfm $SGDK_PATH/bin/xgm2tool.jar Manifest.txt  .

# Building convsym
WORKDIR $SGDK_PATH/tools/convsym
RUN make
RUN cp build/convsym $SGDK_PATH/bin/convsym

# Copy m68k compiler from base image
COPY --from=m68k-files /m68k/ /usr/
ENV PATH="$SGDK_PATH/bin:${PATH}"

# 32-bit not supported on Alpine, this may have side-effects
RUN sed -i /-m32/d $SGDK_PATH/tools/xgmtool/src/CMakeLists.txt
ENV PREFIX=m68k-elf-
RUN cmake -G Ninja -S $SGDK_PATH -B $SGDK_PATH/build_release -DCMAKE_BUILD_TYPE=Release && cmake --build $SGDK_PATH/build_release
RUN cmake -G Ninja -S $SGDK_PATH -B $SGDK_PATH/build_debug -DCMAKE_BUILD_TYPE=Debug && cmake --build $SGDK_PATH/build_debug

RUN rm -rf $SGDK_PATH/tools
RUN rm -rf $SGDK_PATH/build_release
RUN rm -rf $SGDK_PATH/build_debug

# Stage Two - copy tools into a clean image
FROM alpine:$ALPINE_VERSION
RUN apk add --no-cache build-base

# Copy m68k compiler from base image
COPY --from=m68k-files /m68k/ /usr/

# Copy JRE from base image
COPY --from=jre-minimal /opt/jre-minimal /opt/jre-minimal
ENV JAVA_HOME=/opt/jre-minimal
ENV PATH="$PATH:$JAVA_HOME/bin"

# Set-up SGDK
ENV SGDK_PATH=/sgdk

# Create sgdk unprivileged user
RUN addgroup -S sgdk && adduser -S sgdk -G sgdk -h $SGDK_PATH

# Copy SGDK tools and libraries
COPY --from=build --chown=sgdk:sgdk $SGDK_PATH $SGDK_PATH

# Set-up mount point, user, and make command
VOLUME /src
WORKDIR /src
USER sgdk
ENV PATH="$SGDK_PATH/bin:${PATH}"
ENTRYPOINT ["/bin/sh","-c","make -f $SGDK_PATH/makefile.gen $@", "--"]
