# SATS
caqtdm -attach -macro "EVR=FTEST-MRFSATS-EVR0,FF=VME-300" G_EVR_master.ui &
sleep 2
# SATM
caqtdm -attach -macro "EVG=FTEST-MRFSATM-EVG0" G_EVG_VME_master.ui &
sleep 2
caqtdm -attach -macro "EVG=FTEST-MRFSATM-EVG1" G_EVG_VME_master.ui &
sleep 2 
caqtdm -attach -macro "EVG=FTEST-MRFSATM-EVG2" G_EVG_VME_master.ui &
sleep 2

#SATD
caqtdm -attach -macro "EVG=FTEST-MRFSATD-EVG0" G_EVG_VME_master.ui &
sleep 2
caqtdm -attach -macro "EVG=FTEST-MRFSATD-EVG1" G_EVG_VME_master.ui &
