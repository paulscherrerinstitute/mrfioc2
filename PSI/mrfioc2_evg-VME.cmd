## EVG SETUP ##
var dbTemplateMaxVars 500

mrmEvgSetupVME($(DEVICE=EVG0),$(EVG_SLOT=2),$(EVG_MEMOFFSET=0x000000000),$(EVG_IRQLINE=0x2),$(EVG_IRQVECT=0x1))
dbLoadTemplate("$(EVG_SUBS=cfg/$(DEVICE=EVG0).subs)","SYS=$(SYS),DEVICE=$(DEVICE=EVG0)")
