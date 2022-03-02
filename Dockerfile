# 해당 도커 파일은 클라이언트 기준입니다.

FROM ubuntu:latest

ARG DEBIAN_FRONTEND=noninteractive
RUN sed -i 's/archive.ubuntu.com/mirror.kakao.com/g' /etc/apt/sources.list \
    && apt-get update && apt-get install -y \
    gcc gdb git libssl-dev make net-tools \
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/openssl/openssl

WORKDIR /openssl

RUN  ["./Configure"]
RUN  ["make", "-j8"]
RUN  ["make", "install", "-j8"]

RUN ["mkdir", "chat-program"]


WORKDIR /chat-program

ADD ../ ./

RUN ["make"]
RUN ["ldconfig", "/usr/local/lib64"]

# 서버 전용 이미지 제작 시 하단 코드를 주석 처리 혹은 삭제가 필요합니다.
ENTRYPOINT ["./proc_client", "172.17.0.2", "4433"]

# 원격 디버깅
# ENTRYPOINT ["gdbserver", "0.0.0.0:9091", "./proc_server"]