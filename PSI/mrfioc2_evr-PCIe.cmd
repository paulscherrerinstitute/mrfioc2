## EVR SETUP ##
var dbTemplateMaxVars 500

mrmEvrSetupPCI($(DEVICE=EVR0),$(EVR_DOMAIN=0x1),$(EVR_BUS=0x21), $(EVR_DEVICE=0), $(EVR_FUNCTION=0));

dbLoadTemplate $(EVR_SUBS=cfg/$(DEVICE=EVR0).subs),"SYS=$(SYS),DEVICE=$(DEVICE=EVR0)"

##Health monitoring: 
dbLoadRecords "$(mrfioc2_TEMPLATES=db)/evr-health.template", "SYS=$(SYS),DEVICE=$(DEVICE=EVR0)"
