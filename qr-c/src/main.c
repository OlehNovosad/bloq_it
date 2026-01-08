#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

/* ---------------------------------------------------------
 * MACROS & CONSTANTS
 * --------------------------------------------------------- */

typedef enum { Err = -1,
               Ok = 0 } result_t;

typedef enum {
    STATE_BOOT = 0,
    STATE_READY,
    STATE_READING,
} state_t;

#define CMD_BUF_SIZE (128)
#define SERIAL_BUF_SIZE (256)
#define LOG_FILE_PATH "logs/qr-c.log"

static const char *serial_port = NULL;
static int serial_baud = 0;
static int read_timeout_ms = 0;
static int serial_fd = 0;

static int stop_pipe[2] = {-1, -1};

static state_t state = STATE_BOOT;

/* ---------------------------------------------------------
 * MACRO FUNCTIONS
 * --------------------------------------------------------- */

#define VERIFY_ENV(cond, name)                        \
    do {                                              \
        if (!(cond)) {                                \
            log_msg("ERROR", "missing " name " env"); \
            printf("ERROR: missing " name " vars\n"); \
            fflush(stdout);                           \
            return;                                   \
        }                                             \
    } while (0)

#define ASSERT_LOG(cond, msg)      \
    do {                           \
        if (!(cond)) {             \
            log_msg("ERROR", msg); \
            return Err;            \
        }                          \
    } while (0)

/* ---------------------------------------------------------
 * PRIVATE FUNCTIONS
 * --------------------------------------------------------- */

static void log_msg(const char *level, const char *msg) {
    time_t now = time(NULL);

    FILE *f_log = fopen(LOG_FILE_PATH, "a");

    if (f_log == NULL) {
        fprintf(stderr, "Could not open log file! Falling back to stderr.\n");
        fprintf(stderr, "%ld [%s] %s\n", (long)now, level, msg);
        return;
    }

    fprintf(f_log, "%ld [%s] %s\n", (long)now, level, msg);

    fclose(f_log);
}

void trim(char *str) { str[strcspn(str, "\r\n")] = '\0'; }

int get_baud_rate(int serial_baud) {
    switch (serial_baud) {
    case 9600:
        return B9600;
    case 115200:
        return B115200;
    default:
        return B0;
    }
}

result_t open_serial() {
    serial_fd = open(serial_port, O_RDWR | O_NOCTTY | O_SYNC);
    ASSERT_LOG(serial_fd >= 0, "Can't open serial port");

    struct termios tty;
    ASSERT_LOG(tcgetattr(serial_fd, &tty) == 0, "tcgetattr failed");

    speed_t speed = get_baud_rate(serial_baud);
    ASSERT_LOG(speed != B0, "Incorrect baudrate");

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    tty.c_oflag &= ~OPOST;

    ASSERT_LOG(tcsetattr(serial_fd, TCSANOW, &tty) == 0, "tcsetattr failed");

    log_msg("INFO", "Serial port opened");
    return Ok;
}

/* ---------------------------------------------------------
 * Commands handler
 * --------------------------------------------------------- */

void init_handle(void) {
    serial_port = getenv("SERIAL_PORT");
    serial_baud = getenv("SERIAL_BAUD") ? atoi(getenv("SERIAL_BAUD")) : 0;
    read_timeout_ms =
        getenv("READ_TIMEOUT_MS") ? atoi(getenv("READ_TIMEOUT_MS")) : 0;

    VERIFY_ENV(serial_port != NULL, "serial_port");
    VERIFY_ENV(serial_baud != 0, "serial_baud");
    VERIFY_ENV(read_timeout_ms != 0, "read_timeout_ms");

    if (open_serial() != Ok) {
        printf("ERROR: serial open failed\n");
        fflush(stdout);
        return;
    }

    printf("OK\n");
    state = STATE_READY;
    fflush(stdout);
}

void ping_handle(void) {
    printf("PONG\n");
    fflush(stdout);
}

void start_handle(void) {
    if (state != STATE_READY) {
        printf("ERROR: invalid state\n");
        fflush(stdout);
        log_msg("ERROR", "invalid state for START");
        return;
    }

    state = STATE_READING;
    log_msg("INFO", "START received");

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(serial_fd, &rfds);
    FD_SET(stop_pipe[0], &rfds);

    int max_fd = serial_fd > stop_pipe[0] ? serial_fd : stop_pipe[0];

    struct timeval tv;
    tv.tv_sec = read_timeout_ms / 1000;
    tv.tv_usec = (read_timeout_ms % 1000) * 1000;

    int ret = select(max_fd + 1, &rfds, NULL, NULL, &tv);

    char debug_msg[128];
    sprintf(debug_msg, "select returned: %d", ret);
    log_msg("DEBUG", debug_msg);

    /* STOP interrupt */
    if (ret > 0 && FD_ISSET(stop_pipe[0], &rfds)) {
        char tmp[8];
        read(stop_pipe[0], tmp, sizeof(tmp)); /* clear pipe */
        log_msg("INFO", "Read interrupted by STOP");
        state = STATE_READY;
        return;
    }

    /* Serial data */
    if (ret > 0 && FD_ISSET(serial_fd, &rfds)) {
        char buf[SERIAL_BUF_SIZE] = {0};
        int n = read(serial_fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            time_t ts = time(NULL);
            printf("\"qr-data\": {\"code\":\"%s\",\"ts\":%ld}\n", buf, ts);
        } else {
            printf("ERROR: serial read\n");
        }
    }
    /* Timeout */
    else {
        time_t ts = time(NULL);
        log_msg("INFO", "Read timeout");
        printf("\"qr-data\": {\"code\":\"TIMEOUT\",\"ts\":%ld}\n", ts);
    }

    fflush(stdout);
    state = STATE_READY;
}

void stop_handle(void) {
    if (state == STATE_READING) {
        write(stop_pipe[1], "X", 1);
        log_msg("INFO", "STOP received");
        state = STATE_READY;
    }

    printf("OK\n");
    fflush(stdout);
}

/* ---------------------------------------------------------
 * MAIN ENTRY POINT
 * --------------------------------------------------------- */

int main(void) {
    char cmd[CMD_BUF_SIZE];

    setbuf(stdout, NULL);

    pipe(stop_pipe);

    fcntl(stop_pipe[0], F_SETFL, O_NONBLOCK);
    fcntl(stop_pipe[1], F_SETFL, O_NONBLOCK);

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
