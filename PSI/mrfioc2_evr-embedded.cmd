## EVR SETUP ##
var dbTemplateMaxVars 500

mrmEvrSetupEmbedded($(DEVICE=EVR0),$(DEVICE_PARENT=EVG0),$(TYPE));

dbLoadTemplate $(EVR_SUBS=cfg/$(DEVICE=EVR0).subs),"SYS=$(SYS),DEVICE=$(DEVICE=EVR0), DEVICE_PARENT=$(DEVICE_PARENT=EVG0)"

## PSI specific templates
dbLoadRecords $(mrfioc2_TEMPLATES)/evr-BeamOK.template,"SYS=$(SYS),DEVICE=$(DEVICE=EVR0)"


## Health monitoring: 
dbLoadRecords "$(mrfioc2_TEMPLATES=db)/evr-health.template", "SYS=$(SYS),DEVICE=$(DEVICE=EVR0)"
