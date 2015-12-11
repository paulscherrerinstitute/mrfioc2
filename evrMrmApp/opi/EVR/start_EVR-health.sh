#!/bin/bash
set -o errexit

SYS=""
DEVICE="EVR0"

usage()
{
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "    -s <system name>     The system/project name"
    echo "    -d <EVR name>        Event Receiver / timing card name (default: $EVR)"
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

macro="SYS=$SYS,DEVICE=$DEVICE"
caqtdm -attach -macro "$macro" G_EVR-health.ui &
#echo caqtdm -attach -macro "$macro" G_EVR-health.ui
