#!/bin/bash

function die {
    status=$?
    echo failed with status $status
    exit $status
}

source setup_dcs.sh || die

for dir in epics-base #asyn4-21 calc-3-1 StreamSupport SlowMonConBoxIocApp;
do
    ( cd $dir && make ) || die
done

mkdir -p lib/linux-arm
mkdir -p bin/linux-arm

( cd lib/linux-arm && ln -sf ../../*/lib/linux-arm/* . ) || die
( cd bin/linux-arm && ln -sf ../../*/bin/linux-arm/* . ) || die



    
