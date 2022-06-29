#!/bin/bash
git clone git@github.com:chipsalliance/Surelog.git ../Surelog
cd ../Surelog/
git submodule update --init --recursive
make
cd -
mkdir -p ../results
# Either execute the below, or update path to black-parrot-sim as per README.md
# git clone git@github.com:black-parrot/black-parrot-sim.git ../black-parrot-sim
# cd ../black-parrot-sim/
# make prep_lite
# cd -
