#!/bin/bash
set -eou pipefail
BUILD_DIR=/cinnamon_artifact/simulator/build
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}
cmake .. 
cmake --build . --target sst-core 
cmake --build . --target sst-elements