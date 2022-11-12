#ifndef __AT_CMDS_H__
#define __AT_CMDS_H__

#include <stdint.h>

#define MAX_PARAMS      (8)
#define ARG_LEN         (128)

typedef enum 
{
    TEST_COMMAND         = 0,
    READ_COMMAND         = 1,
    WRITE_COMMAND        = 2,
    EXECUTION_COMMAND    = 3
} AtCmdType_t;

typedef struct
{
    char* cmd;
    char arg[ARG_LEN];
    // char* response // napr. +CSQ:
    char* confirmation;
    char* error;
    uint16_t maxRespTime_ms;
} AtCmd_t;

typedef enum
{
    SSL_CTX_ID_0 = 0,
    SSL_CTX_ID_1 = 1,
    SSL_CTX_ID_2 = 2,
    SSL_CTX_ID_3 = 3,
    SSL_CTX_ID_4 = 4,
    SSL_CTX_ID_5 = 5,
} SslContextID_t;

typedef enum
{
    SSL_3_0 = 0,
    TLS_1_0 = 1,
    TLS_1_1 = 2,
    TLS_1_2 = 3,
    ALL     = 4
} SslVersion_t;

typedef enum
{
    NO_AUTHENTICATION = 0,
    MANAGE_SERVER_AUTHENTICATION = 1,
    MANAGE_SERVER_AND_CLIENT_AUTHENTICATION = 2
} SslSecLevel_t;

typedef enum
{
    CONCERN_VALIDITY_CHECK = 0,
    IGNORE_VALIDITY_CHECK = 1
} SslIgnoreLocalTime_t;

// General Commands
AtCmd_t AT_setCommandEchoMode;

// SIM Related Commands
AtCmd_t AT_enterPIN;

// Network Service Commands
AtCmd_t AT_signalQualityReport;
AtCmd_t AT_networkRegistrationStatus;

// Packet Domain Commands
AtCmd_t AT_attachmentOrDetachmentOfPS;

// SSL AT Commands
AtCmd_t AT_configureParametersOfAnSSLContext;

#endif // __AT_CMDS_H__
