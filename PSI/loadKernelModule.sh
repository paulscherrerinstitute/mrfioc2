#!/bin/bash

KERNEL=$(uname -r)

if [ -z "$KERNEL" ]; then
	echo "Could not get kernel name. Aborting."
	exit 1
fi

echo "Loading kernel modules for kernel: $KERNEL"
echo "Loading kernel modules for target architecture: $2"



UIO_LOADED=$(lsmod | grep uio)
if [ -z "$UIO_LOADED" ]; then
	echo "Loading uio module."
	modprobe uio
else
	echo "Uio module already loaded."
fi

UIO_LOADED=$(lsmod | grep uio)
if [ -z "$UIO_LOADED" ]; then
	echo "Loading uio module."
	insmod $1/kernelModules/$KERNEL/uio.ko
fi

UIO_LOADED=$(lsmod | grep uio)
if [ -z "$UIO_LOADED" ]; then
	echo "Uio module not loaded. Aborting."
	exit 1
fi


if [ "$2" = "eldk42-ppc4xxFP" ]; then
  echo "Loading of parport module not needed on this architecture"
else
  PARPORT_LOADED=$(lsmod | grep parport)
  if [ -z "$PARPORT_LOADED" ]; then
    echo "Loading parport module."
    modprobe parport
  else
    echo "parport module already loaded."
  fi

  PARPORT_LOADED=$(lsmod | grep parport)
  if [ -z "$PARPORT_LOADED" ]; then
    echo "Loading parport module."
    insmod $1/kernelModules/$KERNEL/parport.ko
  fi

  PARPORT_LOADED=$(lsmod | grep parport)
  if [ -z "$PARPORT_LOADED" ]; then
    echo "parport module not loaded. Aborting."
    exit 1
  fi
fi


MRF_LOADED=$(lsmod | grep mrf)
if [ -z "$MRF_LOADED" ]; then
	echo "Loading mrf module."
	insmod $1/kernelModules/$KERNEL/mrf.ko
else
	echo "Mrf module already loaded."
fi

MRF_LOADED=$(lsmod | grep mrf)
if [ -z "$MRF_LOADED" ]; then
	echo "Mrf module failed to load."
fi
