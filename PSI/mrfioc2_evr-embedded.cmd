## EVR SETUP ##
var dbTemplateMaxVars 500

mrmEvrSetupEmbedded($(DEVICE=EVR0),$(DEVICE_PARENT=EVG0),$(TYPE));

dbLoadTemplate $(EVR_SUBS=cfg/$(DEVICE=EVR0).subs),"SYS=$(SYS),DEVICE=$(DEVICE=EVR0), DEVICE_PARENT=$(DEVICE_PARENT=EVG0)"

