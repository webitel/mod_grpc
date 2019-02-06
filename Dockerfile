FROM webitel/freeswitch-base:latest

RUN apt-get update
RUN apt-get install -y --force-yes vlc-nox build-essential
RUN apt-get update
RUN apt-get install -y --force-yes freeswitch-video-deps-most

RUN git clone -b v1.6 https://freeswitch.org/stash/scm/fs/freeswitch.git /freeswitch.git

RUN cd /freeswitch.git \
    && sh bootstrap.sh

RUN cd /freeswitch.git && ./configure -C --disable-zrtp && make && make install


RUN mkdir p /build
WORKDIR /build
RUN git clone -b $(curl -L https://grpc.io/release) https://github.com/grpc/grpc

WORKDIR /build/grpc
RUN git submodule update --init --recursive

RUN apt-get install -y software-properties-common
RUN add-apt-repository 'deb http://deb.debian.org/debian stretch main'
RUN apt-get update
RUN apt install -y --force-yes cmake=3.7.2-1
RUN apt install -y --force-yes golang

WORKDIR /build
ADD src /build
RUN cmake -DCMAKE_BUILD_TYPE=Release .
RUN make
RUN make install
RUN ls

ENTRYPOINT ["/usr/local/freeswitch/bin/freeswitch", "-nonat"]