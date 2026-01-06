FROM ubuntu:24.04

# Accept user information as build arguments
ARG USER_ID=1000
ARG GROUP_ID=1000

# Add i386 architecture first for 32-bit support
RUN dpkg --add-architecture i386

# Install required dependencies for 32-bit compilation and FLTK build
RUN apt-get update && apt-get install -y \
    gcc-multilib \
    g++-multilib \
    cmake \
    ninja-build \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    sudo \
    imagemagick \
    libasound2-dev \
    libasound2-dev:i386 \
    libglew-dev \
    libglu1-mesa-dev \
    libglu1-mesa-dev:i386 \
    libcairo2-dev \
    libcairo2-dev:i386 \
    libx11-dev \
    libx11-dev:i386 \
    libxcursor-dev \
    libxcursor-dev:i386 \
    libxft-dev \
    libxft-dev:i386 \
    libxinerama-dev \
    libxinerama-dev:i386 \
    libfontconfig1-dev \
    libfontconfig1-dev:i386 \
    libxrender-dev \
    libxrender-dev:i386 \
    libxext-dev \
    libxext-dev:i386 \
    libxfixes-dev \
    libxfixes-dev:i386 \
    libpng-dev \
    libpng-dev:i386 \
    libjpeg-dev \
    libjpeg-dev:i386 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Build FLTK 1.4 from source for both architectures
ARG FLTK_VERSION=1.4.4
RUN git clone --depth 1 --branch release-${FLTK_VERSION} https://github.com/fltk/fltk.git /tmp/fltk

# Build FLTK for x64
RUN mkdir -p /tmp/fltk/build-x64 && cd /tmp/fltk/build-x64 && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/fltk-x64 \
        -DCMAKE_CXX_STANDARD=11 \
        -DCMAKE_CXX_EXTENSIONS=OFF \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DFLTK_BUILD_SHARED_LIBS=OFF \
        -DFLTK_BUILD_TEST=OFF \
        -DFLTK_BUILD_EXAMPLES=OFF \
        -DFLTK_BUILD_FLUID=OFF \
        -DFLTK_BUILD_FLTK_OPTIONS=OFF \
        -DFLTK_BUILD_FORMS=ON \
        -DFLTK_OPTION_CAIRO_WINDOW=ON \
        -DFLTK_BACKEND_WAYLAND=OFF \
        -DFLTK_USE_GL=OFF \
        -G Ninja && \
    ninja && \
    ninja install

# Build FLTK for x86 (32-bit)
RUN mkdir -p /tmp/fltk/build-x86 && cd /tmp/fltk/build-x86 && \
    PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/opt/fltk-x86 \
        -DCMAKE_CXX_STANDARD=11 \
        -DCMAKE_CXX_EXTENSIONS=OFF \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DCMAKE_C_FLAGS="-m32" \
        -DCMAKE_CXX_FLAGS="-m32" \
        -DCMAKE_EXE_LINKER_FLAGS="-m32" \
        -DCMAKE_SHARED_LINKER_FLAGS="-m32" \
        -DFLTK_BUILD_SHARED_LIBS=OFF \
        -DFLTK_BUILD_TEST=OFF \
        -DFLTK_BUILD_EXAMPLES=OFF \
        -DFLTK_BUILD_FLUID=OFF \
        -DFLTK_BUILD_FLTK_OPTIONS=OFF \
        -DFLTK_BUILD_FORMS=ON \
        -DFLTK_OPTION_CAIRO_WINDOW=ON \
        -DFLTK_BACKEND_WAYLAND=OFF \
        -DFLTK_USE_GL=OFF \
        -G Ninja && \
    ninja && \
    ninja install

# Cleanup FLTK source
RUN rm -rf /tmp/fltk

# Create a user with matching UID/GID (rename existing if needed)
RUN groupadd -g ${GROUP_ID} builder 2>/dev/null || groupmod -n builder $(getent group ${GROUP_ID} | cut -d: -f1) && \
    useradd -u ${USER_ID} -g ${GROUP_ID} -m -s /bin/bash builder 2>/dev/null || usermod -l builder -d /home/builder -m $(getent passwd ${USER_ID} | cut -d: -f1) && \
    echo "builder ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Set working directory and change ownership
WORKDIR /app
RUN chown -R builder:builder /app

# Switch to the created user
USER builder
