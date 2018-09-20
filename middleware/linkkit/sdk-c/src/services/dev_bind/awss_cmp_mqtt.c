/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */



#include <stdio.h>
#include "os.h"
#include "iot_import.h"
#include "iot_export.h"
#include "awss_cmp.h"
#include "awss_notify.h"
#include "awss_packet.h"
#include "awss_log.h"

#if defined(__cplusplus)  /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

static char online_init = 0;

int awss_cmp_mqtt_register_cb(char *topic, void *cb)
{
    if (topic == NULL) {
        return -1;
    }

    return IOT_MQTT_Subscribe(NULL, topic, 0, (iotx_mqtt_event_handle_func_fpt)cb, NULL);
}

int awss_cmp_mqtt_unregister_cb(char *topic)
{
    return IOT_MQTT_Unsubscribe(NULL, topic);
}

int awss_cmp_mqtt_send(char *topic, void *data, int len, int qos)
{
    return IOT_MQTT_Publish_Simple(NULL, topic, qos, data, len);  //IOTX_MQTT_QOS1 or IOTX_MQTT_QOS1
}

const struct awss_cmp_couple awss_online_couple[] = {
    {TOPIC_MATCH_REPORT_REPLY, awss_report_token_reply},
#ifdef WIFI_AWSS_ENABLED
#ifndef AWSS_DISABLE_REGISTRAR
    {TOPIC_ZC_CHECKIN,         awss_enrollee_checkin},
    {TOPIC_ZC_ENROLLEE_REPLY,  awss_report_enrollee_reply},
    {TOPIC_ZC_CIPHER_REPLY,    awss_get_cipher_reply},
#endif
    {TOPIC_SWITCHAP,           awss_online_switchap}
#endif
};

int awss_cmp_online_init()
{
    if (online_init) {
        return 0;
    }

    char topic[TOPIC_LEN_MAX] = {0};
    int i;
#ifdef AWSS_INIT_CMP
    {
        char pk[OS_PRODUCT_KEY_LEN + 1] = {0};
        char dn[OS_DEVICE_NAME_LEN + 1] = {0};
        char ds[OS_DEVICE_SECRET_LEN + 1] = {0};

        os_product_get_key(pk);
        os_device_get_name(dn);
        os_device_get_secret(ds);
        mqtt_init_instance(pk, dn, ds, 1024);
    }
#endif
    for (i = 0; i < sizeof(awss_online_couple) / sizeof(awss_online_couple[0]); i ++) {
        memset(topic, 0, sizeof(topic));
        awss_build_topic(awss_online_couple[i].topic, topic, TOPIC_LEN_MAX);
        awss_cmp_mqtt_register_cb(topic, awss_online_couple[i].cb);
    }

    online_init = 1;

    return 0;
}

int awss_cmp_online_deinit()
{
    uint8_t i;
    char topic[TOPIC_LEN_MAX] = {0};

    if (!online_init) {
        return 0;
    }

    awss_dev_bind_notify_stop();

    for (i = 0; i < sizeof(awss_online_couple) / sizeof(awss_online_couple[0]); i ++) {
        memset(topic, 0, sizeof(topic));
        awss_build_topic(awss_online_couple[i].topic, topic, TOPIC_LEN_MAX);
        awss_cmp_mqtt_unregister_cb(topic);
    }

    online_init = 0;

    return 0;
}

int awss_cmp_mqtt_get_payload(void *mesg, char **payload, uint32_t *playload_len)
{
    if (mesg == NULL || payload == NULL || playload_len == NULL) {
        return - 1;
    }

    iotx_mqtt_event_msg_pt msg = (iotx_mqtt_event_msg_pt)mesg;

    iotx_mqtt_topic_info_pt ptopic_info = (iotx_mqtt_topic_info_pt) msg->msg;

    switch (msg->event_type) {
        case IOTX_MQTT_EVENT_PUBLISH_RECEIVED:
            *playload_len = ptopic_info->payload_len;
            *payload = (char *)ptopic_info->payload;
            break;
        default:
            return -1;
    }
    return 0;
}
#if defined(__cplusplus)  /* If this is a C++ compiler, use C linkage */
}
#endif
