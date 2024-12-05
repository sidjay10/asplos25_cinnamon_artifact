#!/bin/bash
set -eou pipefail

EVALHOME=/cinnamon_artifact/evaluation

cd ${EVALHOME}/keyswitch_comparison/ && bash runscript.sh

wait 
