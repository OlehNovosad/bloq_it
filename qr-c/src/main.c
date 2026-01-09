#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qr_core.h"

/* ---------------------------------------------------------
 * PRIVATE FUNCTIONS
 * --------------------------------------------------------- */

/**
 * @enum command_t
 * @brief Supported commands received via stdin.
 *
 * INIT   - Initialize the QR device
 * PING   - Health check command
 * START  - Start QR processing
 * STOP   - Stop QR processing
 * UNKNOWN- Unrecognized command
 */
typedef enum
{
    INIT = 0,
    PING,
    START,
    STOP,
    UNKNOWN,
} command_t;

/**
 * @brief Removes trailing newline and carriage return characters from a string.
 *
 * This function modifies the input string in place by replacing the first
 * occurrence of '\n' or '\r' with a null terminator.
 *
 * @param str Pointer to a null-terminated string to be trimmed.
 */
static void trim(char* str)
{
    str[strcspn(str, "\r\n")] = '\0';
}

/**
 * @brief Parses a command string and maps it to a command_t value.
 *
 * Compares the input string against known command keywords and returns
 * the corresponding enum value.
 *
 * @param cmd Null-terminated command string (e.g. "INIT", "PING").
 * @return command_t Parsed command enum or UNKNOWN if no match is found.
 */
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

/**
 * @brief Main application entry point.
 *
 * Continuously reads commands from standard input, parses them,
 * and dispatches execution to the appropriate QR handler function.
 *
 * The program runs until EOF is received on stdin.
 *
 * @return int Exit status (0 on normal termination).
 */
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
