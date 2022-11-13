#ifndef __BG96_TCPIP_H__
#define __BG96_TCPIP_H__

#include "BG96_AT_cmds_core.h"
#include "BG96.h"
#include "AT_cmds.h"

void BG96_configureTcpIpParams(void);

void BG96_tcpipOkResponseParser(BG96_AtPacket_t* packet, char* data);


#endif // __BG96_TCPIP_H__
