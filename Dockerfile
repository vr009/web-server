FROM ubuntu as build

RUN apt-get update && \
    apt-get install -y git gcc make cmake

ADD . .

RUN chmod +x build.sh && ./build.sh

WORKDIR /build

EXPOSE 80

ENTRYPOINT ["./main"]