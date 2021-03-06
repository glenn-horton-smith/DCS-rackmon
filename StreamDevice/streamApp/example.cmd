#!/bin/sh
##########################################################################
# This is an example and debug EPICS startup script for StreamDevice.
#
# (C) 2010 Dirk Zimoch (dirk.zimoch@psi.ch)
#
# This file is part of StreamDevice.
#
# StreamDevice is free software: You can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# StreamDevice is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with StreamDevice. If not, see https://www.gnu.org/licenses/.
#########################################################################/

exec O.$EPICS_HOST_ARCH/streamApp $0
dbLoadDatabase "O.Common/streamApp.dbd"
streamApp_registerRecordDeviceDriver

#where can protocols be located?
epicsEnvSet "STREAM_PROTOCOL_PATH", ".:protocols:../protocols/"

#setup the busses

#example serial port setup
#drvAsynSerialPortConfigure "COM2", "/dev/ttyS1"
#asynOctetSetInputEos "COM2",0,"\r\n"
#asynOctetSetOutputEos "COM2",0,"\r\n"
#asynSetOption ("COM2", 0, "baud", "9600")
#asynSetOption ("COM2", 0, "bits", "8")
#asynSetOption ("COM2", 0, "parity", "none")
#asynSetOption ("COM2", 0, "stop", "1")
#asynSetOption ("COM2", 0, "clocal", "Y")
#asynSetOption ("COM2", 0, "crtscts", "N")

#example telnet style IP port setup
drvAsynIPPortConfigure "terminal", "localhost:40000"

# Either set terminators here or in the protocol
asynOctetSetInputEos "terminal",0,"\r\n"
asynOctetSetOutputEos "terminal",0,"\r\n"

#example VXI11 (GPIB via IP) port setup
#vxi11Configure "GPIB","ins023",1,5.0,"hpib"

#load the records
dbLoadRecords "example.db","PREFIX=DZ"

#log debug output to file
#streamSetLogfile StreamDebug.log

#lots(!) of debug output before iocInit
#var streamDebug 1

iocInit

#enable debug output
#var streamDebug 1
