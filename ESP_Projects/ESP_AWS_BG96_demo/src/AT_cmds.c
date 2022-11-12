#include "AT_cmds.h"

/* BG96 AT Commands */

// General Commands
AtCmd_t AT_setCommandEchoMode = 
{
    .cmd = "E1",
    .arg = "",
    .confirmation = "OK",
    .error = "ERROR",
    .maxRespTime_ms = 300
};

// SIM Related Commands
AtCmd_t AT_enterPIN = 
{
    .cmd = "+CPIN",
    .arg = "",
    .confirmation = "OK",
    .error = "ERROR",
    .maxRespTime_ms = 300
};

// Network Service Commands
AtCmd_t AT_signalQualityReport = 
{
    .cmd = "+CSQ",
    .arg = "",
    .confirmation = "OK",
    .error = "+CME ERROR:",
    .maxRespTime_ms = 300
};

AtCmd_t AT_networkRegistrationStatus = 
{
    .cmd = "+CREG",
    .arg = "",
    .confirmation = "OK",
    .error = "ERROR",
    .maxRespTime_ms = 300
};

// Packet Domain Commands
AtCmd_t AT_attachmentOrDetachmentOfPS = 
{
    .cmd = "+CGATT",
    .arg = "",
    .confirmation = "OK",
    .error = "ERROR",
    .maxRespTime_ms = 300
};

// SSL AT Commands
AtCmd_t AT_configureParametersOfAnSSLContext = 
{
    .cmd = "+QSSLCFG",
    .arg = "",
    .confirmation = "OK",
    .error = "ERROR",
    .maxRespTime_ms = 300
};