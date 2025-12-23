#!/bin/bash


# Запуск программы в фоновом режиме

echo "Starting memory_info in the background..."
./memory_info &
PID=$!


# Ждём пару секунд, чтобы программа успела выделить память

sleep 2


# Проверяем, что процесс запущен

if ps -p $PID > /dev/null; then
    echo "Collecting data for PID: $PID"

else
    echo "Error: Process did not start correctly."
    exit 1

fi


# Сбор базовых метрик (VSZ, RSS)

echo "=== Basic Memory Metrics ==="
ps -o pid,comm,vsz,rss -p $PID


# Сбор подробных метрик из /proc/[PID]/status

echo "=== Detailed Memory Metrics (/proc/[PID]/status) ==="

cat /proc/$PID/status | grep -E "^Vm"


# Сбор продвинутых метрик (PSS, USS)

echo "=== Advanced Memory Metrics (/proc/[PID]/smaps_rollup) ==="

cat /proc/$PID/smaps_rollup | grep -E "Pss|Private"


# Сбор карты памяти

echo "=== Memory Map (/proc/[PID]/maps) ==="

cat /proc/$PID/maps


# Остановка программы

echo "Stopping the program..."

kill $PID


echo "Data collection complete."
