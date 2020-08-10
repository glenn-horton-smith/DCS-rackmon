if [ "${BASH_SOURCE[0]}" -ef "$0" ]
then
    echo "Hey, you should source this script, not execute it!"
    exit 1
fi

export EPICS_BASE=$PWD/epics-base_v3.15.7
export EPICS_HOST_ARCH=`$EPICS_BASE/startup/EpicsHostArch`
export PATH="$EPICS_BASE/bin/$EPICS_HOST_ARCH/:$PATH"
