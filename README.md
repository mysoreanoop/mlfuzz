## Instrumentation Tools-suite
Instrumentation of (System)Verilog designs for a methodical identification of key design elements. One potential application is coverage-driven automated fuzzing for hardware verification via cosimulation against reference ISA simulators.

#### Folder descriptions
- BlackParrot source code in SV is contained within `./black-parrot-sim/rtl`. This is the processor design we fuzz; for other processor designs, update the design folder appropriately. You will need to update the invocation method, the top-module name and other parameters as well.
- Surelog toolsuite is contained within `./Surelog`. Compiles SV RTL to UHDM for the walker to identify Control Expressions.
- The walker code for extracting control expressions is in [src/controls.cpp](./src/controls.cpp).
- There are helper scripts to automate fuzzing iterations in `./scripts` (some explained below)
- Generated files are in `./results/`
- [WIP] `./mutation/` contains code for mutating test sequences

#### Setup
- [BlackParrot](https://github.com/black-parrot/black-parrot-sim) if instrumenting it
- [BlackParrot HDK](https://github.com/black-parrot-hdk/zynq-parrot) if requiring FPGA cosimulation, and
- [Surelog](https://github.com/chipsalliance/Surelog)

#### Run
In `./scripts/`, run:
- `./infest.sh` for building the walker
- `./identify.sh` for ID'ing control expressions in the example BlackParrot design
- [TODO] `./fuzz.sh` for fuzzing for verification with rudimentary mutations, on BP
- [WIP] FPGA-cosimulation against Dromajo is now only ever so sligtly more elaborate

