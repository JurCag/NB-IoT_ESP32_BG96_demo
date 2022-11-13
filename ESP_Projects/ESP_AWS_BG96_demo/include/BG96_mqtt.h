#ifndef __BG96_MQTT_H__
#define __BG96_MQTT_H__

#include "BG96_AT_cmds_core.h"
#include "uart.h"
#include "BG96.h"
#include "AT_cmds.h"

void BG96_configureMqttParams(void);
void BG96_connectToMqttServer(void);

void BG96_mqttOkResponseParser(BG96_AtPacket_t* packet, char* data);

#endif // __BG96_MQTT_H__
