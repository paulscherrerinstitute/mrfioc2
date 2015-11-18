
TOP=../..
include $(TOP)/configure/CONFIG

LIBRARY_IOC += mrmShared
INC += sfp.h
INC += mrmShared.h
INC += mrmremoteflash.h
INC += dataBuffer/mrmDataBuffer.h
INC += dataBuffer/mrmDataBuffer_300.h
INC += dataBuffer/mrmDataBuffer_230.h
INC += dataBuffer/mrmDataBufferUser.h

DBD += dataBuffer/mrmDataBuffer.dbd

SRC_DIRS += ../dataBuffer

mrmShared_SRCS += mrmDataBuffer.cpp
mrmShared_SRCS += mrmDataBuffer_300.cpp
mrmShared_SRCS += mrmDataBuffer_230.cpp
mrmShared_SRCS += mrmDataBufferUser.cpp
mrmShared_SRCS += sfp.cpp
mrmShared_SRCS += spi_flash.c
mrmShared_SRCS += mrmremoteflash.cpp

ifeq ($(OS),Windows_NT)
mrmShared_LIBS += mrfCommon $(EPICS_BASE_IOC_LIBS)
mrmShared_SYS_LIBS += WS2_32
endif

#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES
