#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#define ERR_FD 3

static void report_error(const char *msg, const char *line) {
    char buf[8192];
    int n = snprintf(buf, sizeof(buf),
                     "Ошибка: %s: \"%s\"\n", msg, line);
    if (n > 0) {
        ssize_t off = 0;
        while (off < n) {
            ssize_t w = write(ERR_FD, buf + off, n - off);
            if (w == -1) {
                if (errno == EINTR) continue;
                break;
            }
            off += w;
        }
    }
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    char *line = NULL;
    size_t cap = 0;
    ssize_t len;

    /* Читаем построчно из stdin (это pipe1[0]) */
    while ((len = getline(&line, &cap, stdin)) != -1) {
        /* Убираем завершающий \n */
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
            len--;
        }

        if (len == 0) {
            report_error("строка пуста", "");
            continue;
        }

        /* ПРАВИЛО: строка должна начинаться с заглавной буквы */
        if (isupper((unsigned char)line[0])) {
            /* Валидная -> stdout (файл) */
            printf("%s\n", line);
            fflush(stdout);
        } else {
            /* Невалидная -> pipe2 (родитель покажет в stdout) */
            report_error("строка должна начинаться с заглавной буквы", line);
        }
    }

    free(line);
    return 0;
}