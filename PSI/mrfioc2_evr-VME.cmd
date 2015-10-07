## EVR SETUP ##

mrmEvrSetupVME($(EVR=EVR0),$(EVR_SLOT=3),$(EVR_MEMOFFSET=0x3000000), $(EVR_IRQLINE=5), $(EVR_IRQVECT=0x26));
mrfiocDBuffConfigure $(EVR=EVR0)DBUFF $(EVR=EVR0) 1

dbLoadTemplate $(EVR_SUBS=cfg/$(EVR=EVR0).subs),"SYS=$(SYS),EVR=$(EVR=EVR0)"

# ##Babaks databuffer receive template
#dbLoadRecords("${EVR_TEMPLATE_DIR}/evr-bunchId_Rx.template", "SYS=$(SYS)-DBUF,EVR=$(EVR=EVR0),EVRDBUFF=EVRDBUFF")


##Health monitoring: 
dbLoadRecords "evr-health.template", "SYS=$(SYS),EVR=$(EVR=EVR0)"



