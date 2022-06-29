## SV Instrumentation Tools-suite
Arrangement for instrumentation of (System)Verilog designs for a methodical identification of key design elements for, primarily, utility in coverage-driven automated fuzzing for hardware verification via cosimulation against reference ISA simulators.

# Folder descriptions
- BlackParrot source code in SV is contained within `./black-parrot-sim/rtl`. This is the processor design we fuzz; for other processor designs, update the design folder appropriately. You will need to update the invocation method, the top-module name and other parameters as required.
- Surelog toolsuite is contained within `./Surelog`. Compiles SV RTL to UHDM for the walker to identify Control Expressions.
- The walker code for extracting control expressions is in [src/controls.cpp](./src/controls.cpp).
- There are helper scripts to automate common procedures in `./scripts` (some explained below)
- Generated files are in `./results/`
- [WIP] `./mutation/` contains code for mutating test sequences

# Setup
- [BlackParrot](https://github.com/black-parrot/black-parrot-sim)
- [Surelog](https://github.com/chipsalliance/Surelog)

# Run
Either:
1. Set up black-parrot-sim here and follow steps in black-parrot-sim to set it up (will take long).
2. Or, only fetch Surelog and update the path to a preinstalled black-parrot-sim in [identify.sh](./scripts/identify.sh) (preferred). The only change that will be made to the repository is update the Surelog executable path (reverted immediately after successful run), and of course the results of the run will remain in the repository. In case of a failed run, simply run `git restore bp_common/syn/Makefile.surelog` and optionally delete `bp_top/syn/results/surelog/<last run>`.

In [scripts](./scripts/), run:
- `./setup.sh` to setup either Surelog only, and provide path to black-parrot-sim, or, additionally uncomment black-parrot-sim section in there to set it up here.
- `./infest.sh` to copy the walker code into the repository and build it.
- `./identify.sh` for ID'ing control expressions in the example BlackParrot design
- `./rewrap.sh` for replacing the top-level wrapper with CE-instrumented wrapper
- [WIP] `./fuzz.sh` for fuzzing for verification with rudimentary mutations, on BP
