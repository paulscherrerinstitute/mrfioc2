# SATS
caqtdm -attach -macro "EVR=FTEST-VME-SATS-EVR0,FF=VME-300" G_EVR_master.ui &
sleep 2
# SATM
caqtdm -attach -macro "EVG=FTEST-VME-SATM-EVG0" G_EVG_VME_master.ui &
sleep 2
caqtdm -attach -macro "EVG=FTEST-VME-SATM-EVG1" G_EVG_VME_master.ui &
sleep 2 
caqtdm -attach -macro "EVG=FTEST-VME-SATM-EVG2" G_EVG_VME_master.ui &
sleep 2

#SATD
caqtdm -attach -macro "EVG=FTEST-VME-SATD-EVG0" G_EVG_VME_master.ui &
sleep 2
caqtdm -attach -macro "EVG=FTEST-VME-SATD-EVG1" G_EVG_VME_master.ui &
