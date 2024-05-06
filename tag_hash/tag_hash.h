//
// Created by root on 5/6/24.
//

#ifndef NEURON_TAG_HASH_H
#define NEURON_TAG_HASH_H
#include "../ethernet_ip.h"
tag_hash_t *plugin_find_tag(neu_plugin_t *plugin, char *key);
void        plugin_add_tag(neu_plugin_t *plugin, char *key, int32_t value);
void        plugin_del_tag(neu_plugin_t *plugin, char *key);
void        plugin_free_all_tags(neu_plugin_t *plugin);
tag_hash_t *plugin_get_first_tag(neu_plugin_t *plugin);
#endif // NEURON_TAG_HASH_H
