#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define MSG_COUNT 5

static float fake_temperature(int i)
{
    return 20.0f + (float)(i * 3 % 15);
}

int main(void)
{
    int pipefd[2];

    printf("IPC Demo - fork() + pipe()\n");
    printf("---------------------------\n");

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /* Child: pipe'tan oku, ekrana yaz */
        close(pipefd[1]);

        char buf[64];
        ssize_t n;
        printf("[Child  PID=%d] Okuma bekleniyor...\n", getpid());
        fflush(stdout);

        while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
            buf[n] = '\0';
            printf("[Child  PID=%d] Alindi: %s", getpid(), buf);
            fflush(stdout);
        }

        close(pipefd[0]);
        printf("[Child  PID=%d] Pipe kapandi, cikiliyor.\n", getpid());
        exit(0);
    } else {
        /* Parent: olcum yap, pipe'a yaz */
        close(pipefd[0]);

        printf("[Parent PID=%d] Veri gonderiliyor...\n", getpid());
        fflush(stdout);

        for (int i = 0; i < MSG_COUNT; i++) {
            char msg[64];
            float temp = fake_temperature(i);
            int len = snprintf(msg, sizeof(msg),
                               "Olcum #%d: %.1f C\n", i + 1, temp);
            if (write(pipefd[1], msg, len) < 0) break;
            sleep(1);
        }

        close(pipefd[1]);
        waitpid(pid, NULL, 0);
        printf("[Parent PID=%d] Tamamlandi.\n", getpid());
    }

    return 0;
}
