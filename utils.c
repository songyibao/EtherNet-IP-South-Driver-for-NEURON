//
// Created by root on 5/5/24.
//
#include "utils.h"
#include "ethernet_ip.h"
#include "libplctag.h"
#include "tag_hash/tag_hash.h"
int check_connection_status(void *data)
{
    int                code   = 0;
    struct neu_plugin *plugin = (struct neu_plugin *) data;
    tag_hash_t        *item   = plugin_get_first_tag(plugin);
    if (item) {
        int res = plc_tag_status(item->value);
        if (res == PLCTAG_STATUS_OK) {
            plugin->common.link_state = NEU_NODE_LINK_STATE_CONNECTED;
        } else {
            plugin->common.link_state = NEU_NODE_LINK_STATE_DISCONNECTED;
            plog_debug(plugin, "Connection status: %s\n", plc_tag_decode_error(res));
            code = -1;
        }
    } else {
        plugin->common.link_state = NEU_NODE_LINK_STATE_DISCONNECTED;
        plog_error(plugin, "No tags to check connection status\n");
        code = -1;
    }
    return code;
}
int check_all_connection_status(void *data)
{
    int                code   = 0;
    struct neu_plugin *plugin = (struct neu_plugin *) data;
    tag_hash_t        *item   = NULL, *tmp;
    int                count  = 0;
    // 如果 tag_hash_table_head 为空，不会开始迭代， 所以内部不需要判空
    HASH_ITER(hh, plugin->tag_hash_table_head, item, tmp)
    {
        count++;
        int res = plc_tag_status(item->value);

        if (res == PLCTAG_STATUS_OK) {
            plugin->common.link_state = NEU_NODE_LINK_STATE_CONNECTED;

        } else {
            plog_debug(plugin, "Connection %s status: %s\n", item->name, plc_tag_decode_error(res));
            plugin->common.link_state = NEU_NODE_LINK_STATE_DISCONNECTED;
        }
    }
    if (count == 0) {
        plugin->common.link_state = NEU_NODE_LINK_STATE_DISCONNECTED;
        plog_error(plugin, "No tags to check connection status\n");
        code = -1;
    }
    return code;
}
void add_connection_status_checker(neu_plugin_t *plugin)
{
    neu_event_timer_param_t param = { .second      = 1,
                                      .millisecond = 0,
                                      .cb          = check_all_connection_status,
                                      .usr_data    = (void *) plugin,
                                      .type        = NEU_EVENT_TIMER_NOBLOCK };

    plugin->timer = neu_event_add_timer(plugin->events, param);
}
void del_connection_status_checker(neu_plugin_t *plugin)
{
    if (plugin->timer) {

        neu_event_del_timer(plugin->events, plugin->timer);
        plugin->timer = NULL;
    }
}
int32_t create_and_add_plc_tag(neu_plugin_t *plugin, neu_datatag_t *tag)
{
    char *tmp_tag_path = calloc(1, sizeof(char) * 1024);
    strcpy(tmp_tag_path, plugin->tag_path);
    strcat(tmp_tag_path, "&name=");
    strcat(tmp_tag_path, tag->address);
    //    plog_debug(plugin, "tag path: %s", tmp_tag_path);
    int32_t plc_tag = plc_tag_create(tmp_tag_path, plugin->timeout);
    free(tmp_tag_path);
    /* everything OK? */
    if (plc_tag < 0) {
    } else {
        plugin_add_tag(plugin, tag->address, plc_tag);
    }
    return plc_tag;
}