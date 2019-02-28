#!/bin/sh

FFEAD_CPPPTH=$1
export FFEAD_CPP_PATH=${FFEAD_CPPPTH}
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${FFEAD_CPPPTH}/lib

cd $FFEAD_CPP_PATH/rtdcf/
cmake .
make
cp -f *inter* $FFEAD_CPP_PATH/lib/