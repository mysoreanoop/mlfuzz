## mlfuzz
Coverage-driven automated fuzzing for hardware verification via cosimulation against reference ISA simulators.

# Folder descriptions
- BlackParrot source code in SV is contained within ./black-parrot-sim/rtl. This is the processor design we fuzz for example.
- Surelog toolsuite is contained within ./Surelog. Compiles SV RTL to UHDM IR (more in ./Surelog/third\_party/UHDM/) for the walker to identify Control Expressions.
- The walker code for extracting control expressions is in ./src/controls.cpp. Other applications of walking UHDM include more convenient side-channels detection by fuzzing with parameterized attack gadgets (aided by helper gadgets), will appear soon.
- There are helper scripts to automate fuzzing iterations in ./scripts (some explained below)
- Generated files, collaterals are in ./results/
- [WIP] ./mutation/ contains code for mutating test sequences
- [WIP] ./zynq-parrot/ contains BlackParrot's Zynq FPGA Shell (HDK) and instructions for Dromajo co-simulation on FPGA
- [PLAN] ./rl\_fuzz/ contains RL model training code with mutation

# Setup
- Setup BlackParrot, HDK and Surelog repositories by following ./black-parrot-sim/README.md

# Run
In ./scripts/, run:
- ./infest.sh for building the walker
- ./identify.sh for ID'ing control expressions in the example BlackParrot design
- [TODO] ./secure.sh for fuzzing for side-channels detection
- [TODO] ./fuzz.sh for fuzzing for verification with rudimentary mutations, on BP
- [PLAN] ./rl\_fuzz.sh for fuzzing for verification aided by an RL model
- [WIP] FPGA-cosimulation against Dromajo is only ever so sligtly more elaborate
