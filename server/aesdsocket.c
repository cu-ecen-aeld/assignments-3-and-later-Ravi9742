#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>

#define PORT 9000
#define BACKLOG 10
#define DATAFILE "/var/tmp/aesdsocketdata"

static int server_fd = -1;
static int client_fd = -1;
static volatile sig_atomic_t stop_server = 0;

void cleanup_and_exit(int code)
{
    if (client_fd != -1) close(client_fd);
    if (server_fd != -1) close(server_fd);
    remove(DATAFILE);   // delete the data file
    closelog();
    exit(code);
}

void signal_handler(int sig)
{
    syslog(LOG_INFO, "Caught signal, exiting");
    stop_server = 1;
}

void daemonize(void)
{
    pid_t pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "fork failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        // Parent exits, child continues as daemon
        exit(EXIT_SUCCESS);
    }

    // Become session leader
    if (setsid() < 0) {
        syslog(LOG_ERR, "setsid failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Second fork is optional; one fork is enough for this use case
    umask(0); // Clear file mode creation mask
    if (chdir("/") < 0) {
        syslog(LOG_ERR, "chdir failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Redirect stdin/out/err to /dev/null
    int fd = open("/dev/null", O_RDWR);
    if (fd != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > 2) close(fd);
    }
}

int main(int argc, char *argv[])
{
    bool daemon_mode = false;
    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = true;
    } else if (argc > 1) {
        fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    char client_ip[INET_ADDRSTRLEN];

    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);

    // Setup signals
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGTERM, &sa, NULL) == -1) {
        syslog(LOG_ERR, "sigaction failed: %s", strerror(errno));
        return -1;
    }

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        syslog(LOG_ERR, "socket failed: %s", strerror(errno));
        return -1;
    }

    // Allow reuse of address
    int yes = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        syslog(LOG_ERR, "setsockopt failed: %s", strerror(errno));
        cleanup_and_exit(-1);
    }

    // Bind to port 9000
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        syslog(LOG_ERR, "bind failed: %s", strerror(errno));
        cleanup_and_exit(-1);
    }

    // Daemonize after successful bind
    if (daemon_mode) {
        daemonize();
    }

    if (listen(server_fd, BACKLOG) == -1) {
        syslog(LOG_ERR, "listen failed: %s", strerror(errno));
        cleanup_and_exit(-1);
    }

    syslog(LOG_INFO, "Server listening on port %d", PORT);

    // Accept loop
    while (!stop_server) {
        addr_size = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);
        if (client_fd == -1) {
            if (errno == EINTR && stop_server) break;
            syslog(LOG_ERR, "accept failed: %s", strerror(errno));
            continue;
        }

        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        // Receive data until newline
        char *recv_buf = NULL;
        size_t buf_size = 0;
        ssize_t bytes;
        char temp[1024];
        int found_newline = 0;

        while (!found_newline && (bytes = recv(client_fd, temp, sizeof(temp), 0)) > 0) {
            char *new_ptr = realloc(recv_buf, buf_size + bytes);
            if (!new_ptr) {
                syslog(LOG_ERR, "malloc failed");
                free(recv_buf);
                break;
            }
            recv_buf = new_ptr;
            memcpy(recv_buf + buf_size, temp, bytes);
            buf_size += bytes;

            for (ssize_t i = 0; i < bytes; i++) {
                if (temp[i] == '\n') {
                    found_newline = 1;
                    break;
                }
            }
        }

        if (buf_size > 0 && recv_buf) {
            // Append to file
            int fd = open(DATAFILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd == -1) {
                syslog(LOG_ERR, "open file failed: %s", strerror(errno));
            } else {
                if (write(fd, recv_buf, buf_size) != buf_size) {
                    syslog(LOG_ERR, "write failed: %s", strerror(errno));
                }
                close(fd);
            }

            // Send back entire file content
            fd = open(DATAFILE, O_RDONLY);
            if (fd != -1) {
                char filebuf[1024];
                ssize_t n;
                while ((n = read(fd, filebuf, sizeof(filebuf))) > 0) {
                    if (send(client_fd, filebuf, n, 0) != n) {
                        syslog(LOG_ERR, "send failed: %s", strerror(errno));
                        break;
                    }
                }
                close(fd);
            }
            free(recv_buf);
        }

        syslog(LOG_INFO, "Closed connection from %s", client_ip);
        close(client_fd);
        client_fd = -1;
    }

    cleanup_and_exit(0);
    return 0;
}

