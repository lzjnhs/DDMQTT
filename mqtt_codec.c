//
// Created by zr on 23-4-20.
//
#include "mqtt_codec.h"
#include "mqtt_event.h"
#include "mqtt_tcp_conn.h"
#include "mqtt_util.h"
#include <stdlib.h>
#include <stdio.h>

static void decode_tcp_message_(tmq_codec_t* codec, tmq_tcp_conn_t* conn, tmq_buffer_t* buffer)
{

}

void tmq_codec_init(tmq_codec_t* codec)
{
    codec->decode_tcp_message = decode_tcp_message_;
}