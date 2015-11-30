## EVR SETUP ##
var dbTemplateMaxVars 500

mrmEvrSetupPCI($(EVR=EVR0),$(EVR_DOMAIN=0x1),$(EVR_BUS=0x21), $(EVR_DEVICE=0), $(EVR_FUNCTION=0));

dbLoadTemplate $(EVR_SUBS=cfg/$(EVR=EVR0).subs),"SYS=$(SYS),EVR=$(EVR=EVR0)"


##Health monitoring: 
dbLoadRecords "evr-health.template", "SYS=$(SYS),EVR=$(EVR=EVR0)"
