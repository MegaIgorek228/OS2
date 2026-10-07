#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <sys/wait.h>

int main(void) {
    char filename[4096];

    printf("Имя файла для записи: ");
    fflush(stdout);
    if (!fgets(filename, sizeof(filename), stdin)) {
        perror("fgets");
        return 1;
    }
    filename[strcspn(filename, "\n")] = '\0';
    if (strlen(filename) == 0) {
        fprintf(stderr, "Пустое имя\n");
        return 1;
    }

    // Два канала
    int pipe1[2];  // parent -> child
    int pipe2[2];  // child -> parent
    if (pipe(pipe1) == -1) { perror("pipe1"); return 1; }
    if (pipe(pipe2) == -1) { perror("pipe2"); return 1; }

    pid_t pid = fork();
    if (pid == -1) { perror("fork"); return 1; }

    if (pid == 0) { // child
        if (dup2(pipe1[0], STDIN_FILENO) == -1) {
            perror("dup2 stdin"); _exit(1);
        }

        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) { perror("open"); _exit(1); }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2 stdout"); _exit(1);
        }

        if (dup2(pipe2[1], 3) == -1) {
            perror("dup2 err"); _exit(1);
        }

        close(pipe1[0]); close(pipe1[1]);
        close(pipe2[0]); close(pipe2[1]);
        close(fd);

        char *args[] = {"./child", filename, NULL};
        execv("./child", args);
        perror("execv");
        _exit(1);
    }
    close(pipe1[0]);
    close(pipe2[1]);

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = pipe2[0];
    fds[1].events = POLLIN;

    int stdin_open = 1;
    char buf[4096];

    while (stdin_open || fds[1].fd != -1) {
        int nfds = stdin_open ? 2 : 1;
        int r = poll(fds, nfds, -1);
        if (r == -1) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }

        if (stdin_open && (fds[0].revents & (POLLIN | POLLHUP))) {
            ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
            if (n > 0) {
                ssize_t off = 0;
                while (off < n) {
                    ssize_t w = write(pipe1[1], buf + off, n - off);
                    if (w == -1) { perror("write pipe1"); break; }
                    off += w;
                }
            } else if (n == 0) {
                stdin_open = 0;
                close(pipe1[1]);
                fds[0].fd = -1;
            } else {
                perror("read stdin");
                stdin_open = 0;
                close(pipe1[1]);
                fds[0].fd = -1;
            }
        }

        if (fds[1].fd != -1 && (fds[1].revents & (POLLIN | POLLHUP))) {
            ssize_t n = read(pipe2[0], buf, sizeof(buf));
            if (n > 0) {
                write(STDOUT_FILENO, buf, n);
            } else {
                close(pipe2[0]);
                fds[1].fd = -1;
            }
        }
    }
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return 1;
    }
    if (WIFEXITED(status)) {
        printf("\n[parent] Ребёнок завершился с кодом %d\n",
               WEXITSTATUS(status));
    }

    return 0;
}