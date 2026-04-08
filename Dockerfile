FROM ubuntu:22.04

# Avoid tzdata interactive prompt during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    make \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy the entire C++ project (excluding things we don't need, ideally handled by .dockerignore)
COPY . .

# Build the C++ engine
RUN mkdir -p build && cd build && cmake .. && make -j$(nproc) vulturedb

# Run the database
# We map the default entrypoint to the built binary
ENTRYPOINT ["./build/vulturedb"]
