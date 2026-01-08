#include <stdio.h>
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

/* ---------------------------------------------------------
 * MAIN ENTRY POINT
 * --------------------------------------------------------- */

int main(void) {
    log_msg("DEBUG", "Hello world!");

    return 0;
}
