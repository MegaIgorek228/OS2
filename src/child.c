#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

int main(void) {
    char line[4096];

    while (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\n")] = '\0';

        if (line[0] == '\0') {
            fprintf(stderr, "Ошибка: строка пуста\n");
            fflush(stderr);
            continue;
        }

        if (isupper((unsigned char)line[0])) {
            printf("%s\n", line);
            fflush(stdout);
        } else {
            fprintf(stderr,
                "Ошибка: строка должна начинаться с заглавной буквы: \"%s\"\n",
                line);
            fflush(stderr);
        }
    }

    return 0;
}
