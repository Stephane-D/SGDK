FROM i386/alpine

# Set-up argument defaults
ARG JDK_VER=8

# Install supporting packages
RUN apk update && \
  apk add --no-cache \
  bash \
  make \
  openjdk${JDK_VER}-jre \
  freetype \
  wine && \
  rm -fr /var/cache/apk/*

RUN /bin/create-bin-wrappers.sh

# Set-up SGDK
COPY . /sgdk
ENV GDK=/sgdk

# Set-up mount point and make command
VOLUME /src
WORKDIR /src
ENTRYPOINT [ "make", "-f", "/sgdk/makefile.gen" ]
