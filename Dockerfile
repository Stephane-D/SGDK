FROM i386/debian:buster-slim

# Set-up argument defaults
ARG SGDK_WINE_VER=1aca454a2c1419d97c3518907d156ddf1250469b
ARG JDK_VER=11

# Work-around required for JDK
RUN mkdir -p /usr/share/man/man1

# Install supporting packages
RUN export DEBIAN_FRONTEND='noninteractive' && \
    apt-get update && \
    apt-get install -y --no-install-recommends --no-install-suggests \
      make \
      openjdk-${JDK_VER}-jdk-headless \
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

# Crudely get the script from GitHub. This isn't very secure, but at least we
# use the commit hash. Ideally that repository should simply be merged into this
# one.
ADD https://github.com/Franticware/SGDK_wine/raw/${SGDK_WINE_VER}/generate_wine.sh \
    /tmp/generate_wine.sh
RUN chmod u+x /tmp/generate_wine.sh && \
    cd ${GDK}/bin && \
    /tmp/generate_wine.sh && \
    rm -f /tmp/generate_wine.sh

# Set-up mount point and make command
VOLUME /src
WORKDIR /src
ENTRYPOINT [ "make", "-f", "/sgdk/makefile_wine.gen" ]
