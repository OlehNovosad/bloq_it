#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

#ifdef __linux__
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#else
#include <time.h>
#endif

#include "qr_core.h"

/* ---------------------------------------------------------
 * STATIC STATE
 * --------------------------------------------------------- */

/**
 * @brief Global runtime context for the QR module.
 *
 * Holds configuration parameters, file descriptors, IPC primitives,
 * and the current state of the QR device lifecycle.
 *
 * This structure is initialized once and persists for the entire
 * application runtime.
 */
static struct
{
    const char* port;       /**< Serial device path (e.g. /dev/ttyS1) */
    int baud;               /**< Serial baud rate */
    int timeout_ms;         /**< Read timeout in milliseconds */
    int serial_fd;          /**< Open serial port file descriptor */
    int stop_pipe[2];       /**< Pipe used to interrupt blocking reads */
    state_t state;          /**< Current device state */
} g_ctx = {.serial_fd = -1, .stop_pipe = {-1, -1}, .state = STATE_BOOT};

/* ---------------------------------------------------------
 * PRIVATE FUNCTIONS
 * --------------------------------------------------------- */

/**
 * @brief Converts an integer baud rate to a POSIX speed_t value.
 *
 * @param baud Integer baud rate (e.g. 9600, 115200).
 * @return speed_t Corresponding POSIX baud rate constant or B0 if unsupported.
 */
static speed_t get_baud_rate(const int baud)
{
    switch (baud)
    {
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
    default: return B0;
    }
}

/**
 * @brief Opens and configures the serial port.
 *
 * Applies raw serial settings (8N1, no flow control, non-canonical mode)
 * and flushes any pending I/O.
 *
 * @return result_t RESULT_OK on success, assertion failure otherwise.
 */
static result_t open_serial()
{
    g_ctx.serial_fd = open(g_ctx.port, O_RDWR | O_NOCTTY | O_SYNC);
    ASSERT_LOG(g_ctx.serial_fd >= 0, "Can't open serial port");

    struct termios tty;
    if (tcgetattr(g_ctx.serial_fd, &tty) != 0)
    {
        close(g_ctx.serial_fd);
        ASSERT_LOG(0, "tcgetattr failed");
    }

    const speed_t speed = get_baud_rate(g_ctx.baud);
    if (speed == B0)
    {
        close(g_ctx.serial_fd);
        ASSERT_LOG(0, "Unsupported baudrate");
    }

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | CSTOPB);
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK | BRKINT | PARMRK |
        ISTRIP | INLCR | IGNCR | ICRNL);
    tty.c_oflag &= ~OPOST;

    if (tcsetattr(g_ctx.serial_fd, TCSANOW, &tty) != 0)
    {
        close(g_ctx.serial_fd);
        ASSERT_LOG(0, "tcsetattr failed");
    }

    tcflush(g_ctx.serial_fd, TCIOFLUSH);
    LOG_INFO("Serial port opened");
    return RESULT_OK;
}

/* ---------------------------------------------------------
 * COMMANDS HANDLER
 * --------------------------------------------------------- */

/**
 * @brief Initializes the QR device and communication resources.
 *
 * Reads configuration from environment variables, sets up IPC pipes,
 * opens the serial port, and transitions the state to READY.
 *
 * Expected environment variables:
 *  - SERIAL_PORT
 *  - SERIAL_BAUD
 *  - READ_TIMEOUT_MS
 */
void qr_handle_init(void)
{
    if (g_ctx.state != STATE_BOOT)
    {
        LOG_ERR("Already initialized");
        fflush(stdout);
        return;
    }

    g_ctx.port = getenv("SERIAL_PORT");
    const char* baud_env = getenv("SERIAL_BAUD");
    const char* timeout_env = getenv("READ_TIMEOUT_MS");

    VERIFY_ENV(g_ctx.port != NULL, "SERIAL_PORT");
    VERIFY_ENV(baud_env != NULL, "SERIAL_BAUD");
    VERIFY_ENV(timeout_env != NULL, "READ_TIMEOUT_MS");

    g_ctx.baud = atoi(baud_env);
    g_ctx.timeout_ms = atoi(timeout_env);

    // Setup Communication Pipes for STOP COMMAND
    if (pipe(g_ctx.stop_pipe) == -1)
    {
        perror("pipe");
        return;
    }
    fcntl(g_ctx.stop_pipe[0], F_SETFL, O_NONBLOCK);

    if (open_serial() != RESULT_OK)
    {
        LOG_ERR("serial open failed");
        return;
    }

    g_ctx.state = STATE_READY;
    printf("OK\n");
}

/**
 * @brief Responds to a health check command.
 *
 * Prints a simple "PONG" response to stdout.
 */
void qr_handle_ping(void)
{
    printf("PONG\n");
    fflush(stdout);
}

/**
 * @brief Starts a QR scan operation.
 *
 * Blocks until QR data is received from the serial port, a STOP command
 * is issued, or the configured timeout expires.
 *
 * Outputs scan results or timeout information as JSON to stdout.
 */
void qr_handle_start(void)
{
    if (g_ctx.state != STATE_READY)
    {
        LOG_ERR("invalid state");
        return;
    }

    g_ctx.state = STATE_READING;
    LOG_INFO("Starting QR read scan");

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(g_ctx.serial_fd, &rfds);
    FD_SET(g_ctx.stop_pipe[0], &rfds);

    const int max_fd = (g_ctx.serial_fd > g_ctx.stop_pipe[0]) ? g_ctx.serial_fd : g_ctx.stop_pipe[0];

    struct timeval tv;
    tv.tv_sec = g_ctx.timeout_ms / 1000;
    tv.tv_usec = (g_ctx.timeout_ms % 1000) * 1000;

    const int ret = select(max_fd + 1, &rfds, NULL, NULL, &tv);

    if (ret > 0)
    {
        if (FD_ISSET(g_ctx.stop_pipe[0], &rfds))
        {
            char dummy;
            while (read(g_ctx.stop_pipe[0], &dummy, 1) > 0);
            LOG_INFO("Scan aborted by STOP");
        }
        else if (FD_ISSET(g_ctx.serial_fd, &rfds))
        {
            char buf[SERIAL_BUF_SIZE] = {0};
            const ssize_t n = read(g_ctx.serial_fd, buf, sizeof(buf) - 1);
            if (n > 0)
            {
                if (buf[n - 1] == '\r' || buf[n - 1] == '\n')
                    buf[n - 1] = '\0';

                printf("{\"qr-data\": {\"code\":\"%s\",\"ts\":%ld}}\n",
                       buf, (long)time(NULL));
                fflush(stdout);
            }
            else
            {
                LOG_ERR("read error");
            }
        }
    }
    else if (ret == 0)
    {
        printf("{\"qr-data\": {\"code\":\"TIMEOUT\",\"ts\":%ld}}\n",
               (long)time(NULL));
        fflush(stdout);
    }
    else if (errno != EINTR)
    {
        LOG_ERR("select failed");
    }

    g_ctx.state = STATE_READY;
}

/**
 * @brief Stops an ongoing QR scan.
 *
 * Signals the scan loop via an IPC pipe and returns immediately.
 * Always responds with "OK".
 */
void qr_handle_stop(void)
{
    if (g_ctx.state == STATE_READING)
    {
        if (write(g_ctx.stop_pipe[1], "!", 1) == -1)
        {
            LOG_ERR("Failed to write to stop pipe");
        }
    }
    printf("OK\n");
    fflush(stdout);
}
