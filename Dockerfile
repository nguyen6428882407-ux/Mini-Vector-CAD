# Stage 1: Bien dich ma nguon C++ tren Linux
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --target MiniVectorCADServer -j$(nproc)

# Stage 2: Runtime image nhe va bao mat
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/MiniVectorCADServer /app/MiniVectorCADServer
COPY --from=builder /app/web /app/web

RUN mkdir -p /app/output/projects

ENV PORT=10000
EXPOSE 10000

CMD ["/app/MiniVectorCADServer"]
