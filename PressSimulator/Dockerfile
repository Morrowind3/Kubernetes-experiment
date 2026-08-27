
# ---- Build stage ----
FROM debian:bookworm AS build

# install conan
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

RUN pip install --break-system-packages conan

# Copy source to container
WORKDIR /src
COPY . .

# parallel-build source statically
RUN conan profile detect --force && \
    conan install . --build=missing \
        -s build_type=Release \
        -o "*:shared=False" && \
    cmake --preset conan-release && \
    cmake --build --preset conan-release --target PressSimulator -j"$(nproc)"

# ---- Runtime stage ----
FROM debian:bookworm-slim AS runtime


# copy built binaries to bin
COPY --from=build /src/build/Release/PressSimulator /usr/local/bin/press_simulator

EXPOSE 50051

# Important: use exec form, not shell form (without [""]). If k8s sends a SIGTERM, it goes to pid 1.
# pid 1 should be our application, not the shell!
ENTRYPOINT ["/usr/local/bin/press_simulator"]
