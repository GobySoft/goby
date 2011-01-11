#!/bin/bash -e

pushd ..
echo "./CONFIGURE -Dbuild_moos=ON -Dbuild_core=OFF"
./CONFIGURE -Dbuild_moos=ON -Dbuild_core=OFF
echo "./INSTALL"
./INSTALL
popd
