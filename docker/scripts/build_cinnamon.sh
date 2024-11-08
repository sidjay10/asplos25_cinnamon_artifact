#!/bin/bash
set -eou pipefail
cd /cinnamon_artifact/simulator/build/
cmake --build . --target install
