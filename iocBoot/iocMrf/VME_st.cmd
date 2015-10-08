#!../../bin/linux-x86_64/mrf

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/mrf.dbd"
mrf_registerRecordDeviceDriver pdbbase

var dbTemplateMaxVars 500

## Set the system name
epicsEnvSet SYS "MTEST-PCI-TIMINGTEST"

##########################
#-----! EVG Setup ------!#
##########################
## The following parameters are available to set up the device. They can either be set as an epics environmental variable, or passed directly in the mrmEvgSetupVME command:

epicsEnvSet  EVG		"EVG0"			## is the event generator card name
epicsEnvSet  EVG_SLOT		2			## is the VME crate slot where the card is inserted
epicsEnvSet  EVG_MEMOFFSET	0x0			## is the base A24 address
epicsEnvSet  EVG_IRQLINE 	0x2			## is the interrupt level
epicsEnvSet  EVG_IRQVECT 	0x1			## is the interrupt vector

mrmEvgSetupVME($(EVG), $(EVG_SLOT), $(EVG_MEMOFFSET), $(EVG_IRQLINE), $(EVG_IRQVECT))




##########################
#-----! EVR Setup ------!#
##########################
## The following parameters are available to set up the device.They can either be set as an epics environmental variable, or passed directly in the mrmEvrSetupVME command:

epicsEnvSet  EVR		"EVR0"			## is the event receiver card name
epicsEnvSet  EVR_SLOT		3			## is the VME crate slot where EVR is inserted
epicsEnvSet  EVR_MEMOFFSET	0x3000000		## is the base A24 address
epicsEnvSet  EVR_IRQLINE 	0x5			## is the interrupt level
epicsEnvSet  EVR_IRQVECT	0x26			## is the interrupt vector

mrmEvrSetupVME($(EVR), $(EVR_SLOT), $(EVR_MEMOFFSET), $(EVR_IRQLINE), $(EVR_IRQVECT))




##########################
#----! Load records ----!#
##########################
## It is recommended to put record loading in a seperate substitution file. 
## Records contain many macros which can be used to set starting values of the device components. Refer to the documentation for details.
## For other options inspect substitution files in $(TOP)/PSI folder. They also contain all macro substitutions available. Theese example substitution files can be reused if SYS and EVR macros are added to each record inclusion. 

## Load form factor and/or firmware specific records with default device component values.
dbLoadRecords("db/evr-vme-300.db", "SYS=$(SYS), EVR=$(EVR)")
dbLoadRecords("db/evg-vme-300.db", "SYS=$(SYS), EVG=$(EVG)")

#dbLoadRecords("db/evr-vme-230.db", "SYS=$(SYS), EVR=$(EVR)")
#dbLoadRecords("db/evg-vme-230.db", "SYS=$(SYS), EVG=$(EVG)")


cd ${TOP}/iocBoot/${IOC}
iocInit


