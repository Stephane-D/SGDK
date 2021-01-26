FROM i386/debian:buster-slim
RUN echo 'APT::Install-Suggests "false";' >> /etc/apt/apt.conf.d/30install-suggests \
    && echo 'APT::Install-Recommends "false";' >> /etc/apt/apt.conf.d/30install-suggests \
    && apt-get update -y

RUN apt-get install -y wine
RUN apt-get install -y make

# workdaround required for jdk
RUN mkdir -p /usr/share/man/man1
RUN apt-get install -y openjdk-11-jdk-headless

ADD . /sgdk
WORKDIR /sgdk

# Crudely get the script from github.
# This isn't very secure, but at least we use the commit hash.
# Ideally that repository should simply be merged into this one.
ADD https://raw.githubusercontent.com/Franticware/SGDK_wine/c8a580a06f20fb45f43a18b65849e318b5f1341e/generate_wine.sh bin/generate_wine.sh
RUN cd bin && sh generate_wine.sh

ENV GDK=/sgdk
WORKDIR /src
ENTRYPOINT [ "make", "-f", "/sgdk/makefile_wine.gen" ]
