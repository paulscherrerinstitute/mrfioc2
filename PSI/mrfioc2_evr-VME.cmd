## EVR SETUP ##
var dbTemplateMaxVars 500

mrmEvrSetupVME($(DEVICE=EVR0),$(EVR_SLOT=3),$(EVR_MEMOFFSET=0x3000000), $(EVR_IRQLINE=5), $(EVR_IRQVECT=0x26));

dbLoadTemplate $(EVR_SUBS=cfg/$(DEVICE=EVR0).subs),"SYS=$(SYS),DEVICE=$(DEVICE=EVR0)"


## Health monitoring: 
dbLoadRecords "$(mrfioc2_TEMPLATES=db)/evr-health.template", "SYS=$(SYS),DEVICE=$(DEVICE=EVR0)"

## Template for storing event patterns - userfull for debugging but disabled by default
dbLoadRecords "$(mrfioc2_TEMPLATES=db)/evr-eventPatternCheck.template", "SYS=$(SYS),DEVICE=$(DEVICE=EVR0),EVT=6,DISABLE=1"
