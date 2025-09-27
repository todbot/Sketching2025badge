#!/bin/bash
# Batch firmware programming of Sketching2026 badge using arduino-cli
# Requires arduino-cli to be set up as described in "docs/firmware-install.md"
# Summary:
# - install arduino-cli
# - arduino-cli core install megatinycore:megaavr  \
#             --additional-urls https://drazzy.com/package_drazzy.com_index.json
#
# Run this code with:
#  bash bash_build_program.sh
 
#set -x
#PORT=/dev/ttyUSB0
PORT=/dev/tty.usbserial-0001
SKETCH_NAME=Sketching2025badge_attiny816
BUILD_DIR=$(pwd)/build
HEX_FILE=./build/$SKETCH_NAME.ino.hex

echo "Compiling..."

rm -rf $BUILD_DIR

arduino-cli compile  \
            --fqbn megaTinyCore:megaavr:atxy6:chip=816,printf=minimal,clock=8internal  \
            --build-path=$BUILD_DIR $SKETCH_NAME

echo "Programming..."
while [ 1 ] ; do
    read -rsn1 -p"Press any key program firmware"; echo
    
    arduino-cli upload \
              --verbose --fqbn megaTinyCore:megaavr:atxy6:chip=816  \
              --port $PORT --programmer serialupdi  \
              --input-file=$HEX_FILE
  echo "--------------------"
done




