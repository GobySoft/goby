#!/bin/bash

pushd ~/goby/1.0/build
ctest -D NightlyStart
ctest -D NightlyUpdate
ctest -D NightlyConfigure
ctest -D NightlyBuild
#ctest -D NightlyTest
#ctest -D NightlyMemCheck
#ctest -D NightlyCoverage
ctest -D NightlySubmit
popd
