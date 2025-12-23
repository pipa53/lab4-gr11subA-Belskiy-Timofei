#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <sys/mman.h>

#include <sys/stat.h>

#include <fcntl.h>

#include <unistd.h>

#include <time.h>

// Функция для очистки page cache (требует sudo)
void clear_page_cache() {
    int result = system("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'");
    if (result == -1) {
        perror("Ошибка при выполнении system()");
    } else if (WIFEXITED(result)) {
        int exit_status = WEXITSTATUS(result);
        if (exit_status != 0) {
            fprintf(stderr, "Команда завершилась с кодом %d\n", exit_status);
        }
    }
}

// Функция для чтения файла через системные вызовы (read)
unsigned long read_with_syscalls(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 0;
    }

    struct stat sb;
    fstat(fd, &sb);
    unsigned long total = 0;
    char buffer[4096];
    ssize_t bytes_read;

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            total += (unsigned char)buffer[i];
        }
    }

    close(fd);
    return total;
}

// Функция для чтения файла через mmap
unsigned long read_with_mmap(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 0;
    }

    struct stat sb;
    fstat(fd, &sb);

    char *mapped = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 0;
    }

    unsigned long total = 0;
    for (off_t i = 0; i < sb.st_size; i++) {
        total += (unsigned char)mapped[i];
    }

    munmap(mapped, sb.st_size);
    close(fd);
    return total;
}

int main() {
    const char *filename = "testfile.bin";

    // Очистка page cache перед каждым запуском
    clear_page_cache();

    // Замер времени для read_with_syscalls
    clock_t start = clock();
    unsigned long sum1 = read_with_syscalls(filename);
    clock_t end = clock();
    double time1 = (double)(end - start) / CLOCKS_PER_SEC;

    // Очистка page cache
    clear_page_cache();

    // Замер времени для read_with_mmap
    start = clock();
    unsigned long sum2 = read_with_mmap(filename);
    end = clock();
    double time2 = (double)(end - start) / CLOCKS_PER_SEC;

    // Вывод результатов
    printf("=== Performance Comparison ===\n");
    printf("File: %s\n", filename);
    printf("Sum (syscalls): %lu\n", sum1);
    printf("Sum (mmap): %lu\n", sum2);
    printf("\nExecution Time:\n");
    printf("  syscalls: %.3f seconds\n", time1);
    printf("  mmap:     %.3f seconds\n", time2);

    // Анализ page faults
    printf("\n=== Page Fault Analysis ===\n");

    // Получаем PID текущего процесса
    pid_t pid = getpid();

    // Читаем статистику page faults из /proc/[PID]/stat
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen /proc/[PID]/stat");
        return 1;
    }

    char line[256];
    if (fgets(line, sizeof(line), f)) {
        // Парсим поля min_flt и maj_flt
        long min_flt, maj_flt;
        sscanf(line, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %ld %ld", &min_flt, &maj_flt);
        printf("Minor page faults: %ld\n", min_flt);
        printf("Major page faults: %ld\n", maj_flt);
    }
    fclose(f);

    return 0;
}
