#!/bin/bash

pushd ../build
rm -rf *
cmake .. -DCMAKE_BUILD_TYPE=Debug -Dbuild_doc=ON
ctest -D ExperimentalStart
ctest -D ExperimentalUpdate
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalMemCheck
ctest -D ExperimentalCoverage
ctest -D ExperimentalSubmit
