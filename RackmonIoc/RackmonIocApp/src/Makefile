TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#-- macros related to making a device support library
LIBRARY_IOC += devRackmon
DBD += devRackmon.dbd
devRackmon_SRCS += devDS1621.c devBiRackProt.c dev_i2c.c 
devRackmon_LIBS += $(EPICS_BASE_IOC_LIBS)



#=============================
# Build the IOC application

PROD_LIBS += stream
PROD_LIBS += asyn
PROD_LIBS += calc

PROD_IOC = RackmonIoc
# RackmonIoc.dbd will be created and installed
DBD += RackmonIoc.dbd

# RackmonIoc.dbd will be made up from these files:
RackmonIoc_DBD += base.dbd

# Include dbd files from all support applications:
RackmonIoc_DBD += asyn.dbd
RackmonIoc_DBD += stream.dbd
RackmonIoc_DBD += calcSupport.dbd
RackmonIoc_DBD += drvAsynSerialPort.dbd drvAsynIPPort.dbd
RackmonIoc_DBD += devRackmon.dbd

# Add all the support libraries needed by this IOC
RackmonIoc_LIBS += devRackmon
RackmonIoc_LIBS += stream asyn calc

# RackmonIoc_registerRecordDeviceDriver.cpp derives from RackmonIoc.dbd
RackmonIoc_SRCS += RackmonIoc_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
RackmonIoc_SRCS_DEFAULT += RackmonIocMain.cpp
RackmonIoc_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#RackmonIoc_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
RackmonIoc_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

