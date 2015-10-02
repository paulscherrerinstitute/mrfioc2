require mrfioc2

epicsEnvSet SYS "MTEST-VME-TIMINGTEST"


##########################
#-----! EVG Setup ------!#
##########################
## Macros:
## EVG is the event generator card name. (default: EVG0)
## EVG_SLOT is the VME crate slot where the card is inserted. (default: 2)
## EVG_MEMOFFSET is the base A24 address (default: 0x0)
## EVG_IRQLINE is the interrupt level. (default: 0x1)
## EVG_IRQVECT is the interrupt vector (default: 0x1)
## EVG_SUBS is the path to the substitution file that should be loaded. (default: cfg/$(EVG).subs=cfg/EVG0.subs)

runScript $($(MODULE)_DIR)/mrfioc2_evg-VME.cmd, "EVG_SUBS=$($(MODULE)_TEMPLATES)/evg_VME-300.subs"



##########################
#-----! EVR Setup ------!#
##########################
## Macros:
## EVR is the event receiver card name. (default: EVR0)
## EVR_SLOT is the VME crate slot where EVR is inserted. (default: 3)
## EVR_MEMOFFSET is the base A24 address (default: 0x3000000)
## EVR_IRQLINE is the interrupt level. (default: 0x5)
## EVR_IRQVECT is the interrupt vector (default: 0x26)
## EVR_SUBS is the path to the substitution file that should be loaded. (default: cfg/$(EVR).subs=cfg/EVR0.subs)

runScript $($(MODULE)_DIR)/mrfioc2_evr-VME.cmd, "EVR_SUBS=$($(MODULE)_TEMPLATES)/evr_VME-300.subs"

