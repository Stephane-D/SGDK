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

# Create sgdk unprivileged user
RUN useradd -ms /bin/sh -d /sgdk sgdk

# Set-up SGDK
COPY . /sgdk
ENV GDK=/sgdk
ENV SGDK_DOCKER=y

# Create wrappers to execute .exe files using wine
RUN sed -i 's/\r$//' sgdk/bin/create-bin-wrappers.sh && \  
        chmod +x sgdk/bin/create-bin-wrappers.sh
        
RUN sgdk/bin/create-bin-wrappers.sh

# Set-up mount point and make command
VOLUME /src
WORKDIR /src

# Use sgdk user
USER sgdk
ENTRYPOINT [ "make", "-f", "/sgdk/makefile.gen" ]
