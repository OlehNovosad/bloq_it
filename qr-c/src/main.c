#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "qr_core.h"

/* ---------------------------------------------------------
 * COMMAND DEFINITIONS
 * --------------------------------------------------------- */

/**
 * @enum command_t
 * @brief Commands supported by the QR service.
 *
 * Commands are received line-by-line via STDIN.
 */
typedef enum
{
    CMD_INIT = 0,   /**< Initialize QR subsystem */
    CMD_PING,       /**< Health check */
    CMD_START,      /**< Start QR scan */
    CMD_STOP,       /**< Stop ongoing QR scan */
    CMD_UNKNOWN     /**< Unrecognized command */
} command_t;


/* ---------------------------------------------------------
 * THREADING & SYNCHRONIZATION
 * --------------------------------------------------------- */

/**
 * @brief Thread used to run qr_handle_start().
 *
 * START is executed in a separate thread so that the main
 * loop can continue receiving commands (e.g. STOP).
 */
static pthread_t start_thread;

/**
 * @brief Mutex protecting qr_handle_start().
 *
 * Ensures that only one START operation can run at a time.
 */
static pthread_mutex_t start_mutex = PTHREAD_MUTEX_INITIALIZER;


/* ---------------------------------------------------------
 * UTILITY FUNCTIONS
 * --------------------------------------------------------- */

/**
 * @brief Trim newline and carriage return characters.
 *
 * Modifies the string in-place by terminating it at the
 * first '\n' or '\r'.
 *
 * @param str Input string to trim.
 */
static void trim_line(char* str)
{
    str[strcspn(str, "\r\n")] = '\0';
}

/**
 * @brief Convert a command string to command_t.
 *
 * @param cmd Null-terminated command string.
 * @return Parsed command or CMD_UNKNOWN.
 */
static command_t parse_command(const char* cmd)
{
    if (strcmp(cmd, "INIT") == 0)
        return CMD_INIT;

    if (strcmp(cmd, "PING") == 0)
        return CMD_PING;

    if (strcmp(cmd, "START") == 0)
        return CMD_START;

    if (strcmp(cmd, "STOP") == 0)
        return CMD_STOP;

    return CMD_UNKNOWN;
}


/* ---------------------------------------------------------
 * THREAD ENTRY POINT
 * --------------------------------------------------------- */

/**
 * @brief Thread wrapper for qr_handle_start().
 *
 * The mutex guarantees that concurrent START commands
 * do not execute qr_handle_start() simultaneously.
 */
static void* start_thread_func(void* arg)
{
    (void)arg; /* unused */

    pthread_mutex_lock(&start_mutex);
    qr_handle_start();
    pthread_mutex_unlock(&start_mutex);

    return NULL;
}


/* ---------------------------------------------------------
 * MAIN ENTRY POINT
 * --------------------------------------------------------- */

/**
 * @brief QR command dispatcher.
 *
 * Reads commands from STDIN, parses them, and invokes the
 * appropriate handler. Runs until EOF is received.
 *
 * Supported commands:
 *  - INIT
 *  - PING
 *  - START
 *  - STOP
 *
 * @return 0 on normal termination.
 */
int main(void)
{
    char cmd[CMD_BUF_SIZE];

    pthread_mutex_init(&start_mutex, NULL);

    LOG_INFO("QR service started");

    while (fgets(cmd, sizeof(cmd), stdin))
    {
        trim_line(cmd);

        switch (parse_command(cmd))
        {
        case CMD_INIT:
            LOG_INFO("CMD: INIT");
            qr_handle_init();
            break;

        case CMD_PING:
            LOG_INFO("CMD: PING");
            qr_handle_ping();
            break;

        case CMD_START:
            LOG_INFO("CMD: START");
            pthread_create(&start_thread, NULL, start_thread_func, NULL);
            break;

        case CMD_STOP:
            LOG_INFO("CMD: STOP");
            qr_handle_stop();
            break;

        default:
            LOG_ERR("CMD: UNKNOWN");
            break;
        }
    }

    return 0;
}
