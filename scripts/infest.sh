#!/bin/bash
cp ../src/controls.cpp ../Surelog/src/
cd ../Surelog/
# Older versions of Surelog needed this:
# sed -i 's|git://github.com/nemtrif/utfcpp|https://github.com/nemtrif/utfcpp.git|' third_party/antlr4_fast/runtime/Cpp/runtime/CMakeLists.txt
git restore CMakeLists.txt
sed -i '\|^add_executable(hellodesign ${PROJECT_SOURCE_DIR}/src/hellodesign.cpp)|a add_executable(controls ${PROJECT_SOURCE_DIR}/src/controls.cpp)' CMakeLists.txt
sed -i '\|^target_link_libraries(hellodesign surelog)|a target_link_libraries(controls surelog)' CMakeLists.txt
make
cd -
