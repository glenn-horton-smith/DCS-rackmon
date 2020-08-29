#!../../bin/linux-arm/RackmonIoc

# trying not to load envPaths to avoid hard-coding of TOP ...
#< envPaths

epicsEnvSet "STREAM_PROTOCOL_PATH" "./"


## Register all support components
dbLoadDatabase("../../dbd/RackmonIoc.dbd",0,0)
RackmonIoc_registerRecordDeviceDriver(pdbbase) 

## Load record instances
#Set up ASYN ports
drvAsynIPPortConfigure ("IP1", "192.168.144.193:4000")

## Load record instances
dbLoadRecords("../../db/DPO3034.db","P=IP1, detector=uB")

iocInit()
