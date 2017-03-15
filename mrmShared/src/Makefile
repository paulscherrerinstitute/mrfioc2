
TOP=../..
include $(TOP)/configure/CONFIG

LIBRARY_IOC += mrmShared
INC += sfp.h
INC += mrmShared.h
INC += mrmFlash.h
INC += mrmRemoteFlash.h
INC += dataBuffer/mrmDataBuffer.h
INC += dataBuffer/mrmDataBuffer_300.h
INC += dataBuffer/mrmDataBuffer_230.h
INC += dataBuffer/mrmDataBufferUser.h
INC += dataBuffer/mrmDataBufferObj.h
INC += dataBuffer/mrmDataBufferType.h
INC += mrmDeviceInfo.h
INC += mrmSoftEvent.h

DBD += mrmShared.dbd

SRC_DIRS += ../dataBuffer

mrmShared_SRCS += mrmDataBuffer.cpp
mrmShared_SRCS += mrmDataBuffer_300.cpp
mrmShared_SRCS += mrmDataBuffer_230.cpp
mrmShared_SRCS += mrmDataBufferUser.cpp
mrmShared_SRCS += mrmDataBufferObj.cpp
mrmShared_SRCS += mrmDataBufferType.cpp
mrmShared_SRCS += sfp.cpp
mrmShared_SRCS += mrmFlash.cpp
mrmShared_SRCS += mrmRemoteFlash.cpp
mrmShared_SRCS += mrmDeviceInfo.cpp
mrmShared_SRCS += mrmSoftEvent.cpp

ifeq ($(OS),Windows_NT)
mrmShared_LIBS += mrfCommon $(EPICS_BASE_IOC_LIBS)
mrmShared_SYS_LIBS += WS2_32
endif

#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES
