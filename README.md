# DCS Rackmon

EPICS IOC for the Mu2e experiment rack monitor box based on BeagleBone Black and custom interface board.
This includes setup for cross-compiling on a linux x86_64 machine.

## Set up

For cross-compiling, you need to first install the cross-compiler from
https://www.digikey.com/eewiki/display/linuxonarm/BeagleBoard#BeagleBoard-ARMCrossCompiler:GCC
in your home directory.
As of 2020-08-13 this was gcc-linaro-6.5.0-2018.12-x86_64_arm-linux-gnueabihf.

Then 
<pre>
source setup_dcs.sh
</pre>
to set up the paths and environment variables for compiling and running the EPICS libraries.

## Build

Use
<pre>
./build.sh
</pre>
to compile all components and move the libraries and executables into the top level.

## Deploy

The script
<pre>
./make_tgz.sh
</pre>
will make a `tgz` directory and create two gzipped tar files in it named



## To Do

TODO: more detailed documentation inside of `.md` files in the `docs` folder.

Coming soon: packaging.
