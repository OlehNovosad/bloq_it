#ifndef QR_CORE_H
#define QR_CORE_H

/* ---------------------------------------------------------
 * CONSTANTS & CONFIGURATION
 * --------------------------------------------------------- */

#define CMD_BUF_SIZE    128
#define SERIAL_BUF_SIZE 256
#define LOG_FILE_PATH   "logs/qr-c.log"

/* ---------------------------------------------------------
 * TYPES & ENUMS
 * --------------------------------------------------------- */

typedef enum
{
    RESULT_ERR = -1,
    RESULT_OK = 0
} result_t;

typedef enum
{
    STATE_BOOT = 0,
    STATE_READY,
    STATE_READING,
} state_t;

/* ---------------------------------------------------------
 * LOGGING & ERROR HANDLING
 * --------------------------------------------------------- */

void log_msg(const char* level, const char* msg);

/**
 * Validates environment conditions.
 * Returns from the calling function if condition fails.
 */
#define VERIFY_ENV(cond, name)                                          \
    do {                                                                \
        if (!(cond)) {                                                  \
            log_msg("ERROR", "Missing environment variable: " name);    \
            fprintf(stderr, "ERROR: missing %s vars\n", name);          \
            return;                                                     \
        }                                                               \
    } while (0)

/**
 * Asserts a condition; logs and returns QR_RESULT_ERR on failure.
 */
#define ASSERT_LOG(cond, msg)               \
    do {                                    \
        if (!(cond)) {                      \
        log_msg("ERROR", (msg));            \
            return RESULT_ERR;              \
        }                                   \
    } while (0)

#define LOG_ERR(msg) log_msg("ERROR", msg)
#define LOG_INFO(msg) log_msg("INFO", msg)
#define LOG_DEBUG(msg) log_msg("DEBUG", msg)

/* ---------------------------------------------------------
 * SYSTEM HANDLERS
 * --------------------------------------------------------- */

void qr_handle_init(void);
void qr_handle_ping(void);
void qr_handle_start(void);
void qr_handle_stop(void);

#endif /* QR_CORE_H */
