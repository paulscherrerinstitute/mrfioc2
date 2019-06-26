#!/bin/bash
set -o errexit

HIPA_UI_LOCATION=/hipa/DisplaysQt
HIPA_SCRIPT_LOCATION=$CONTROLBIN
DRY_RUN=0

# print usage string
usage()
{
    echo "Usage: $0"
  	echo
    echo "This script installes the EVG and EVR user interface (ui) files"
    echo "and start_* scripts to start the uis in HIPA."
    echo
    echo "The script assumes that pwd=mrfioc root dir."
    echo
    echo "Options:"
    echo "    -h                   This help"
    echo "    -d                   dry run (say what you would do but do not install)"
}

# handle cmd line options
while getopts ":dh" o; do
    case "${o}" in
        h)
            usage
            exit 0
            ;;
        d)
            DRY_RUN=1
            ;;
        *)
            usage
            exit 1
            ;;
    esac
done

# check that pwd = mrfioc2 root dir
if [ ! -d "PSI" ]; then
  echo 
  echo "ERROR: pwd != mrfioc2 root dir"
  echo
  usage;
  exit 1;
fi

# install EVG uis
echo
echo "## installing uis for EVG -------------------------------"
if [ $DRY_RUN -eq 1 ]; then
  for ui in evgMrmApp/opi/EVG/*.ui; do echo "installing $ui to $HIPA_UI_LOCATION"; done
else
  for ui in evgMrmApp/opi/EVG/*.ui; do upy -f --cp $ui $HIPA_UI_LOCATION; done
fi
echo "## installation of uis for EVG done"

# install start_* scripts for EVG
echo
echo "## installing start scripts for EVG -------------------------------"
if [ $DRY_RUN -eq 1 ]; then
  for script in evgMrmApp/opi/EVG/*.sh; do echo "installing $script to $HIPA_SCRIPT_LOCATION"; done
else
  for script in evgMrmApp/opi/EVG/*.sh; do upy -f --cp $script $HIPA_SCRIPT_LOCATION; done
fi
echo "## installation of start scripts for EVG done"

# install EVR uis
echo
echo "## installing uis for EVR -------------------------------"
if [ $DRY_RUN -eq 1 ]; then
  for ui in evrMrmApp/opi/EVR/*.ui; do echo "installing $ui to $HIPA_UI_LOCATION"; done
else
  for ui in evrMrmApp/opi/EVR/*.ui; do upy -f --cp $ui $HIPA_UI_LOCATION; done
fi
echo "## installation of uis for EVR done"

# install start_* scripts for EVR
echo
echo "## installing start scripts for EVR -------------------------------"
if [ $DRY_RUN -eq 1 ]; then
  for script in evrMrmApp/opi/EVR/*.sh; do echo "installing $script to $HIPA_SCRIPT_LOCATION"; done
  for script in evrMrmApp/opi/EVR/*.bat; do echo "installing $script to $HIPA_SCRIPT_LOCATION"; done
else
  for script in evrMrmApp/opi/EVR/*.sh; do upy -f --cp $script $HIPA_SCRIPT_LOCATION; done
  for script in evrMrmApp/opi/EVR/*.bat; do upy -f --cp $script $HIPA_SCRIPT_LOCATION; done
fi
echo "## installation of start scripts for EVR done"

