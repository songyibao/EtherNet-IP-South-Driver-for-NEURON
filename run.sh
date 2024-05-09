#!/bin/sh
rm -rf ./build
# shellcheck disable=SC2164
mkdir build && cd build && cmake .. && make
echo "build done,coping plugin ..."
cp -r ./libplugin-EtherNet-IP.so /home/songyibao/Downloads/neuron-main/build/plugins/
cp -r ./libplugin-EtherNet-IP.so /opt/neuron/plugins/
cp -r ../EtherNet-IP.json /opt/neuron/plugins/schema/
echo "copy done"