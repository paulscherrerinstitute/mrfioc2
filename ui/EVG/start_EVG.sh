#!/bin/bash
set -o errexit

SYS=""
EVG="EVG0"
FF="VME"

usage()
{
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "    -s <system name>     The system/project name"
    echo "    -e <EVG name>        Event Generator name (default: $EVG)"
    echo "    -f <form factor>     EVR form factor (default: $FF)"
    echo "                         Choices: VME, VME-300"
    echo "    -h                   This help"
}

while getopts ":s:e:f:h" o; do
    case "${o}" in
        s)
            SYS=${OPTARG}
            ;;
        e)
            EVG=${OPTARG}
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

if [ $FF != "VME" ] && [ $FF != "VME-300" ]; then
    echo "Invalid form factor selected: $FF"
    echo "        Available choices: VME, VME-300"
    exit 1
fi

macro="EVG=$SYS-$EVG,FF=$FF"
caqtdm -attach -macro "$macro" G_EVG_main.ui &
#echo caqtdm -attach -macro "$macro" G_EVG_main.ui &
