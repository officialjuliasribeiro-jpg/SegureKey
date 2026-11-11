FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libssl-dev \
    libcpp-httplib-dev \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN cmake -S . -B build \
    && cmake --build build --target SecureKeyServer -j2

ENV PORT=10000

EXPOSE 10000

CMD ["sh", "-c", "./build/SecureKeyServer"]
