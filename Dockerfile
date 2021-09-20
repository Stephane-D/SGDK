FROM amd64/ubuntu:20.04

# Set-up argument defaults
ARG JDK_VER=11

# Install supporting packages
RUN dpkg --add-architecture i386 \
  && apt-get update  \
  && DEBIAN_FRONTEND=noninteractive apt-get install -y \
  bash \
  make \
  openjdk-${JDK_VER}-jre-headless \
  wine32 \
  && rm -rf /var/lib/apt/lists/*

# Set-up SGDK
COPY . /sgdk
ENV GDK=/sgdk

# Create wrappers to execute .exe files using wine
RUN /sgdk/bin/create-bin-wrappers.sh

# Set-up mount point and make command
VOLUME /src
WORKDIR /src

ENTRYPOINT [ "make", "-f", "/sgdk/makefile.gen" ]
