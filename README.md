# mlfuzz
A coverage-driven automated fuzzing for hardware verification via cosimulation against reference ISA simulators or formal interface specifcations.

BlackParrot source code is contained within ./black-parrot-sim/rtl. This is the processor design currently being fuzzed.
Surelog toolsuite is contained within ./Surelog. Compiles SV RTL to UHDM IR necessary for the walker to identify Control Expressions.
The walker code is in ./src
There are helper scripts to automate fuzzing iterations in ./scripts
Mutation of test sequences is part of a different repository that will be merged soon.
