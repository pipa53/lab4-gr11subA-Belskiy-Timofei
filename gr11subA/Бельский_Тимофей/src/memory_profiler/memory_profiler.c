#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <unistd.h>

#include <fcntl.h>

#include <sys/stat.h>

#include <time.h>

#include <sys/resource.h>


#define MAX_LINE 1024

// Функция для вывода метрик памяти
void print_memory_metrics(pid_t pid) {
    char path[256];
    char line[MAX_LINE];

    // VSZ, RSS
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *status = fopen(path, "r");
    if (!status) {
        perror("fopen /proc/[PID]/status");
        return;
    }

    unsigned long vsz = 0, rss = 0;
    while (fgets(line, sizeof(line), status)) {
        if (strncmp(line, "VmSize:", 7) == 0) {
            sscanf(line, "VmSize: %lu kB", &vsz); // Размер виртуальной памяти
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS: %lu kB", &rss); // Размер резидентной памяти
        }
    }
    fclose(status);

    // PSS, USS
    snprintf(path, sizeof(path), "/proc/%d/smaps_rollup", pid);
    FILE *smaps = fopen(path, "r");
    if (!smaps) {
        perror("fopen /proc/[PID]/smaps_rollup");
        return;
    }

    unsigned long pss = 0, uss = 0;
    while (fgets(line, sizeof(line), smaps)) {
        if (strncmp(line, "Pss:", 4) == 0) {
            sscanf(line, "Pss: %lu kB", &pss); // Пропорциональная память
        } else if (strncmp(line, "Private_Clean:", 14) == 0 || strncmp(line, "Private_Dirty:", 14) == 0) {
            unsigned long private_mem;
            sscanf(line, "%*s %lu kB", &private_mem); // Уникальная память
            uss += private_mem;
        }
    }
    fclose(smaps);

    // Page faults
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *stat = fopen(path, "r");
    if (!stat) {
        perror("fopen /proc/[PID]/stat");
        return;
    }

    unsigned long min_flt = 0, maj_flt = 0;
    int result = fscanf(stat, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %lu %lu", &min_flt, &maj_flt);
    if (result != 2) { // Проверяем, что fscanf успешно считал оба значения
        fprintf(stderr, "Error reading page faults from /proc/%d/stat\n", pid);
        fclose(stat);
        return;
    }
    fclose(stat);

    // Вывод метрик
    printf("Process: %d\n", pid);
    printf("VSZ: %.1f MB\n", vsz / 1024.0); // Виртуальная память
    printf("RSS: %.1f MB\n", rss / 1024.0); // Резидентная память
    printf("PSS: %.1f MB\n", pss / 1024.0); // Пропорциональная память
    printf("USS: %.1f MB\n", uss / 1024.0); // Уникальная память
    printf("\nPage Faults:\n");
    printf("Minor: %lu\n", min_flt); // Мягкие ошибки страниц
    printf("Major: %lu\n", maj_flt); // Жесткие ошибки страниц
}

// Функция для парсинга карты памяти
void parse_maps(pid_t pid) {
    char path[256];
    char line[MAX_LINE];

    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE *maps = fopen(path, "r");
    if (!maps) {
        perror("fopen /proc/[PID]/maps");
        return;
    }

    printf("\nMemory Map:\n");
    while (fgets(line, sizeof(line), maps)) {
        unsigned long start, end;
        char perms[5], path[128];
        sscanf(line, "%lx-%lx %s %*s %*s %*s %[^\n]", &start, &end, perms, path);

        printf("Range: %lx-%lx, Permissions: %s, Path: %s\n", start, end, perms, path);
    }
    fclose(maps);
}

// Динамический мониторинг
void monitor_memory(pid_t pid) {
    unsigned long prev_rss = 0;

    while (1) {
        char path[256];
        char line[MAX_LINE];
        unsigned long rss = 0;

        snprintf(path, sizeof(path), "/proc/%d/status", pid);
        FILE *status = fopen(path, "r");
        if (!status) {
            perror("fopen /proc/[PID]/status");
            break;
        }

        while (fgets(line, sizeof(line), status)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                sscanf(line, "VmRSS: %lu kB", &rss); // Считываем текущее значение RSS
            }
        }
        fclose(status);

        printf("RSS: %.1f MB", rss / 1024.0);
        if (prev_rss != 0) {
            printf(" (Delta: %.1f MB)", (rss - prev_rss) / 1024.0); // Изменение RSS
        }
        printf("\n");

        prev_rss = rss;
        sleep(1); // Обновление каждую секунду
    }
}

// Сравнение двух процессов
void compare_processes(pid_t pid1, pid_t pid2) {
    printf("\nComparing processes %d and %d:\n", pid1, pid2);

    // Метрики для первого процесса
    printf("Process %d:\n", pid1);
    print_memory_metrics(pid1);

    // Метрики для второго процесса
    printf("\nProcess %d:\n", pid2);
    print_memory_metrics(pid2);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <PID> [--watch] [--compare PID2]\n", argv[0]);
        return 1;
    }

    pid_t pid = atoi(argv[1]);

    // Проверка существования процесса
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    if (access(path, F_OK) != 0) {
        perror("Process does not exist");
        return 1;
    }

    if (argc > 2 && strcmp(argv[2], "--watch") == 0) {
        // Динамический мониторинг
        monitor_memory(pid);
    } else if (argc > 3 && strcmp(argv[2], "--compare") == 0) {
        // Сравнение двух процессов
        pid_t pid2 = atoi(argv[3]);
        compare_processes(pid, pid2);
    } else {
        // Базовый режим
        print_memory_metrics(pid);
        parse_maps(pid);
    }

    return 0;
}
