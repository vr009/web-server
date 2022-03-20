FROM ubuntu as build

ENV TZ=Europe/Kiev
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get update && \
    apt-get install -y cmake gcc make git g++

ADD . .

RUN chmod +x build.sh && ./build.sh

WORKDIR /build

EXPOSE 80

ENTRYPOINT ["./main"]