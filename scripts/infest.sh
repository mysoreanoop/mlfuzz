#!/bin/bash
cp ../src/controls.cpp ../Surelog/src/
sed "|^add_executable(hellodesign ${PROJECT_SOURCE_DIR}/src/hellodesign.cpp)|add_executable(controls ${PROJECT_SOURCE_DIR}/src/controls.cpp)|" ../Surelog/CMakeLists.txt
sed "|^target_link_libraries(hellodesign surelog)|target_link_libraries(controls surelog)|" ../Surelog/CMakeLists.txt
make -C ../Surelog/
