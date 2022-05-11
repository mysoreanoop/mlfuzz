#!/bin/bash

# updating the location of our control-extraction walker
# sometimes, commenting out -lowmem option helps too :)
sed -i "s|SURELOG ?= surelog|SURELOG ?= \$\(TOP\)/..\/..\/Surelog\/build\/bin\/controls|" ../black-parrot-sim/rtl/bp_common/syn/Makefile.surelog

# clean the previous default result -- not sure if necessary after rename
make -C ../black-parrot-sim/rtl/bp_top/syn clean.surelog
make -C ../black-parrot-sim/rtl/bp_top/syn parse.surelog |& tee ../results/logs/t_$(date +"%T") && mv ../black-parrot-sim/rtl/bp_top/syn/results/surelog ../results/surelog_$(date +"%T")
