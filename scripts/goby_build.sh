#!/bin/bash
set -e -u

pushd ../build
cmake ..
make $@
popd

