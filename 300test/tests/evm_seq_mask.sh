

SYS="MTEST-VME-CSL2-EVR0"

caput MTEST-VME-CSL2-EVG0:Mxc0-Frequency-SP 10

caput MTEST-VME-CSL2-EVG0:FrontInp1-SeqMask-SP 1 #Mask 1st bit

caput MTEST-VME-CSL2-EVG0:SoftSeq-2-TsInpMode-Sel EGU
caput MTEST-VME-CSL2-EVG0:SoftSeq-2-TsResolution-Sel uSec
caput MTEST-VME-CSL2-EVG0:SoftSeq-2-RunMode-Sel Normal
caput MTEST-VME-CSL2-EVG0:SoftSeq-2-TrigSrc-Sel Mxc0

caput -a MTEST-VME-CSL2-EVG0:SoftSeq-2-EvtCode-SP 4 1 2 4 7 
caput -a MTEST-VME-CSL2-EVG0:SoftSeq-2-EvtMask-SP 4 0 2 4 0
caput -a MTEST-VME-CSL2-EVG0:SoftSeq-2-Timestamp-SP 4 1000 2000 3000 4000


caput MTEST-VME-CSL2-EVG0:SoftSeq-2-Commit-Cmd 1
caput MTEST-VME-CSL2-EVG0:SoftSeq-2-Load-Cmd 1
caput MTEST-VME-CSL2-EVG0:SoftSeq-2-Enable-Cmd 1