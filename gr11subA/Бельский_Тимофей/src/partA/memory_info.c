#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <sys/mman.h>

#include <unistd.h>

#include <sys/stat.h>

#include <fcntl.h>

// Функция для парсинга карты памяти
void print_memory_map() {
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) {
        perror("fopen /proc/self/maps");
        return;
    }

    char line[256];
    printf("\n=== Memory Map (from /proc/self/maps) ===\n");
    printf("Start-End       Perm  Offset  Device  Inode   Path\n");
    
    while (fgets(line, sizeof(line), f)) {
        char start[10], end[10], perms[5], offset[10], dev[10], inode[10], path[256];
        if (sscanf(line, "%9s-%9s %4s %9s %9s %9s %[^\n]", 
                start, end, perms, offset, dev, inode, path) == 7) {
            printf("%-15s %s %s %s %s %s %s\n", start, end, perms, offset, dev, inode, path);
        }
    }
    fclose(f);
}

// Функция для получения метрик VSZ и RSS
void get_memory_metrics(char *buffer, size_t size) {
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) {
        perror("fopen /proc/self/status");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "VmSize:") || strstr(line, "VmRSS:")) {
            strncat(buffer, line, size - strlen(buffer) - 1);
        }
    }
    fclose(f);
}

// Функция для получения продвинутых метрик PSS и USS
void get_advanced_metrics(char *buffer, size_t size) {
    FILE *f = fopen("/proc/self/smaps_rollup", "r");
    if (!f) {
        perror("fopen /proc/self/smaps_rollup");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "Pss:") || strstr(line, "Private:")) {
            strncat(buffer, line, size - strlen(buffer) - 1);
        }
    }
    fclose(f);
}

int main() {
    // 1. Выделяем память разными способами
    
    // Стек (1 KB)
    char stack_var[1024];
    memset(stack_var, 'A', sizeof(stack_var));
    
    // Куча (1 MB)
    char *heap_var = malloc(1024 * 1024);
    if (!heap_var) {
        perror("malloc");
        return 1;
    }
    memset(heap_var, 'B', 1024 * 1024);
    
    // Anonymous mmap (1 MB)
    char *mmap_var = mmap(NULL, 1024 * 1024, 
                         PROT_READ | PROT_WRITE, 
                         MAP_PRIVATE | MAP_ANONYMOUS, 
                         -1, 0);
    if (mmap_var == MAP_FAILED) {
        perror("mmap");
        free(heap_var);
        return 1;
    }
    memset(mmap_var, 'C', 1024 * 1024);

    // 2. Выводим информацию о памяти
    printf("=== Memory Analysis ===\n");
    printf("Stack variable (1 KB): %p\n", (void *)stack_var);
    printf("Heap variable (1 MB): %p\n", (void *)heap_var);
    printf("Mmap variable (1 MB): %p\n", (void *)mmap_var);
    
    // Печатаем карту памяти
    print_memory_map();
    
    // Получаем базовые метрики
    char metrics[1024] = {0};
    get_memory_metrics(metrics, sizeof(metrics));
    printf("\n=== Basic Memory Metrics ===\n");
    printf("%s", metrics);
    
    // Получаем продвинутые метрики
    char advanced_metrics[512] = {0};
    get_advanced_metrics(advanced_metrics, sizeof(advanced_metrics));
    printf("\n=== Advanced Memory Metrics ===\n");
    printf("%s", advanced_metrics);
    
    // 3. Освобождаем память
    free(heap_var);
    munmap(mmap_var, 1024 * 1024);
    
    // Дополнительный вывод для анализа
    printf("\n=== After memory release ===\n");
    char metrics_after[1024] = {0};
    get_memory_metrics(metrics_after, sizeof(metrics_after));
    printf("%s", metrics_after);
    printf("Waiting for 30 seconds to allow analysis...\n");
    sleep(30); // Программа будет ждать 30 секунд    
    return 0;
}
