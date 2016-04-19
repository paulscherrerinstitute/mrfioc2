#!/bin/bash
set -o errexit

SYS=""
DEVICE="EVG0"
FF="VME-300"
ATTACH="-attach"

usage()
{
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "    -s <system name>     The system/project name"
    echo "    -d <EVG name>        Event Generator / timing card name (default: $EVG)"
    echo "    -f <form factor>     EVR form factor (default: $FF)"
    echo "                         Choices: VME, VME-300"
    echo "    -n                   Do not attach to existing caQtDM. Open new one instead"
    echo "    -h                   This help"
}

while getopts ":s:d:f:nh" o; do
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
        n)
            ATTACH=""
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

if [ $FF != "VME" ] && [ $FF != "VME-300" ]; then
    echo "Invalid form factor selected: $FF"
    echo "        Available choices: VME, VME-300"
    exit 1
fi

macro="SYS=$SYS,DEVICE=$DEVICE,FF=$FF"
caqtdm -$ATTACH -macro "$macro" G_EVG_main.ui &
