#include <time.h>
#include <stdio.h>

#include "qr_core.h"

/**
 * @brief Writes a timestamped log message to the application log file.
 *
 * Prepends the current local time and log level to the provided message
 * and appends it to the log file defined by LOG_FILE_PATH.
 *
 * If the log file cannot be opened, the message is printed to stderr
 * instead, including the timestamp and log level.
 *
 * @param level Log severity level (e.g. "INFO", "ERROR", "DEBUG").
 * @param msg   Null-terminated log message to be written.
 */
void log_msg(const char* level, const char* msg)
{
    char ts_buf[24];
    const time_t now = time(NULL);
    const struct tm* tm = localtime(&now);
    strftime(ts_buf, sizeof(ts_buf), "%Y-%m-%d %H:%M:%S", tm);

    FILE* f_log = fopen(LOG_FILE_PATH, "a");

    if (f_log == NULL)
    {
        fprintf(stderr, "[%s] [%s] %s (Log file error)\n", ts_buf, level, msg);
        return;
    }

    fprintf(f_log, "[%s] [%s] %s\n", ts_buf, level, msg);
    fclose(f_log);
}
