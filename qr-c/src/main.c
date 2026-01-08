#include <stdio.h>
#include <string.h>
#include <time.h>

/* ---------------------------------------------------------
 * MACROS & CONSTANTS
 * --------------------------------------------------------- */

#define CMD_BUF_SIZE (128)
#define LOG_FILE_PATH "logs/qr-c.log"

/* ---------------------------------------------------------
 * PRIVATE FUNCTIONS
 * --------------------------------------------------------- */

static void log_msg(const char *level, const char *msg) {
    time_t now = time(NULL);

    FILE *flog = fopen(LOG_FILE_PATH, "a");

    if (flog == NULL) {
        fprintf(stderr, "Could not open log file! Falling back to stderr.\n");
        fprintf(stderr, "%ld [%s] %s\n", (long)now, level, msg);
        return;
    }

    fprintf(flog, "%ld [%s] %s\n", (long)now, level, msg);

    fclose(flog);
}

static void trim(char *str) {
    str[strcspn(str, "\r\n")] = '\0';
}

/* ---------------------------------------------------------
 * Commands handler
 * --------------------------------------------------------- */

void init_handle(void) {}

void ping_handle(void) {}

void start_handle(void) {}

void stop_handle(void) {}

/* ---------------------------------------------------------
 * MAIN ENTRY POINT
 * --------------------------------------------------------- */

int main(void) {
    char cmd[CMD_BUF_SIZE];

    log_msg("INFO", "QR started");

    while (fgets(cmd, sizeof(cmd), stdin)) {
        trim(cmd);

        if (strcmp(cmd, "INIT") == 0) {
            log_msg("DEBUG", "CMD: INIT invoked");
            init_handle();
        } else if (strcmp(cmd, "PING") == 0) {
            log_msg("DEBUG", "CMD: PING invoked");
            ping_handle();
        } else if (strcmp(cmd, "START") == 0) {
            log_msg("DEBUG", "CMD: START invoked");
            start_handle();
        } else if (strcmp(cmd, "STOP") == 0) {
            log_msg("DEBUG", "CMD: STOP invoked");
            stop_handle();
        } else {
            log_msg("ERROR", "Invalid command");
        }
    }

    return 0;
}
