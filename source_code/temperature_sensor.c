#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define TEMP_BASE    20.0f
#define TEMP_RANGE   15.0f

static float read_temperature(void)
{
    return TEMP_BASE + ((float)rand() / RAND_MAX) * TEMP_RANGE;
}

int main(void)
{
    srand((unsigned int)time(NULL));

    printf("Sicaklik Sensoru Baslatildi\n");
    printf("----------------------------\n");

    while (1) {
        float temp = read_temperature();

        time_t now = time(NULL);
        struct tm *t = localtime(&now);

        printf("[%02d:%02d:%02d] Sicaklik: %.2f °C\n",
               t->tm_hour, t->tm_min, t->tm_sec, temp);

        fflush(stdout);
        sleep(1);
    }

    return 0;
}
