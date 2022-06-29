#!/bin/bash

# update the path to black-parrot-sim if already present
BP='../black-parrot-sim'

# updating the location of our control-extraction walker
# sometimes, commenting out '-lowmem' option helps; also, adding multithreading speeds up the compilation by much; -nocache helps in the first run
sed -i "s|SURELOG ?= surelog|SURELOG ?= \$\(TOP\)/..\/..\/Surelog\/build\/bin\/controls|" $BP/rtl/bp_common/syn/Makefile.surelog
# sed -i '\|SURELOG_OPTS += -lowmem|a SURELOG_OPTS += -mp8 -mt8 -nocache' $BP/rtl/bp_common/syn/Makefile.surelog
# sed -i "s|SURELOG_OPTS += -lowmem|# SURELOG_OPTS += -lowmem|" $BP/rtl/bp_common/syn/Makefile.surelog

# clean the previous default result -- not necessary after rename
# make -C ../black-parrot-sim/rtl/bp_top/syn clean.surelog
make -C $BP/rtl/bp_top/syn parse.surelog | tee ../results/logs/t_$(date +"%T") 
mv $BP/rtl/bp_top/syn/results/surelog ../results/surelog_$(date +"%T")

# restoring BP's path to original Surelog executable
sed -i "s|SURELOG ?= \$\(TOP\)/..\/..\/Surelog\/build\/bin\/controls|SURELOG ?= surelog|" $BP/rtl/bp_common/syn/Makefile.surelog
