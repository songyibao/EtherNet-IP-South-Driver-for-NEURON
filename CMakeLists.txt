#
# Copyright 2018 Capitar IT Group BV <info@capitar.com>
# Copyright 2018 Staysail Systems, Inc. <info@staysail.tech>
#
# This software is supplied under the terms of the MIT License, a
# copy of which should be located in the distribution where this
# file was obtained (LICENSE.txt).  A copy of the license may also be
# found online at https://opensource.org/licenses/MIT.

cmake_minimum_required(VERSION 3.13)
set(LIBRARY_OUTPUT_PATH "./")
file(COPY /home/songyibao/Downloads/neuron-main/plugins/EtherNet-IP/EtherNet-IP.json DESTINATION
        /home/songyibao/Downloads/neuron-main/build/plugins/schema)
project(EtherNet-IP)


add_library(plugin-EtherNet-IP SHARED ethernet_ip.c
        utils.c
        tag_hash/tag_hash.c
        tag_handle/tag_handle.c)
target_include_directories(plugin-EtherNet-IP PRIVATE
        /home/songyibao/Downloads/neuron-main/include/neuron
)
target_link_libraries(plugin-EtherNet-IP PRIVATE /usr/local/lib/neuron/libneuron-base.so plctag)

