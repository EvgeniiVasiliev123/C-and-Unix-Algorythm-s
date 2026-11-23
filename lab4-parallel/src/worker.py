#!/usr/bin/env python3
"""Обработчик группы одинаковых запросов для режима subprocess."""
import sys
import time


def main():
    action = sys.argv[1]
    first_id = int(sys.argv[2])
    count = int(sys.argv[3])
    duration = float(sys.argv[4])
    for request_id in range(first_id, first_id + count):
        time.sleep(duration)
        print(f"Запрос {request_id}: {action} — выполнен")


if __name__ == "__main__":
    main()
