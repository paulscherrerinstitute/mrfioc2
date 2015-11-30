## EVR SETUP ##
var dbTemplateMaxVars 500

mrmEvrSetupVME($(EVR=EVR0),$(EVR_SLOT=3),$(EVR_MEMOFFSET=0x3000000), $(EVR_IRQLINE=5), $(EVR_IRQVECT=0x26));

dbLoadTemplate $(EVR_SUBS=cfg/$(EVR=EVR0).subs),"SYS=$(SYS),EVR=$(EVR=EVR0)"


##Health monitoring: 
dbLoadRecords "evr-health.template", "SYS=$(SYS),EVR=$(EVR=EVR0)"
