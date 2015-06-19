#!/bin/bash

## Test the Pulser gateing functionality:
## Pulser 0 and 1 are both driving FPUniv1 (ORed)
## Pulsers 0 and 1 are both gated by gate 0, sending event 7 will enable pulser 0 and disable pulser 1, sending event 8 will enabale pulser 1 and disable pulser 0


SYS="MTEST-VME-CSL2-EVR0"

#Configure output

caput $SYS:FrontUnivOut1-Src-SP 0	 #Set to pulser 0
caput MTEST-VME-CSL2-EVR0:FrontUnivOut1-Src2-SP 1 #Set to Force low

############################################################
##Configure pulser 0, trigger on event 1
caput MTEST-VME-CSL2-EVR0:Pul0-Evt-Trig0-SP 1
caput MTEST-VME-CSL2-EVR0:Pul0-Evt-Reset0-SP 0
caput MTEST-VME-CSL2-EVR0:Pul0-Evt-Set0-SP 0

#Pulser0 0us delay, 100us pulse
caput MTEST-VME-CSL2-EVR0:Pul0-Ena-Sel 1
caput MTEST-VME-CSL2-EVR0:Pul0-Polarity-Sel "Active High"
caput MTEST-VME-CSL2-EVR0:Pul0-Delay-SP 0


caput MTEST-VME-CSL2-EVR0:Pul0-Prescaler-SP 1 
caput MTEST-VME-CSL2-EVR0:Pul0-Width-SP 100

#Disable gateing 
caput MTEST-VME-CSL2-EVR0:Pul0-Gate-Enable-SP 0
caput MTEST-VME-CSL2-EVR0:Pul0-Gate-Mask-SP 16


############################################################
##Configure pulser 0, trigger on event 1
caput MTEST-VME-CSL2-EVR0:Pul1-Evt-Trig0-SP 1
caput MTEST-VME-CSL2-EVR0:Pul1-Evt-Reset0-SP 0
caput MTEST-VME-CSL2-EVR0:Pul1-Evt-Set0-SP 0

#Pulser0 0us delay, 100us pulse
caput MTEST-VME-CSL2-EVR0:Pul1-Ena-Sel 1
caput MTEST-VME-CSL2-EVR0:Pul1-Polarity-Sel "Active High"
caput MTEST-VME-CSL2-EVR0:Pul1-Delay-SP 500


caput MTEST-VME-CSL2-EVR0:Pul1-Prescaler-SP 1 
caput MTEST-VME-CSL2-EVR0:Pul1-Width-SP 300

#Disable gateing 
caput MTEST-VME-CSL2-EVR0:Pul1-Gate-Enable-SP 16
caput MTEST-VME-CSL2-EVR0:Pul1-Gate-Mask-SP 0


############################################################
##Configure pulser 28 (gate0)
caput MTEST-VME-CSL2-EVR0:Pul28-Evt-Trig0-SP 0
caput MTEST-VME-CSL2-EVR0:Pul28-Evt-Reset0-SP 8
caput MTEST-VME-CSL2-EVR0:Pul28-Evt-Set0-SP 7

#Pulser0 500us delay, 100us pulse
caput MTEST-VME-CSL2-EVR0:Pul28-Ena-Sel 1
caput MTEST-VME-CSL2-EVR0:Pul28-Polarity-Sel "Active High"
caput MTEST-VME-CSL2-EVR0:Pul28-Delay-SP 1000


caput MTEST-VME-CSL2-EVR0:Pul28-Prescaler-SP 1 
caput MTEST-VME-CSL2-EVR0:Pul28-Width-SP 200

#Disable gateing 
caput MTEST-VME-CSL2-EVR0:Pul28-Gate-Enable-SP 0
caput MTEST-VME-CSL2-EVR0:Pul28-Gate-Mask-SP 0

