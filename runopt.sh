#!/usr/bin/env bash

set -e

make clean
make -j17 OPT=1

/usr/bin/time -v ./a.out -q mega-muxer-metadata.json mega-muxer-20221126/channel0_4
