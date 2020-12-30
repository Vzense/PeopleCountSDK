#!/bin/sh

ORIG_PATH=`pwd`
cd `dirname $0`
SCRIPT_PATH=`pwd`
cd $ORIG_PATH

if [ "`uname -s`" != "Darwin" ]; then
    # Install UDEV rules for USB device
    sudo cp ${SCRIPT_PATH}/0660-vzense-usb.rules /etc/udev/rules.d/0660-vzense-usb.rules
    sudo udevadm control --reload
    sudo cp -d ${SCRIPT_PATH}/3rdparty/opencv-4.5.1/lib/* /usr/lib/x86_64-linux-gnu/
    mkdir -p ~/.config/Vzense/
    cp -r ${SCRIPT_PATH}/Config ~/.config/Vzense/
fi
