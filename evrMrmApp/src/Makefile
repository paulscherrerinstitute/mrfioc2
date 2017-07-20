TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the modular register map event receiver library
LIBRARY_IOC += evrMrm

INC += evrMrm.h
INC += evrInput.h
INC += evrOutput.h
INC += evrPulser.h
INC += evrCML.h
INC += evrPrescaler.h
INC += evrRegMap.h
INC += evrGpio.h
INC += evrDelayModule.h
INC += evrSequencer.h

#INC += support/evrGTIF.h => evrApp/src

#SRC_DIRS += ../support  ../devSupport  => all sources moved to evrApp/src

#evrMrm_SRCS += evrGTIF.cpp => evrApp/src
#evrMrm_SRCS += devEvrStringIO.cpp => evrApp/src
#evrMrm_SRCS += devEvrEvent.cpp => evrApp/src
#evrMrm_SRCS += devEvrMapping.cpp => evrApp/src
#evrMrm_SRCS += devEvrPulserMapping.cpp => evrApp/src
#evrMrm_SRCS += devWfMailbox.c => evrApp/src
#evrMrm_SRCS += asub.c => evrApp/src => evrApp/src
#evrMrm_SRCS_Linux += ntpShm.cpp => evrApp/src
#evrMrm_SRCS_DEFAULT += ntpShmNull.cpp => evrApp/src

evrMrm_SRCS += evrIocsh.c
evrMrm_SRCS += evr.cpp
evrMrm_SRCS += evrMrm.cpp
evrMrm_SRCS += evrOutput.cpp
evrMrm_SRCS += evrInput.cpp
evrMrm_SRCS += evrPrescaler.cpp
evrMrm_SRCS += evrPulser.cpp
evrMrm_SRCS += evrCML.cpp
evrMrm_SRCS += evrDelayModule.cpp
evrMrm_SRCS += evrSequencer.cpp

evrMrm_SRCS += irqHack.cpp

evrMrm_SRCS += evrGpio.cpp

ifeq ($(OS),Windows_NT)
evrMrm_LIBS += evgMrm mrfCommon mrmShared epicspci epicsvme $(EPICS_BASE_IOC_LIBS)
endif

#=============================
# Install the modular register map event receiver support dbd

#DBD += evrSupport.dbd => evrApp/src

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

