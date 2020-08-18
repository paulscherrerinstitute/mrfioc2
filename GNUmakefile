ifeq ($(wildcard /ioc/tools/driver.makefile),)
$(warning It seems you do not have the PSI build environment. Remove GNUmakefile.)
include Makefile
else
include /ioc/tools/driver.makefile

MODULE=mrfioc2
#LIBVERSION=test

LIB_SYS_LIBS_WIN32 += WS2_32

BUILDCLASSES=Linux WIN32
EXCLUDE_VERSIONS=3.13 3.14.8
# build for IFC, PPMAC, PC
ARCH_FILTER=eldk52-e500v2 eldk42-ppc4xxFP SL% RHEL% win%
#ARCH_FILTER=eldk42-ppc4xxFP
#ARCH_FILTER=eldk52-e500v2
#ARCH_FILTER=SL%
#ARCH_FILTER=eldk52-e500v2 SL%

SOURCES+=evrMrmApp/src/support/asub.c
SOURCES+=evrMrmApp/src/devSupport/devWfMailbox.c
SOURCES+=evgMrmApp/src/seqnsls2.c
SOURCES+=evgMrmApp/src/seqconst.c
SOURCES+=mrfCommon/src/devMbboDirectSoft.c
SOURCES+=mrfCommon/src/linkoptions.c
SOURCES+=mrfCommon/src/mrfFracSynth.c


SOURCES+=mrmShared/src/sfp.cpp
SOURCES+=mrmShared/src/mrmFlash.cpp
SOURCES+=mrmShared/src/mrmRemoteFlash.cpp
SOURCES+=mrmShared/src/dataBuffer/mrmDataBuffer.cpp
SOURCES+=mrmShared/src/dataBuffer/mrmDataBuffer_300.cpp
SOURCES+=mrmShared/src/dataBuffer/mrmDataBuffer_230.cpp
SOURCES+=mrmShared/src/dataBuffer/mrmDataBufferUser.cpp
SOURCES+=mrmShared/src/dataBuffer/mrmDataBufferObj.cpp
SOURCES+=mrmShared/src/dataBuffer/mrmDataBufferType.cpp
SOURCES+=mrmShared/src/mrmDeviceInfo.cpp
SOURCES+=mrmShared/src/mrmSoftEvent.cpp

SOURCES+=evrMrmApp/src/devSupport/devEvrStringIO.cpp
SOURCES+=evrMrmApp/src/devSupport/devEvrPulserMapping.cpp
SOURCES_Linux+=evrMrmApp/src/support/ntpShm.cpp
SOURCES_WIN32+=evrMrmApp/src/support/ntpShmNull.cpp
SOURCES+=evrMrmApp/src/devSupport/devEvrEvent.cpp
SOURCES+=evrMrmApp/src/support/evrGTIF.cpp
SOURCES+=evrMrmApp/src/evr.cpp
SOURCES+=evrMrmApp/src/devSupport/devEvrMapping.cpp
SOURCES+=evrMrmApp/src/evrPulser.cpp
SOURCES+=evrMrmApp/src/evrIocsh.cpp
SOURCES+=evrMrmApp/src/evrMrm.cpp
SOURCES+=evrMrmApp/src/evrInput.cpp
SOURCES+=evrMrmApp/src/os/default/irqHack.cpp
SOURCES+=evrMrmApp/src/evrPrescaler.cpp
SOURCES+=evrMrmApp/src/evrCML.cpp
SOURCES+=evrMrmApp/src/evrOutput.cpp
SOURCES+=evrMrmApp/src/evrSequencer.cpp
SOURCES+=evgMrmApp/src/evgSequencer/evgSeqRam.cpp
SOURCES+=evgMrmApp/src/evgSequencer/evgSoftSeq.cpp
SOURCES+=evgMrmApp/src/evgSequencer/evgSeqRamManager.cpp
SOURCES+=evgMrmApp/src/evgSequencer/evgSoftSeqManager.cpp
SOURCES+=evgMrmApp/src/evgOutput.cpp
SOURCES+=evgMrmApp/src/evgEvtClk.cpp
SOURCES+=evgMrmApp/src/evgTrigEvt.cpp
SOURCES+=evgMrmApp/src/devSupport/devEvgDbus.cpp
SOURCES+=evgMrmApp/src/devSupport/devEvgTrigEvt.cpp
SOURCES+=evgMrmApp/src/devSupport/devEvgMrm.cpp
SOURCES+=evgMrmApp/src/devSupport/devEvgSoftSeq.cpp
SOURCES+=evgMrmApp/src/evgAcTrig.cpp
SOURCES+=evgMrmApp/src/evgDbus.cpp
SOURCES+=evgMrmApp/src/evgMxc.cpp
SOURCES+=evgMrmApp/src/evgInput.cpp
SOURCES+=evgMrmApp/src/evgPhaseMonSel.cpp
SOURCES+=evgMrmApp/src/evgMrm.cpp
SOURCES+=evgMrmApp/src/evg.cpp
SOURCES+=evgMrmApp/src/evgInit.cpp
SOURCES+=evgMrmApp/src/evgFct.cpp
SOURCES+=mrfCommon/src/mrfCommon.cpp
SOURCES+=mrfCommon/src/devObjMBBDirect.cpp
SOURCES+=mrfCommon/src/devObjWf.cpp
SOURCES+=mrfCommon/src/devObjLong.cpp
SOURCES+=mrfCommon/src/object.cpp
SOURCES+=mrfCommon/src/devObjAnalog.cpp
SOURCES+=mrfCommon/src/devObjString.cpp
SOURCES+=mrfCommon/src/devObjBinary.cpp
SOURCES+=mrfCommon/src/objectTest.cpp
SOURCES+=mrfCommon/src/devObjMBB.cpp

SOURCES+=evrMrmApp/src/evrGpio.cpp
SOURCES+=evrMrmApp/src/evrDelayModule.cpp

DBDS+=evrMrmApp/src/evrSupport.dbd
DBDS+=evgMrmApp/src/evgInit.dbd
DBDS+=mrfCommon/src/mrfCommon.dbd
DBDS+=mrmShared/src/mrmShared.dbd

# external data buffer support (eg. mrfioc2_regDev)
HEADERS+=mrmShared/src/dataBuffer/mrmDataBufferUser.h
HEADERS+=mrmShared/src/dataBuffer/mrmDataBufferType.h

# TESTS
#SOURCES+=mrmShared/src/dataBuffer/tests/mrmDataBuffer_test.cpp
#DBDS+=mrmShared/src/dataBuffer/tests/dataBufferTests.dbd



############# TEMPLATES #############

## EVG templates
TEMPLATES += evgMrmApp/Db/evg-vme-300.db
TEMPLATES += evgMrmApp/Db/evg-vme-230.db
TEMPLATES += evgMrmApp/Db/evgSoftSeq.template
TEMPLATES += evgMrmApp/Db/evg-health.template

##Generated EVR templates
TEMPLATES += evrMrmApp/Db/evr-cpci-230.db
TEMPLATES += evrMrmApp/Db/evr-pcie-300.db
TEMPLATES += evrMrmApp/Db/evr-pcie-300DC.db
TEMPLATES += evrMrmApp/Db/evr-vme-300.db
TEMPLATES += evrMrmApp/Db/evr-vme-230.db
TEMPLATES += evrMrmApp/Db/evr-embedded.db

## Fixed EVR templates
TEMPLATES += evrMrmApp/Db/evr-softEvent.template
TEMPLATES += evrMrmApp/Db/evr-softEvent-measure.template
TEMPLATES += evrMrmApp/Db/evr-eventPatternCheck.template
TEMPLATES += evrMrmApp/Db/evr-specialFunctionMap.template
TEMPLATES += evrMrmApp/Db/evr-pulserMap.template
TEMPLATES += evrMrmApp/Db/evr-pulserMap-dbus.template
TEMPLATES += evrMrmApp/Db/evr-delayModule.template

## EVR health monitoring (fixed template)
TEMPLATES += evrMrmApp/Db/evr-health.template

## EVR helper templates for trigger switching configuration (fixed template)
TEMPLATES += evrMrmApp/Db/evr-configTriggerSwitch.template


## EVR and EVG substitution files
TEMPLATES += PSI/example/evr_VME-230.subs
TEMPLATES += PSI/example/evr_VME-300.subs
TEMPLATES += PSI/example/evr_cPCI-230.subs
TEMPLATES += PSI/example/evr_PCIe-300.subs
TEMPLATES += PSI/example/evr_PCIe-300DC.subs
TEMPLATES += PSI/example/evr_embedded.subs
TEMPLATES += PSI/example/evg_VME-230.subs
TEMPLATES += PSI/example/evg_VME-300.subs
TEMPLATES += PSI/example/evg_VME-300-fout.subs

## Shared templates
TEMPLATES += mrmShared/Db/flash.template
TEMPLATES += mrmShared/Db/sfp.template

## GENERIC STARTUP SCRIPTS ##
SCRIPTS += PSI/mrfioc2_evr-PCIe.cmd
SCRIPTS += PSI/mrfioc2_evr-VME.cmd
SCRIPTS += PSI/mrfioc2_evr-embedded.cmd
SCRIPTS += PSI/mrfioc2_evg-VME.cmd

## SCRIPTS FOR LOADING KERNEL MODULES
SCRIPTS += PSI/loadKernelModule.sh
SCRIPTS += PSI/SL6-x86_64.cmd
SCRIPTS += PSI/SL6-x86.cmd
SCRIPTS += PSI/eldk42-ppc4xxFP.cmd
SCRIPTS += PSI/RHEL7-x86_64.cmd

db: dbclean dbexpand

dbclean: 
	
	echo "Cleaning databases"	

	rm -f evgMrmApp/Db/evg-vme-300.db
	rm -f evgMrmApp/Db/evg-vme-230.db
	rm -f evrMrmApp/Db/evr-cpci-230.db
	rm -f evrMrmApp/Db/evr-pcie-300.db
	rm -f evrMrmApp/Db/evr-vme-300.db
	rm -f evrMrmApp/Db/evr-vme-230.db
	rm -f evrMrmApp/Db/evr-embedded.db

dbexpand: 
	
	echo "Exapanding EVG database"	

	msi -I evgMrmApp/Db/ -I mrmShared/Db/ -S evgMrmApp/Db/evg-vme-300.substitutions 	-o evgMrmApp/Db/evg-vme-300.db
	msi -I evgMrmApp/Db/ -I mrmShared/Db/ -S evgMrmApp/Db/evg-vme-230.substitutions 	-o evgMrmApp/Db/evg-vme-230.db

	echo "Exapanding EVR database"	
	msi -I evrMrmApp/Db/ -I mrmShared/Db/ -S evrMrmApp/Db/evr-cpci-230.substitutions 	-o evrMrmApp/Db/evr-cpci-230.db
	msi -I evrMrmApp/Db/ -I mrmShared/Db/ -S evrMrmApp/Db/evr-pcie-300.substitutions 	-o evrMrmApp/Db/evr-pcie-300.db
	msi -I evrMrmApp/Db/ -I mrmShared/Db/ -S evrMrmApp/Db/evr-pcie-300DC.substitutions 	-o evrMrmApp/Db/evr-pcie-300DC.db
	msi -I evrMrmApp/Db/ -I mrmShared/Db/ -S evrMrmApp/Db/evr-vme-300.substitutions 	-o evrMrmApp/Db/evr-vme-300.db
	msi -I evrMrmApp/Db/ -I mrmShared/Db/ -S evrMrmApp/Db/evr-vme-230.substitutions 	-o evrMrmApp/Db/evr-vme-230.db
	msi -I evrMrmApp/Db/ -I mrmShared/Db/ -S evrMrmApp/Db/evr-embedded.substitutions 	-o evrMrmApp/Db/evr-embedded.db

gui: 
	PSI/installScreens.sh evrMrmApp/opi/EVR/
	PSI/installScreens.sh evgMrmApp/opi/EVG/

doc:
	$(MAKE) -C documentation/
	$(MAKE) -C documentation/ clean

endif

