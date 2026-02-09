FROM debian:bookworm AS builder

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Install D++ from the official .deb package.
RUN wget -O /tmp/dpp.deb https://dl.dpp.dev/ && \
    (dpkg -i /tmp/dpp.deb || (apt-get update && apt-get install -y -f && dpkg -i /tmp/dpp.deb)) && \
    rm -f /tmp/dpp.deb && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY CMakeLists.txt /app/CMakeLists.txt
COPY cmake /app/cmake
COPY include /app/include
COPY src /app/src

RUN cmake -S /app -B /app/build \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Release && \
    cmake --build /app/build

FROM debian:bookworm-slim AS runtime

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Install D++ runtime from the official .deb package.
RUN wget -O /tmp/dpp.deb https://dl.dpp.dev/ && \
    (dpkg -i /tmp/dpp.deb || (apt-get update && apt-get install -y -f && dpkg -i /tmp/dpp.deb)) && \
    rm -f /tmp/dpp.deb && \
    rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/build/palette /usr/local/bin/palette

RUN ldconfig

USER nobody
ENTRYPOINT ["/usr/local/bin/palette"]
