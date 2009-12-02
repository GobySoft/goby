#!/bin/bash
set -e -u

pushd build
cmake ../src
make $@
popd

