#!/bin/bash
set -eou pipefail

EVALHOME=/cinnamon_artifact/evaluation

cd ${EVALHOME}/fig7/ && bash runscript.sh

wait 
