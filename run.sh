#!/bin/sh
rm -rf ./build
# shellcheck disable=SC2164
mkdir build && cd build && cmake .. && make
echo "build done,coping plugin ..."
cp -r ./libplugin-EtherNet-IP.so /home/songyibao/Downloads/neuron-main/build/plugins/
echo "copy done"