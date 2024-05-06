//
// Created by SongYibao on 5/5/24.
//
/**
 * NEURON IIoT System for Industry 4.0
 * Copyright (C) 2020-2022 EMQ Technologies Co., Ltd All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **/
#include <stdlib.h>

#include "neuron.h"

#include "errcodes.h"

#include "ethernet_ip.h"
#include "libplctag.h"
#include "tag_handle/tag_handle.h"
#include "tag_hash/tag_hash.h"
#include "utils.h"
static neu_plugin_t *driver_open(void);

static int driver_close(neu_plugin_t *plugin);
static int driver_init(neu_plugin_t *plugin, bool load);
static int driver_uninit(neu_plugin_t *plugin);
static int driver_start(neu_plugin_t *plugin);
static int driver_stop(neu_plugin_t *plugin);
static int driver_config(neu_plugin_t *plugin, const char *config);
static int driver_request(neu_plugin_t *plugin, neu_reqresp_head_t *head, void *data);

static int driver_tag_validator(const neu_datatag_t *tag);
static int driver_validate_tag(neu_plugin_t *plugin, neu_datatag_t *tag);
static int driver_group_timer(neu_plugin_t *plugin, neu_plugin_group_t *group);
static int driver_write(neu_plugin_t *plugin, void *req, neu_datatag_t *tag, neu_value_u value);
static int driver_write_tags(neu_plugin_t *plugin, void *req, UT_array *tags);
static int driver_del_tags(neu_plugin_t *plugin, int n_tag);
static const neu_plugin_intf_funs_t plugin_intf_funs = {
    .open    = driver_open,
    .close   = driver_close,
    .init    = driver_init,
    .uninit  = driver_uninit,
    .start   = driver_start,
    .stop    = driver_stop,
    .setting = driver_config,
    .request = driver_request,

    .driver.validate_tag  = driver_validate_tag,
    .driver.group_timer   = driver_group_timer,
    .driver.write_tag     = driver_write,
    .driver.tag_validator = driver_tag_validator,
    .driver.write_tags    = driver_write_tags,
    .driver.add_tags      = NULL,
    .driver.load_tags     = NULL,
    .driver.del_tags      = driver_del_tags,
};

const neu_plugin_module_t neu_plugin_module = {
    .version         = NEURON_PLUGIN_VER_1_0,
    .schema          = "EtherNet-IP",
    .module_name     = "EtherNet/IP",
    .module_descr    = "This plugin is used to connect devices using the EtherNet/IP protocol",
    .module_descr_zh = "该插件用于连接使用 EtherNet/IP 协议的设备。",
    .intf_funs       = &plugin_intf_funs,
    .kind            = NEU_PLUGIN_KIND_SYSTEM,
    .type            = NEU_NA_TYPE_DRIVER,
    .display         = true,
    .single          = false,
};

static neu_plugin_t *driver_open(void)
{
    nlog_debug("\n==============================driver_open"
               "===========================\n");
    neu_plugin_t *plugin = calloc(1, sizeof(neu_plugin_t));

    neu_plugin_common_init(&plugin->common);

    return plugin;
}

static int driver_close(neu_plugin_t *plugin)
{
    plog_debug(plugin,
               "\n==============================driver_close"
               "===========================\n");
    free(plugin);

    return 0;
}

static int driver_init(neu_plugin_t *plugin, bool load)
{
    plog_debug(plugin,
               "\n==============================driver_init"
               "===========================\n");
    (void) load;
    plugin->events = neu_event_new();
    plugin->fd     = -1;
    // todo:在配置中解析 slot
    plugin->slot                = 0;
    plugin->tag_hash_table_head = NULL;
    add_connection_status_checker(plugin);
    plog_notice(plugin, "%s init success", plugin->common.name);
    return 0;
}

static int driver_uninit(neu_plugin_t *plugin)
{
    plog_debug(plugin,
               "\n==============================driver_uninit"
               "===========================\n");
    plog_notice(plugin, "%s uninit start", plugin->common.name);

    plugin->fd = -1;

    neu_event_close(plugin->events);
    plugin_free_all_tags(plugin);
    plog_notice(plugin, "%s uninit success", plugin->common.name);
    plugin_free_all_tags(plugin);
    return 0;
}

static int driver_start(neu_plugin_t *plugin)
{
    plog_debug(plugin,
               "\n==============================driver_start"
               "===========================\n");
    //    neu_conn_start(plugin->conn);
    return 0;
}

static int driver_stop(neu_plugin_t *plugin)
{
    plog_debug(plugin,
               "\n==============================driver_stop"
               "===========================\n");
    plog_notice(plugin, "%s stop success", plugin->common.name);
    return 0;
}

static int driver_config(neu_plugin_t *plugin, const char *config)
{
    plog_debug(plugin,
               "\n==============================driver_config"
               "===========================\n");
    int   ret       = 0;
    char *err_param = NULL;

    //    neu_json_elem_t slot = { .name = "slot", .t = NEU_JSON_INT };

    neu_json_elem_t port           = { .name = "port", .t = NEU_JSON_INT };
    neu_json_elem_t timeout        = { .name = "timeout", .t = NEU_JSON_INT };
    neu_json_elem_t host           = { .name = "host", .t = NEU_JSON_STR, .v.val_str = NULL };
    neu_json_elem_t interval       = { .name = "interval", .t = NEU_JSON_INT };
    neu_json_elem_t mode           = { .name = "connection_mode", .t = NEU_JSON_INT };
    neu_json_elem_t max_retries    = { .name = "max_retries", .t = NEU_JSON_INT };
    neu_json_elem_t retry_interval = { .name = "retry_interval", .t = NEU_JSON_INT };

    ret = neu_parse_param((char *) config, &err_param, 5, &port, &host, &mode, &timeout, &interval);

    if (ret != 0) {
        plog_error(plugin, "config: %s, decode error: %s", config, err_param);
        free(err_param);
        if (host.v.val_str != NULL) {
            free(host.v.val_str);
        }
        return -1;
    }

    if (timeout.v.val_int <= 0) {
        plog_error(plugin, "config: %s, set timeout error: %s", config, err_param);
        free(err_param);
        return -1;
    }

    ret = neu_parse_param((char *) config, &err_param, 2, &port, &max_retries, &retry_interval);
    if (ret != 0) {
        free(err_param);
        max_retries.v.val_int    = 0;
        retry_interval.v.val_int = 0;
    }

    plog_notice(plugin, "config: host: %s, port: %" PRId64 ", mode: %" PRId64 "", host.v.val_str, port.v.val_int,
                mode.v.val_int);

    plugin->port = port.v.val_int;
    strcpy(plugin->host, host.v.val_str);
    free(host.v.val_str);
    // timeout 单位是毫秒
    plugin->timeout = timeout.v.val_int;
    sprintf(plugin->tag_path, TAG_PATH, plugin->host);
    plog_debug(plugin, "tag path: %s", plugin->tag_path);
    return 0;
}

static int driver_request(neu_plugin_t *plugin, neu_reqresp_head_t *head, void *data)
{
    plog_debug(plugin,
               "\n==============================driver_request"
               "===========================\n");
    (void) plugin;
    (void) head;
    (void) data;
    return 0;
}
static int driver_tag_validator(const neu_datatag_t *tag)
{
    return 0;
}
static int driver_validate_tag(neu_plugin_t *plugin, neu_datatag_t *tag)
{
    plog_debug(plugin,
               "\n==============================driver_validate_tag"
               "===========================\n");
    // 打印tag的信息
    plog_debug(plugin,
               "tag: %s, address: %s, attribute: %d, type: %d, "
               "precision: %d, decimal: %f, description: %s",
               tag->name, tag->address, tag->attribute, tag->type, tag->precision, tag->decimal, tag->description);
    // 创建 plctag 库的 plc_tag 句柄
    int32_t plc_tag = create_and_add_plc_tag(plugin, tag);
    if (plc_tag != PLCTAG_STATUS_OK) {
        plog_debug(plugin, "ERROR %s: Could not create tag!\n", plc_tag_decode_error(plc_tag));
    }
    return 0;
}

static int driver_group_timer(neu_plugin_t *plugin, neu_plugin_group_t *group)
{
    plog_debug(plugin,
               "\n\n==============================driver_group_timer"
               "===========================\n");

    utarray_foreach(group->tags, neu_datatag_t *, tag)
    {
        handle_tag(plugin, tag, group);
    }
    return 0;
}
static int driver_write(neu_plugin_t *plugin, void *req, neu_datatag_t *tag, neu_value_u value)
{
    plog_debug(plugin,
               "\n==============================driver_write"
               "===========================\n");
    plog_debug(plugin, "write tag: %s, address: %s", tag->name, tag->address);
    return 0;
}

static int driver_write_tags(neu_plugin_t *plugin, void *req, UT_array *tags)
{
    plog_debug(plugin,
               "\n==============================driver_write_tags"
               "===========================\n");
    return 0;
}
static int driver_del_tags(neu_plugin_t *plugin, int n_tag)
{
    plog_debug(plugin,
               "\n==============================driver_del_tags"
               "===========================\n");
    plog_debug(plugin, "deleting %d tags", n_tag);
    return 0;
}