#!/usr/bin/env python3
"""Дочерний процесс: последовательно выполняет группу однотипных запросов."""

import sys
import time


def main():
    action_name = sys.argv[1]
    first_request_id = int(sys.argv[2])
    request_count = int(sys.argv[3])
    duration = float(sys.argv[4])

    for request_id in range(first_request_id, first_request_id + request_count):
        time.sleep(duration)
        print(f"Запрос {request_id}: {action_name} — выполнен")


