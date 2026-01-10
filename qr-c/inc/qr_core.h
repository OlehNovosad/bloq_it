#ifndef QR_CORE_H
#define QR_CORE_H

/**
 * @file qr_core.h
 * @brief Core public API for the QR device controller.
 *
 * Defines configuration constants, core types, logging utilities,
 * and command handler interfaces for the QR module.
 */

/* ---------------------------------------------------------
 * CONSTANTS & CONFIGURATION
 * --------------------------------------------------------- */

/** Maximum size of a command read from stdin */
#define CMD_BUF_SIZE    128

/** Maximum size of a serial read buffer */
#define SERIAL_BUF_SIZE 256

/** Path to the application log file */
#define LOG_FILE_PATH   "logs/qr-c.log"

/* ---------------------------------------------------------
 * TYPES & ENUMS
 * --------------------------------------------------------- */

/**
 * @brief Generic result codes returned by internal operations.
 */
typedef enum
{
    RESULT_ERR = -1, /**< Operation failed */
    RESULT_OK = 0 /**< Operation succeeded */
} result_t;

/**
 * @brief QR device lifecycle states.
 *
 * STATE_BOOT     - Initial startup state
 * STATE_READY    - Initialized and ready to read
 * STATE_READING  - Actively reading QR data
 * STATE_ERROR    - Error state, requires re-initialization
 * STATE_UNKNOWN  - Undefined state
 */
typedef enum
{
    STATE_BOOT = 0,
    STATE_READY,
    STATE_READING,
    STATE_ERROR,
    STATE_UNKNOWN,
} state_t;

/* ---------------------------------------------------------
 * LOGGING & ERROR HANDLING
 * --------------------------------------------------------- */

/**
 * @brief Writes a timestamped log entry.
 *
 * Appends a log message with the given severity level to the log file.
 * Falls back to stderr if the log file cannot be opened.
 *
 * @param level Log severity (e.g. "INFO", "ERROR", "DEBUG")
 * @param msg   Null-terminated message string
 */
void log_msg(const char* level, const char* msg);

/**
 * @brief Validates a required environment variable.
 *
 * If the condition fails, logs an error message and returns from
 * the calling function.
 *
 * @param cond Expression that must evaluate to true
 * @param name Environment variable name (string literal)
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
 * @brief Asserts a condition inside a function returning result_t.
 *
 * If the condition fails, logs the error message and returns RESULT_ERR.
 *
 * @param cond Expression that must evaluate to true
 * @param msg  Error message to log
 */
#define ASSERT_LOG(cond, msg)               \
    do {                                    \
        if (!(cond)) {                      \
            log_msg("ERROR", (msg));        \
            return RESULT_ERR;              \
        }                                   \
    } while (0)

/** Logs an error-level message */
#define LOG_ERR(msg)   log_msg("ERROR", msg)

/** Logs an info-level message */
#define LOG_INFO(msg)  log_msg("INFO", msg)

/** Logs a debug-level message */
#define LOG_DEBUG(msg) log_msg("DEBUG", msg)

/* ---------------------------------------------------------
 * SYSTEM HANDLERS
 * --------------------------------------------------------- */

/**
 * @brief Initializes the QR subsystem.
 *
 * Reads configuration from environment variables, initializes
 * the serial interface, and prepares the system for QR scanning.
 */
void qr_handle_init(void);

/**
 * @brief Performs a health-check operation.
 *
 * Responds immediately to confirm the service is alive.
 */
void qr_handle_ping(void);

/**
 * @brief Starts a QR scan operation.
 *
 * Blocks until QR data is received, a timeout occurs,
 * or a stop command is issued.
 */
void qr_handle_start(void);

/**
 * @brief Stops an ongoing QR scan.
 *
 * Safely interrupts a blocking read operation and returns
 * the system to a ready state.
 */
void qr_handle_stop(void);

#endif /* QR_CORE_H */
