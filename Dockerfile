FROM amd64/ubuntu:focal

# Set-up argument defaults
ARG SGDK_WINE_VER=1aca454a2c1419d97c3518907d156ddf1250469b
ARG JRE_VER=14

# Work-around required for JDK
RUN mkdir -p /usr/share/man/man1

# Install supporting packages
RUN export DEBIAN_FRONTEND='noninteractive' && \
    apt-get update && \
    apt-get install -y --no-install-recommends --no-install-suggests \
      make \
      openjdk-${JDK_VER}-jre-headless \
      wine && \
    apt-get autoremove --purge -y && \
    apt-get clean && \
    rm -rf \
      /var/lib/apt/lists/* \
      /var/tmp/* \
      /tmp/*

# Set-up SGDK
COPY . /sgdk
WORKDIR /sgdk
ENV GDK=/sgdk

# Set-up mount point and make command
VOLUME /src
WORKDIR /src
ENTRYPOINT [ "make", "-f", "/sgdk/makefile_wine.gen" ]
