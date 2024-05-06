//
// Created by root on 5/6/24.
//
#include "tag_handle.h"
#include "../tag_hash/tag_hash.h"
#include "libplctag.h"
int read_tag(neu_plugin_t *plugin, neu_plugin_group_t *group, neu_datatag_t *tag, uint8_t size, neu_dvalue_t *dvalue)
{
    dvalue->type              = tag->type;
    int         res           = 0;
    int         plc_tag       = -1;
    tag_hash_t *tag_hash_item = plugin_find_tag(plugin, tag->address);
    if (tag_hash_item) {
        plc_tag = tag_hash_item->value;
        /* get the data */
        res = plc_tag_read(plc_tag, plugin->timeout);
        if (res != PLCTAG_STATUS_OK) {
            plog_error(plugin,
                       "ERROR: Unable to read the data! Got error code %d: %s, deleting tag %s from "
                       "hashtable\n",
                       res, plc_tag_decode_error(res), tag->address);
            dvalue->type      = NEU_TYPE_ERROR;
            dvalue->value.i32 = NEU_ERR_PLUGIN_TAG_NOT_READY;
            plugin_del_tag(plugin, tag->address);
            plc_tag_destroy(plc_tag);
            res = -1;
            goto exit;
        }
        /* get the tag size and element size. Do this _AFTER_ reading the tag otherwise we may not know how big the
         * tag is! */
        int elem_size  = plc_tag_get_int_attribute(plc_tag, "elem_size", 0);
        int elem_count = plc_tag_get_int_attribute(plc_tag, "elem_count", 0);
        //        plog_debug(plugin, "Tag has %d elements each of %d bytes.\n", elem_count, elem_size);
        if (tag->type != NEU_TYPE_STRING) {
            if ((elem_count != 1 || elem_size != size)) {
                plog_error(plugin, "type: not match,elem_count: %d, elem_size: %d\n", elem_count, elem_size);
                dvalue->type      = NEU_TYPE_ERROR;
                dvalue->value.i32 = NEU_ERR_PLUGIN_TAG_TYPE_MISMATCH;
                res               = -1;
                goto exit;
            }
        } else {
            if ((elem_count != 1)) {
                plog_error(plugin, "type: not match,elem_count: %d, elem_size: %d\n", elem_count, elem_size);
                dvalue->type      = NEU_TYPE_ERROR;
                dvalue->value.i32 = NEU_ERR_PLUGIN_TAG_TYPE_MISMATCH;
                res               = -1;
                goto exit;
            }
        }
        // 外层循环中已赋值
        //            dvalue->type      = NEU_TYPE_ERROR;
        switch (tag->type) {
        case NEU_TYPE_INT8:
            dvalue->value.i8 = plc_tag_get_int8(plc_tag, 0);
            break;
        case NEU_TYPE_UINT8:
            dvalue->value.u8 = plc_tag_get_uint8(plc_tag, 0);
            break;
        case NEU_TYPE_INT16:
            dvalue->value.i16 = plc_tag_get_int16(plc_tag, 0);
            break;
        case NEU_TYPE_UINT16:
            dvalue->value.u16 = plc_tag_get_uint16(plc_tag, 0);
            break;
        case NEU_TYPE_INT32:
            dvalue->value.i32 = plc_tag_get_int32(plc_tag, 0);
            break;
        case NEU_TYPE_UINT32:
            dvalue->value.u32 = plc_tag_get_uint32(plc_tag, 0);
            break;
        case NEU_TYPE_INT64:
            dvalue->value.i64 = plc_tag_get_int64(plc_tag, 0);
            break;
        case NEU_TYPE_UINT64:
            dvalue->value.u64 = plc_tag_get_uint64(plc_tag, 0);
            break;
        case NEU_TYPE_FLOAT:
            dvalue->value.f32 = plc_tag_get_float32(plc_tag, 0);
            break;
        case NEU_TYPE_DOUBLE:
            dvalue->value.d64 = plc_tag_get_float64(plc_tag, 0);
            break;
        case NEU_TYPE_BIT:
            dvalue->value.u8 = plc_tag_get_bit(plc_tag, 0);
            break;
        case NEU_TYPE_BOOL:
            dvalue->value.boolean = plc_tag_get_uint8(plc_tag, 0); // Assuming BOOL is a single byte
            break;
        case NEU_TYPE_STRING:
            plc_tag_get_string(plc_tag, 0, dvalue->value.str, NEU_VALUE_SIZE);
            break;
        case NEU_TYPE_WORD:
            dvalue->value.u16 = plc_tag_get_uint16(plc_tag, 0);
            break;
        case NEU_TYPE_DWORD:
            dvalue->value.u32 = plc_tag_get_uint32(plc_tag, 0);
            break;
        case NEU_TYPE_LWORD:
            dvalue->value.u64 = plc_tag_get_uint64(plc_tag, 0);
            break;
        default:
            break;
        }
    } else {
        plog_error(plugin, "tag_hash not found,tag->address: %s", tag->address);
        dvalue->type      = NEU_TYPE_ERROR;
        dvalue->value.i32 = NEU_ERR_PLUGIN_TAG_NOT_READY;
        // 尝试重新创建 plctag 库的 plc_tag 句柄
        plog_debug(plugin, "Trying to create and add tag %s\n", tag->address);
        plc_tag = create_and_add_plc_tag(plugin, tag);
        if (plc_tag != PLCTAG_STATUS_OK) {
            plog_debug(plugin, "ERROR %s: Could not create tag!\n", plc_tag_decode_error(plc_tag));
        }
        res = -1;
        goto exit;
    }
    return res;
exit:
    return res;
}
void handle_tag(neu_plugin_t *plugin, neu_datatag_t *tag, neu_plugin_group_t *group)
{
    //    plog_debug(plugin,
    //               "tag: %s, address: %s, attribute: %d, type: %d, "
    //               "precision: %d, decimal: %f, description: %s",
    //               tag->name, tag->address, tag->attribute, tag->type, tag->precision, tag->decimal,
    //               tag->description);
    neu_dvalue_t dvalue = { 0 };
    dvalue.type         = tag->type;
    switch (tag->type) {
    case NEU_TYPE_INT8:
    case NEU_TYPE_UINT8:
    case NEU_TYPE_BIT:
    case NEU_TYPE_BOOL:
        read_tag(plugin, group, tag, 1, &dvalue);
        break;
    case NEU_TYPE_INT16:
    case NEU_TYPE_UINT16:
    case NEU_TYPE_WORD:
        read_tag(plugin, group, tag, 2, &dvalue);
        break;
    case NEU_TYPE_INT32:
    case NEU_TYPE_UINT32:
    case NEU_TYPE_FLOAT:
    case NEU_TYPE_DWORD:
        read_tag(plugin, group, tag, 4, &dvalue);
        break;
    case NEU_TYPE_INT64:
    case NEU_TYPE_UINT64:
    case NEU_TYPE_DOUBLE:
    case NEU_TYPE_LWORD:
        read_tag(plugin, group, tag, 8, &dvalue);
        break;
    case NEU_TYPE_STRING:
        read_tag(plugin, group, tag, 0, &dvalue);
        break;
    default:
        break;
    }
    plugin->common.adapter_callbacks->driver.update(plugin->common.adapter, group->group_name, tag->name, dvalue);
}