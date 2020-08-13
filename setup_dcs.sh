if [ "${BASH_SOURCE[0]}" -ef "$0" ]
then
    echo "Hey, you should source this script, not execute it!"
    exit 1
fi

export EPICS_BASE=$PWD/epics-base
export EPICS_HOST_ARCH=`$EPICS_BASE/startup/EpicsHostArch`
export PATH="$EPICS_BASE/bin/$EPICS_HOST_ARCH/:$PATH"

# 
if ( arch | grep arm ); then
  echo "You are on an arm-based machine, hopefully a BeagleBone."
  echo "No cross-compiler needed. But compilation may be slow."
else
  want_cross_compiler=gcc-linaro-6.5.0-2018.12-x86_64_arm-linux-gnueabihf
  export GCC_ARM_FQ_DIR=$HOME/$want_cross_compiler
  if [ ! -e $GCC_ARM_FQ_DIR/bin ]; then
    echo "Hey, sorry, I need you to install $want_cross_compiler in your home directory so I can cross-compile for the Beagle."
    echo "See https://www.digikey.com/eewiki/display/linuxonarm/BeagleBoard#BeagleBoard-ARMCrossCompiler:GCC"
    exit 1
  fi
fi

