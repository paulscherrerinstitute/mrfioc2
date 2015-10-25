TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the modular register map event receiver library
LIBRARY_IOC += evrMrm

INC += evrMrm.h
INC += devMrmBuf.h
INC += evrInput.h
INC += evrOutput.h
INC += evrPulser.h
INC += evrCML.h
INC += evrPrescaler.h
INC += evrRxBuf.h
INC += evrRegMap.h
INC += evrGpio.h
INC += mrmpci.h
INC += plx9030.h
INC += plx9056.h
INC += evrDelayModule.h
INC += bufrxmgr.h

INC += support/evrGTIF.h

SRC_DIRS += ../support ../devSupport 

evrMrm_SRCS += evrGTIF.cpp
evrMrm_SRCS += devEvrStringIO.cpp
evrMrm_SRCS += devEvrEvent.cpp
evrMrm_SRCS += devEvrMapping.cpp
evrMrm_SRCS += devEvrPulserMapping.cpp
evrMrm_SRCS += devWfMailbox.c
evrMrm_SRCS += asub.c
evrMrm_SRCS_Linux += ntpShm.cpp
evrMrm_SRCS_DEFAULT += ntpShmNull.cpp

evrMrm_SRCS += evrIocsh.c
evrMrm_SRCS += evr.cpp
evrMrm_SRCS += evrMrm.cpp
evrMrm_SRCS += evrOutput.cpp
evrMrm_SRCS += evrInput.cpp
evrMrm_SRCS += evrPrescaler.cpp
evrMrm_SRCS += evrPulser.cpp
evrMrm_SRCS += evrCML.cpp
evrMrm_SRCS += evrDelayModule.cpp
evrMrm_SRCS += evrRxBuf.cpp
evrMrm_SRCS += devMrmBuf.cpp

evrMrm_SRCS += bufrxmgr.cpp
evrMrm_SRCS += devMrmBufRx.cpp

evrMrm_SRCS += irqHack.cpp

evrMrm_SRCS += evrGpio.cpp

ifeq ($(OS),Windows_NT)
evrMrm_LIBS += mrfCommon mrmShared epicspci epicsvme $(EPICS_BASE_IOC_LIBS)
endif

#=============================
# Install the modular register map event receiver support dbd

DBD += evrSupport.dbd

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

