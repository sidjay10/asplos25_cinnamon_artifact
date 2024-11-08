#!/bin/bash
set -eou pipefail
OUTPUT_DIR=/cinnamon_artifact/outputs/
if [[ ! -d ${OUTPUT_DIR} ]]; then
    echo "Output directory not mounted"
    exit 1
fi

echo "Container Up"
tail -f /dev/null