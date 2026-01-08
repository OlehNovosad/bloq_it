#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qr.h"

/* ---------------------------------------------------------
 * PRIVATE FUNCTIONS
 * --------------------------------------------------------- */

static void trim(char* str) { str[strcspn(str, "\r\n")] = '\0'; }

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

        if (strcmp(cmd, "INIT") == 0)
        {
            LOG_INFO("CMD: INIT invoked");
            qr_handle_init();
        }
        else if (strcmp(cmd, "PING") == 0)
        {
            LOG_INFO("CMD: PING invoked");
            qr_handle_ping();
        }
        else if (strcmp(cmd, "START") == 0)
        {
            LOG_INFO("CMD: START invoked");
            qr_handle_start();
        }
        else if (strcmp(cmd, "STOP") == 0)
        {
            LOG_INFO("CMD: STOP invoked");
            qr_handle_stop();
        }
        else
        {
            LOG_ERR("Invalid command");
        }
    }

    return 0;
}
