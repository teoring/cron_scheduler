#!/bin/bash

rm -rf build bin
rm -rf coverage
mkdir -p build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Debug -G 'Unix Makefiles' ../
make

cd ..
mkdir -p coverage
ls
lcov -c -i -d . -o ./build/run_tests.base
lcov -c -d . -o ./build/run_tests.run
lcov -d . -a ./build/run_tests.base -a ./build/run_tests.run -o ./coverage/run_tests.total
cd coverage
genhtml --no-branch-coverage -o . run_tests.total