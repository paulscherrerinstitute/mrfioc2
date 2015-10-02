require mrfioc2

epicsEnvSet SYS "MTEST-PCI-TIMINGTEST"


##########################
#-----! EVR Setup ------!#
##########################
## Macros:
## EVR is the event receiver card name. (default: EVR0)
## EVR_DOMAIN is the PCI domain where the card is inserted. (default: 0x1)
## EVR_BUS is the PCI bus where the card is inserted. (default: 0x81)
## EVR_DEVICE is the PCI device where the card is inserted. (default: 0)
## EVR_FUNCTION is the PCI function where the card is inserted. (default: 0)


runScript $($(MODULE)_DIR)/mrfioc2_evr-PCIe.cmd, "EVR_SUBS=$($(MODULE)_TEMPLATES)/evr_PCIe-300.subs"

