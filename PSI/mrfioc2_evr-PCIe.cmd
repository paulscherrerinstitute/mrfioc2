## EVR SETUP ##

mrmEvrSetupPCI($(EVR=EVR0),$(EVR_DOMAIN=0x1),$(EVR_BUS=0x21), $(EVR_DEVICE=0), $(EVR_FUNCTION=0));

mrfiocDBuffConfigure $(EVR=EVR0)DBUFF $(EVR=EVR0) 1

dbLoadTemplate $(EVR_SUBS=cfg/$(EVR=EVR0).subs),"SYS=$(SYS),EVR=$(EVR=EVR0)"


# ##Babaks databuffer receive template
#dbLoadRecords("${EVR_TEMPLATE_DIR}/evr-bunchId_Rx.template", "SYS=$(SYS)-DBUF,EVR=$(EVR=EVR0),EVRDBUFF=EVRDBUFF")


##Health monitoring: 
dbLoadRecords "evr-health.template", "SYS=$(SYS),EVR=$(EVR=EVR0)"



