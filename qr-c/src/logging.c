#include <time.h>
#include <stdio.h>

#include "qr.h"

void log_msg(const char* level, const char* msg)
{
    char ts_buf[24];
    time_t now = time(NULL);
    struct tm* tm = localtime(&now);
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
