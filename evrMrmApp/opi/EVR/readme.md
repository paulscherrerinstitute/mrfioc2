# mrfioc2 EVR GUI
## Quick start
run 

    ./startEVR.sh -s <system name>
from this folder.

Script options:

 - -s <system name>     The system/project name
 - -e <EVR name>        Event Receiver name (default: EVR0)
 - -h                   This help


## EVR health monitoring
The __evr-health.template__ template allows for EVR health monitoring. It is recommended to use this template with each EVR setup. A GUI that displays the health information can be run by issuing the following command:

    ./start_EVR-health.sh -s <system name>
from this folder.

Script options:

 - -s <system name>     The system/project name
 - -e <EVR name>        Event Receiver name (default: EVR0)
 - -h                   This help


The database can be included in the following way (startup script commands, change macros to correct values): 

		dbLoadRecords "evr-health.template" "SYS=CSL-IFC1,EVR=EVR0"

