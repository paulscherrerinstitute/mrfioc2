include /ioc/tools/driver.makefile

PROJECT=mrfioc2

BUILDCLASSES=Linux
EXCLUDE_VERSIONS=3.13 3.14.8
# build for IFC, PPMAC, PC
ARCH_FILTER=eldk52-e500v2 eldk42-ppc4xxFP SL%
#ARCH_FILTER=eldk52-e500v2


SOURCES+=../evrMrmApp/src/support/asub.c
SOURCES+=../evrMrmApp/src/devSupport/devWfMailbox.c
#SOURCES+=../evrMrmApp/src/evrdump.c
SOURCES+=../evgMrmApp/src/seqnsls2.c
SOURCES+=../evgMrmApp/src/seqconst.c
#SOURCES+=../mrfCommon/src/linkoptionsTest.c
SOURCES+=../mrfCommon/src/devMbboDirectSoft.c
#SOURCES+=../mrfCommon/src/FracSynthAnalyze.c
#SOURCES+=../mrfCommon/src/aSubRecord.c # aSubRecord is part of epics base!
SOURCES+=../mrfCommon/src/linkoptions.c
#SOURCES+=../mrfCommon/src/FracSynthControlWord.c
SOURCES+=../mrfCommon/src/mrfFracSynth.c


SOURCES+=../mrmShared/src/sfp.cpp
SOURCES+=../mrmShared/src/mrmDataBufTx.cpp
SOURCES+=../mrmShared/src/devMrfBufTx.cpp
SOURCES+=../mrmShared/src/spi_flash.c
SOURCES+=../mrmShared/src/mrmremoteflash.cpp



SOURCES+=../evrMrmApp/src/devSupport/devEvrStringIO.cpp
SOURCES+=../evrMrmApp/src/devSupport/devEvrPulserMapping.cpp
SOURCES+=../evrMrmApp/src/support/ntpShm.cpp
SOURCES+=../evrMrmApp/src/devSupport/devEvrEvent.cpp
SOURCES+=../evrMrmApp/src/support/evrGTIF.cpp
SOURCES+=../evrMrmApp/src/evr.cpp
#SOURCES+=../evrApp/src/ntpShmNull.cpp
#SOURCES+=../evrApp/src/evrMain.cpp
SOURCES+=../evrMrmApp/src/devSupport/devEvrMapping.cpp
SOURCES+=../evrMrmApp/src/devMrmBuf.cpp
SOURCES+=../evrMrmApp/src/evrPulser.cpp
SOURCES+=../evrMrmApp/src/evrIocsh.cpp
SOURCES+=../evrMrmApp/src/bufrxmgr.cpp
SOURCES+=../evrMrmApp/src/devMrmBufRx.cpp
SOURCES+=../evrMrmApp/src/evrMrm.cpp
SOURCES+=../evrMrmApp/src/evrInput.cpp
SOURCES+=../evrMrmApp/src/evrRxBuf.cpp
SOURCES+=../evrMrmApp/src/os/default/irqHack.cpp
#SOURCES+=../evrMrmApp/src/os/RTEMS/irqHack.cpp
#SOURCES+=../evrMrmApp/src/evrmrmMain.cpp
SOURCES+=../evrMrmApp/src/evrPrescaler.cpp
SOURCES+=../evrMrmApp/src/evrCML.cpp
SOURCES+=../evrMrmApp/src/evrOutput.cpp
SOURCES+=../evgMrmApp/src/evgSequencer/evgSeqRam.cpp
SOURCES+=../evgMrmApp/src/evgSequencer/evgSoftSeq.cpp
SOURCES+=../evgMrmApp/src/evgSequencer/evgSeqRamManager.cpp
SOURCES+=../evgMrmApp/src/evgSequencer/evgSoftSeqManager.cpp
SOURCES+=../evgMrmApp/src/evgOutput.cpp
SOURCES+=../evgMrmApp/src/evgEvtClk.cpp
#SOURCES+=../evgMrmApp/src/evgMrmMain.cpp
SOURCES+=../evgMrmApp/src/evgSoftEvt.cpp
SOURCES+=../evgMrmApp/src/evgTrigEvt.cpp
SOURCES+=../evgMrmApp/src/devSupport/devEvgDbus.cpp
SOURCES+=../evgMrmApp/src/devSupport/devEvgTrigEvt.cpp
SOURCES+=../evgMrmApp/src/devSupport/devEvgMrm.cpp
SOURCES+=../evgMrmApp/src/devSupport/devEvgSoftSeq.cpp
SOURCES+=../evgMrmApp/src/evgAcTrig.cpp
SOURCES+=../evgMrmApp/src/evgDbus.cpp
SOURCES+=../evgMrmApp/src/evgMxc.cpp
SOURCES+=../evgMrmApp/src/evgInput.cpp
SOURCES+=../evgMrmApp/src/evgMrm.cpp
SOURCES+=../evgMrmApp/src/evg.cpp
SOURCES+=../evgMrmApp/src/evgInit.cpp
SOURCES+=../evgMrmApp/src/evgFct.cpp
SOURCES+=../mrfCommon/src/mrfCommon.cpp
SOURCES+=../mrfCommon/src/devObjMBBDirect.cpp
SOURCES+=../mrfCommon/src/devObjWf.cpp
SOURCES+=../mrfCommon/src/devObjLong.cpp
SOURCES+=../mrfCommon/src/object.cpp
SOURCES+=../mrfCommon/src/devObjAnalog.cpp
SOURCES+=../mrfCommon/src/devObjString.cpp
SOURCES+=../mrfCommon/src/databuf.cpp
SOURCES+=../mrfCommon/src/devObjBinary.cpp
SOURCES+=../mrfCommon/src/objectTest.cpp
SOURCES+=../mrfCommon/src/devObjMBB.cpp
#SOURCES+=../mrmtestApp/src/mrfMain.cpp

SOURCES+=../evrMrmApp/src/evrGpio.cpp
SOURCES+=../evrMrmApp/src/evrDelayModule.cpp

DBDS+=../mrmShared/src/mrmShared.dbd
DBDS+=../evrMrmApp/src/evrSupport.dbd
DBDS+=../evgMrmApp/src/evgInit.dbd
#DBDS+=../mrfCommon/src/aSubRecord.dbd # aSubRecord is part of epics base!
DBDS+=../mrfCommon/src/mrfCommon.dbd

# regDev Support
SOURCES+=mrfioc2RegDev/drvMrfiocDBuff.cpp
DBDS+=mrfioc2RegDev/mrfregdev.dbd
 
HEADERS+=../evrMrmApp/src/devMrmBuf.h
