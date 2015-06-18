## Load IFC1210 devLib and pev modules
require 'pev'
## Load mrfioc2 device support
require 'mrfioc2','slejko_t'
#var pevIntrDebug 255

epicsEnvSet SYS "MTEST-VME-CSL2"
var dbTemplateMaxVars 500

##########################
#-----! EVG Setup ------!#
##########################
## Configure EVG
## Arguments:
##  - device name
##  - slot number
##  - A24 base address
##  - IRQ level
##  - IRQ vector

mrmEvgSetupVME(EVG0,3,0x000000000,0x1,0x1);

##EVG Databases:
dbLoadTemplate("evg.subs","SYS=MTEST-VME-CSL2,EVG=EVG0")



##########################
#-----! EVR Setup ------!#
##########################
## Configure EVG
## Arguments:
##  - device name
##  - slot number
##  - A24 base address
##  - IRQ level
##  - IRQ vector

mrmEvrSetupVME(EVR0,4,0x3000000,4,0x28);
dbLoadTemplate("evr.subs","SYS=MTEST-VME-CSL2,EVR=EVR0")

