#!/bin/bash
BP="black-parrot-sim" # update the path to black-parrot-sim if already present
# updating the location of our control-extraction walker
# sometimes, commenting out '-lowmem' option helps :)
sed -i "s|SURELOG ?= surelog|SURELOG ?= \$\(TOP\)/..\/..\/Surelog\/build\/bin\/controls|" ../$BP/rtl/bp_common/syn/Makefile.surelog

# clean the previous default result -- not necessary after rename
# make -C ../black-parrot-sim/rtl/bp_top/syn clean.surelog
make -C ../$BP/rtl/bp_top/syn parse.surelog | tee ../results/logs/t_$(date +"%T") && mv ../$BP/rtl/bp_top/syn/results/surelog ../results/surelog_$(date +"%T")

sed -i "s|SURELOG ?= \$\(TOP\)/..\/..\/Surelog\/build\/bin\/controls|SURELOG ?= surelog|" ../$BP/rtl/bp_common/syn/Makefile.surelog
