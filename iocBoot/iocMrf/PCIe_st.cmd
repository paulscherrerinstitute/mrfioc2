#!../../bin/linux-x86_64/mrf

## You may have to change mrf to something else
## everywhere it appears in this file

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase "dbd/mrf.dbd"
mrf_registerRecordDeviceDriver pdbbase

var dbTemplateMaxVars 500

## Set the system name
epicsEnvSet SYS "MTEST-PCI-TIMINGTEST"

##########################
#-----! EVR Setup ------!#
##########################
## The following parameters are available to set up the device.They can either be set as an epics environmental variable, or passed directly in the mrmEvrSetupVME command:

epicsEnvSet EVR			"EVR0" 		## is the event receiver card name. (default: EVR0)
epicsEnvSet EVR_DOMAIN  	0x1 		## is the PCI domain where the card is inserted. (default: 0x1)
epicsEnvSet EVR_BUS 		0x21 		## is the PCI bus where the card is inserted. (default: 0x21)
epicsEnvSet EVR_DEVICE  	0 		## is the PCI device where the card is inserted. (default: 0)
epicsEnvSet EVR_FUNCTION 	0 		## is the PCI function where the card is inserted. (default: 0)

mrmEvrSetupPCI($(EVR), $(EVR_DOMAIN), $(EVR_BUS), $(EVR_DEVICE), $(EVR_FUNCTION))



##########################
#----! Load records ----!#
##########################
## It is recommended to put record loading in a seperate substitution file. 
## Records contain many macros which can be used to set starting values of the device components. Refer to the documentation for details.
## For other options inspect substitution files in $(TOP)/PSI/example folder. They also contain all macro substitutions available. Theese example substitution files can be reused if SYS and EVR macros are added to each record inclusion.

## Load form factor and/or firmware specific records with default device component values.
dbLoadRecords("db/evr-pcie-300.db", "SYS=$(SYS), EVR=$(EVR)")

#dbLoadRecords("db/evr-cpci-230.db", "SYS=$(SYS), EVR=$(EVR)")


cd ${TOP}/iocBoot/${IOC}
iocInit


