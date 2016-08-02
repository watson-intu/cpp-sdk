#!/bin/bash
#Usage: stage_mac.sh [profile]

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
if [ "$BUILD_CONFIG" == "" ]; then 
	BUILD_CONFIG="build-mac"
fi

BUILD_DIR=$DIR/../$BUILD_CONFIG
BIN_DIR=$BUILD_DIR/sdk/bin
LIB_DIR=$BUILD_DIR/sdk/lib
ETC_DIR=$DIR/../etc
STAGE_DIR=$DIR/../stage

echo Staging files...
if [ -d $STAGE_DIR ]; then
	rm -rf $STAGE_DIR
fi
mkdir -p $STAGE_DIR

# copy all files into the stage directory
cp -R -L $BIN_DIR/* $STAGE_DIR/
cp -R -L $LIB_DIR/* $STAGE_DIR/
cp -R $ETC_DIR $STAGE_DIR/

if [ "$1" != "" ]; then
        $DIR/install_profile.sh $1 
fi

