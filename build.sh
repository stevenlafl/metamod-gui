#!/bin/bash

# Build script for metamod-gui Docker container
# Builds x86 and/or x64 architectures based on METAMOD_GUI_ARCH env var
# Usage: METAMOD_GUI_ARCH=x86|x64|all ./build.sh

set -e

IMAGE_NAME="metamod-gui"
IMAGE_TAG="latest"
CONTAINER_NAME="metamod-gui-build"

echo "Running Docker container: ${CONTAINER_NAME}"

# Remove existing container if it exists
docker rm -f "${CONTAINER_NAME}" 2>/dev/null || true

# Run the container and build the code
docker run --name "${CONTAINER_NAME}" \
    -v "$(pwd):/app" \
    -e METAMOD_GUI_ARCH="${METAMOD_GUI_ARCH:-all}" \
    "${IMAGE_NAME}:${IMAGE_TAG}" \
    bash -c "
        cd /app

        # Initialize vcpkg submodule if needed
        if [ ! -d 'deps/vcpkg' ]; then
            echo 'Cloning vcpkg...'
            git clone https://github.com/microsoft/vcpkg.git deps/vcpkg
        fi

        echo 'Bootstrapping vcpkg...'
        cd /app/deps/vcpkg && ./bootstrap-vcpkg.sh
        cd /app

        # Convert SVG icon to XPM format
        if [ -f 'assets/icon.svg' ]; then
            echo 'Converting icon.svg to icon.xpm...'
            convert -background none -resize 32x32 assets/icon.svg src/icon.xpm
            echo 'Icon conversion complete'
        fi

        # Build x86
        if [ \"\$METAMOD_GUI_ARCH\" = \"all\" ] || [ \"\$METAMOD_GUI_ARCH\" = \"x86\" ] || [ \"\$METAMOD_GUI_ARCH\" = \"ia32\" ]; then
            echo ''
            echo '=============================================='
            echo 'Building metamod-gui for x86...'
            echo '=============================================='
            cmake --preset linux-x86-debug
            cmake --build build-x86 --config Debug
        fi

        # Build x64
        if [ \"\$METAMOD_GUI_ARCH\" = \"all\" ] || [ \"\$METAMOD_GUI_ARCH\" = \"x64\" ]; then
            echo ''
            echo '=============================================='
            echo 'Building metamod-gui for x64...'
            echo '=============================================='
            cmake --preset linux-x64-debug
            cmake --build build-x64 --config Debug
        fi
    "

echo "Container run completed."

# Show output paths based on what was built
ARCH="${METAMOD_GUI_ARCH:-all}"
if [ "$ARCH" = "all" ] || [ "$ARCH" = "x86" ] || [ "$ARCH" = "ia32" ]; then
    echo "  x86: ./build-x86/Debug/bin/libmetamod-gui.so"
fi
if [ "$ARCH" = "all" ] || [ "$ARCH" = "x64" ]; then
    echo "  x64: ./build-x64/Debug/bin/libmetamod-gui.so"
fi

# Copy the built .so file to the HLDS installation (x86 for Half-Life)
if [ -f "./build-x86/Debug/bin/libmetamod-gui.so" ]; then
    echo "Copying x86 libmetamod-gui.so to HLDS installation..."
    mkdir -p "/home/stevenlafl/Containers/hlds/hlds/ts/addons/metamod-gui/dlls/"
    cp "./build-x86/Debug/bin/libmetamod-gui.so" "/home/stevenlafl/Containers/hlds/hlds/ts/addons/metamod-gui/dlls/"
    echo "Done"
fi
