#!../../bin/linux-arm/RackmonIoc

#< envPaths

## Register all support components
dbLoadDatabase("../../dbd/RackmonIoc.dbd",0,0)
RackmonIoc_registerRecordDeviceDriver(pdbbase) 

## Load record instances
dbLoadRecords("../../db/RackmonIoc.db","detector=TEST")

iocInit()

