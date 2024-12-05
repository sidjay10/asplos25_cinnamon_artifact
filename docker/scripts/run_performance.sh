#!/bin/bash
set -eou pipefail

EVALHOME=/cinnamon_artifact/evaluation

cd ${EVALHOME}/performance && bash runscript.sh

wait 
