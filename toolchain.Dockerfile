ARG ALPINE_VERSION=3.18.3

FROM alpine:$ALPINE_VERSION as build
RUN apk add --no-cache bash build-base zlib-dev

RUN mkdir -p /tmp/m68k
COPY build_toolchain /tmp/m68k/
RUN cd /tmp/m68k && ./build_toolchain

# Second stage, just create the m68k user and copy the built files
FROM alpine:$ALPINE_VERSION
COPY --from=build /tmp/m68k/staging/usr/ /usr/
WORKDIR m68k 
COPY --from=build /tmp/m68k/staging/usr/ .

