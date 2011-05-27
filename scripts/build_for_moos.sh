#!/bin/bash -e

pushd ..
GOBY_CMAKE_FLAGS="-Dbuild_moos=ON -DBUILD_TESTING=OFF" ./BUILD
popd
