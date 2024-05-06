//
// Created by root on 5/5/24.
//

#ifndef NEURON_ETHERNET_IP_H
#define NEURON_ETHERNET_IP_H
#include "neuron.h"
#include "utils.h"
typedef struct tag_hash {
    char           name[50]; /* key (string is WITHIN the structure) */
    int32_t        value;
    UT_hash_handle hh; /* makes this structure hashable */
} tag_hash_t;
struct neu_plugin {
    neu_plugin_common_t common;

    int         fd;
    int         slot;
    char        host[16];
    uint64_t    port;
    char        tag_path[1024];
    uint16_t    timeout;
    tag_hash_t *tag_hash_table_head;

    neu_event_timer_t *timer;
    neu_events_t      *events;
};

#define TAG_PATH "protocol=ab-eip&gateway=%s&path=1,0&cpu=controllogix"
#endif // NEURON_ETHERNET_IP_H
