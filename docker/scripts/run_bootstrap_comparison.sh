#!/bin/bash
set -eou pipefail

EVALHOME=/cinnamon_artifact/evaluation

cd ${EVALHOME}/bootstrap_comparison/ && bash runscript.sh

wait 
