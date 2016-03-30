# mrfioc2 EVR GUI
## Quick start
run 

    ./startEVR.sh -s <system name>
from this folder.

Script options:

 - -s <system name\> ..... The system/project name
 - -d <EVR name\>    ..... Event Receiver / timing card name (default: EVR0)
 - -f <form factor\> ..... Choose the event receiver form factor (default: VME-300)
 
	                     Choices: VME, PCIe, VME-300, PCIe-300DC

 - -h ..... This help


## EVR health monitoring
The __evr-health.template__ template allows for EVR health monitoring. It is recommended to use this template with each EVR setup. A GUI that displays the health information can be run by issuing the following command:

    ./start_EVR-health.sh -s <system name>
from this folder.

Script options:

 - -s <system name>     The system/project name
 - -d <EVR name>        Event Receiver /timing card name (default: EVR0)
 - -h                   This help


The database can be included in the following way (startup script commands, change macros to correct values): 

		dbLoadRecords "evr-health.template" "SYS=CSL-IFC1,DEVICE=EVR0"

