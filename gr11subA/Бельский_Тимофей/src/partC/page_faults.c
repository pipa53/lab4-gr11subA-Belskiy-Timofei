#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <sys/resource.h>

#include <time.h>

#include <unistd.h>

// Функция для вывода количества page faults
void print_page_faults() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    printf("Minor faults: %ld, Major faults: %ld\n", 
           usage.ru_minflt, usage.ru_majflt);
}

int main() {
    size_t size = 100 * 1024 * 1024; // 100 MB
    char *arr = malloc(size);
    if (!arr) {
        perror("malloc");
        return 1;
    }

    printf("=== Page Fault Analysis ===\n");

    // Выводим начальное количество page faults
    printf("Initial state:\n");
    print_page_faults();

    // Последовательное заполнение (по страницам)
    printf("\nFilling array sequentially (by pages):\n");
    for (size_t i = 0; i < size; i += 4096) { // По одной странице (4 KB)
        arr[i] = 'A';
    }
    print_page_faults();

    // Случайное заполнение
    printf("\nFilling array randomly (10000 random accesses):\n");
    for (int i = 0; i < 10000; i++) {
        size_t index = rand() % size;
        arr[index] = 'B';
    }
    print_page_faults();

    // Освобождаем память
    free(arr);

    return 0;
}
