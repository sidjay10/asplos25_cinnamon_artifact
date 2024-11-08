#!/bin/bash
set -eou pipefail

EVALHOME=/cinnamon_artifact/evaluation

cd ${EVALHOME}/fig9/ && bash runscript.sh
cd ${EVALHOME}/fig10/ && bash runscript.sh

wait 
