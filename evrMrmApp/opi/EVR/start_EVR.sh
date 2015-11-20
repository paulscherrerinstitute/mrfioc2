#!/bin/bash
set -o errexit

SYS=""
EVR="EVR0"
FF="VME"

usage()
{
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "    -s <system name>     The system/project name"
    echo "    -e <EVR name>        Event Receiver name (default: $EVR)"
    echo "    -f <form factor>     EVR form factor (default: $FF)"
    echo "                         Choices: VME, PCIe, VME-300"
    echo "    -h                   This help"
}

while getopts ":s:e:f:h" o; do
    case "${o}" in
        s)
            SYS=${OPTARG}
            ;;
        e)
            EVR=${OPTARG}
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

macro="EVR=$SYS-$EVR,FF=$FF"
caqtdm -attach -macro "$macro" G_EVR_main.ui &
#echo caqtdm -attach -macro "$macro" G_EVR_main.ui &