#!/bin/bash
set -o errexit

SYS=""
EVR="EVR0"

usage()
{
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "    -s <system name>     The system/project name"
    echo "    -e <EVR name>        Event Receiver name (default: $EVR)"
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

macro="SYS=$SYS,EVR=$EVR"
caqtdm -attach -macro "$macro" G_EVR-health.ui &
#echo caqtdm -attach -macro "$macro" G_EVR-health.ui
