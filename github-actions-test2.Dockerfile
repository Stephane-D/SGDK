ARG ALPINE_VERSION=3.18.3
ARG BASE_IMAGE=alpine:latest
FROM $BASE_IMAGE
CMD ["echo", "Hello World 2!"]
