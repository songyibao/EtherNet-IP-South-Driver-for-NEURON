//
// Created by root on 5/5/24.
//

#ifndef NEURON_UTILS_H
#define NEURON_UTILS_H
#include "ethernet_ip.h"
void conn_connected(void *data, int fd);

void    conn_disconnected(void *data, int fd);
void    add_connection_status_checker(neu_plugin_t *plugin);
void    del_connection_status_checker(neu_plugin_t *plugin);
int32_t create_and_add_plc_tag(neu_plugin_t *plugin, neu_datatag_t *tag);
#endif // NEURON_UTILS_H
