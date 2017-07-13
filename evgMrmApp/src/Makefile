TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

LIBRARY_IOC += evgMrm

INC += evgMrm.h
INC += evgRegMap.h
INC += evgAcTrig.h
INC += evgEvtClk.h
INC += evgTrigEvt.h
INC += evgMxc.h
INC += evgDbus.h
INC += evgInput.h
INC += evgOutput.h
INC += evgFct.h
INC += evgPhaseMonSel.h

INC += evgSequencer/evgSoftSeqManager.h
INC += evgSequencer/evgSoftSeq.h
INC += evgSequencer/evgSeqRamManager.h
INC += evgSequencer/evgSeqRam.h

SRC_DIRS += ../evgSequencer ../devSupport 

#All the source files that are compiled and put in the library.
evgMrm_SRCS += evgInit.cpp

evgMrm_SRCS += evg.cpp

evgMrm_SRCS += evgMrm.cpp
evgMrm_SRCS += devEvgMrm.cpp

evgMrm_SRCS += evgAcTrig.cpp

evgMrm_SRCS += evgEvtClk.cpp

evgMrm_SRCS += evgTrigEvt.cpp
evgMrm_SRCS += devEvgTrigEvt.cpp

evgMrm_SRCS += evgMxc.cpp

evgMrm_SRCS += evgDbus.cpp
evgMrm_SRCS += devEvgDbus.cpp

evgMrm_SRCS += evgInput.cpp

evgMrm_SRCS += evgOutput.cpp

evgMrm_SRCS += evgFct.cpp

evgMrm_SRCS += evgSoftSeq.cpp
evgMrm_SRCS += evgSoftSeqManager.cpp
evgMrm_SRCS += devEvgSoftSeq.cpp

evgMrm_SRCS += evgSeqRam.cpp
evgMrm_SRCS += evgSeqRamManager.cpp

evgMrm_SRCS += seqconst.c
evgMrm_SRCS += seqnsls2.c

evgMrm_SRCS += evgPhaseMonSel.cpp


ifeq ($(OS),Windows_NT)
evgMrm_LIBS += mrfCommon mrmShared epicsvme epicspci $(EPICS_BASE_IOC_LIBS)
endif

DBD += evgInit.dbd

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
