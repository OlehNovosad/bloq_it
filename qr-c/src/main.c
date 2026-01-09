#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qr.h"

/* ---------------------------------------------------------
 * PRIVATE FUNCTIONS
 * --------------------------------------------------------- */

typedef enum
{
    INIT = 0,
    PING,
    START,
    STOP,
    UNKNOWN,
} command_t;

static void trim(char* str) { str[strcspn(str, "\r\n")] = '\0'; }

static command_t parse_command(const char* cmd)
{
    if (strcmp(cmd, "INIT") == 0)
    {
        return INIT;
    }
    else if (strcmp(cmd, "PING") == 0)
    {
        return PING;
    }
    else if (strcmp(cmd, "START") == 0)
    {
        return START;
    }
    else if (strcmp(cmd, "STOP") == 0)
    {
        return STOP;
    }
    else
    {
        return UNKNOWN;
    }
}

/* ---------------------------------------------------------
 * MAIN ENTRY POINT
 * --------------------------------------------------------- */

int main(void)
{
    char cmd[CMD_BUF_SIZE];

    LOG_INFO("QR started");

    while (fgets(cmd, sizeof(cmd), stdin))
    {
        trim(cmd);

        switch (parse_command(cmd))
        {
        case INIT:
            LOG_INFO("CMD: INIT invoked");
            qr_handle_init();
            break;
        case PING:
            LOG_INFO("CMD: PING invoked");
            qr_handle_ping();
            break;
        case START:
            LOG_INFO("CMD: START invoked");
            qr_handle_start();
            break;
        case STOP:
            LOG_INFO("CMD: STOP invoked");
            qr_handle_stop();
            break;
        default:
            LOG_ERR("Invalid command");
            break;
        }
    }

    return 0;
}
