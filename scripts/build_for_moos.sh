#!/bin/bash -e

pushd ..
echo "./CONFIGURE"
./CONFIGURE -Dbuild_moos=ON -Dbuild_core=ON -Dbuild_core_bin=OFF -Dbuild_core_moosmimic=OFF -Dbuild_core_examples=OFF -Dbuild_test=OFF
echo "./INSTALL"
./INSTALL
popd
