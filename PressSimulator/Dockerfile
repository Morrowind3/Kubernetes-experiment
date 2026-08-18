
# ---- Build stage ----
FROM debian:bookworm AS build

# install compile deps
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    protobuf-compiler \
    protobuf-compiler-grpc \
    libgrpc++-dev \
    libprotobuf-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy source to container
WORKDIR /src
COPY . .

# parallel-build source
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --target PressSimulator -j"$(nproc)"

# ---- Runtime stage ----
FROM debian:bookworm-slim AS runtime

# install runtime deps
RUN apt-get update && apt-get install -y \
    libgrpc++1 \
    libprotobuf32 \
    && rm -rf /var/lib/apt/lists/*

# copy built binaries to bin
COPY --from=build /src/build/PressSimulator /usr/local/bin/press_simulator

EXPOSE 50051

# Important: use exec form, not shell form (without [""]). If k8s sends a SIGTERM, it goes to pid 1.
# pid 1 should be our application, not the shell!
ENTRYPOINT ["/usr/local/bin/press_simulator"]
