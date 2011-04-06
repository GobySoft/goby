#!/bin/bash

pushd ../build
ctest -D ExperimentalStart
ctest -D ExperimentalUpdate
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalMemCheck
ctest -D ExperimentalCoverage
ctest -D ExperimentalSubmit
