## EVG SETUP ##
var dbTemplateMaxVars 500

mrmEvgSetupVME($(EVG=EVG0),$(EVG_SLOT=2),$(EVG_MEMOFFSET=0x000000000),$(EVG_IRQLINE=0x1),$(EVG_IRQVECT=0x1)
dbLoadTemplate("$(EVG_SUBS=cfg/$(EVG=EVG0).subs)","SYS=$(SYS),EVG=$(EVG=EVG0)")
#mrfiocDBuffConfigure EVGDBUFF $(EVG=EVG0) 1