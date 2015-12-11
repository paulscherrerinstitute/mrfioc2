#!/bin/bash
set -o errexit

SYS=""
DEVICE="EVR0"
FF="VME"

usage()
{
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "    -s <system name>     The system/project name"
    echo "    -d <EVR name>        Event Receiver / timing card name (default: $EVR)"
    echo "    -f <form factor>     EVR form factor (default: $FF)"
    echo "                         Choices: VME, PCIe, VME-300"
    echo "    -h                   This help"
}

while getopts ":s:d:f:h" o; do
    case "${o}" in
        s)
            SYS=${OPTARG}
            ;;
        d)
            DEVICE=${OPTARG}
            ;;
        f)
            FF=${OPTARG}
            ;;
        h)
            usage
            exit 0
            ;;
        *)
            usage
            exit 1
            ;;
    esac
done

if [ $OPTIND -le 1 ]; then
    usage
    exit 1
fi

if [ -z $SYS ]; then
    usage
    exit 1
fi

if [ $FF != "VME" ] && [ $FF != "PCIe" ] && [ $FF != "VME-300" ]; then
    echo "Invalid form factor selected: $FF"
    echo "        Available choices: VME, VME-300, PCIe"
    exit 1
fi

macro="SYS=$SYS,DEVICE=$DEVICE,FF=$FF"
caqtdm -attach -macro "$macro" G_EVR_main.ui &
#echo caqtdm -attach -macro "$macro" G_EVR_main.ui &
