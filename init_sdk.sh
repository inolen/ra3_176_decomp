#!/usr/bin/env bash

set -ex

SDKVER=1.17
SDKZIP=$PWD/assets/quake3-$SDKVER-source.zip
SDKDIR=$PWD/ra3-sdk

if ! [ -f $SDKDIR/README.txt ]; then
  mkdir -p $SDKDIR

  if ! [ -d $SDKDIR/quake3-$SDKVER ]; then
    unzip -d $SDKDIR $SDKZIP
  fi

  mv $SDKDIR/quake3-$SDKVER/* $SDKDIR

  if [ -d $SDKDIR/quake3-$SDKVER ]; then
    rmdir $SDKDIR/quake3-$SDKVER
  fi
fi
