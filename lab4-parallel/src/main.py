#!/usr/bin/env python3
"""Лабораторная работа №4: сравнение способов обработки запросов."""

import asyncio
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path


# Типы запросов и условная загрузка CPU из задания.
ACTIONS = [
    {"name": "Регистрация пользователя", "cpu": 25},
    {"name": "Главная страница", "cpu": 15},
    {"name": "Список пользователей", "cpu": 1},
]

WORKER_FILE = Path(__file__).with_name("worker.py")


def ask_number(message, default, number_type):
    """Считывает число. Если пользователь нажал Enter, берёт значение по умолчанию."""
    answer = input(f"{message} [{default}]: ").strip()
    if not answer:
        return default

    value = number_type(answer.replace(",", ".") if number_type is float else answer)
    if value < 0:
        raise ValueError("Значение не может быть отрицательным.")
    return value


def get_settings():
    """Запрашивает количество пользователей и время обработки запросов."""
    print("\nКоличество пользователей:")
    counts = [
        ask_number("Регистрация", 3, int),
        ask_number("Главная страница", 3, int),
        ask_number("Список пользователей", 3, int),
    ]

    print("\nВремя обработки одного запроса в секундах:")
    durations = [
        ask_number("Регистрация", 1.0, float),
        ask_number("Главная страница", 0.7, float),
        ask_number("Список пользователей", 0.3, float),
    ]
    return counts, durations


def create_requests(counts, durations):
    """Создаёт общий список запросов с понятными полями."""
    requests = []
    request_id = 1

    for action, count, duration in zip(ACTIONS, counts, durations):
        for _ in range(count):
            requests.append({
                "id": request_id,
                "name": action["name"],
                "duration": duration,
            })
            request_id += 1

    return requests


def show_result(request):
    print(f"Запрос {request['id']}: {request['name']} — выполнен")


def run_sequential(requests):
    """Выполняет запросы один за другим."""
    print("\nПоследовательная обработка:")
    for request in requests:
        time.sleep(request["duration"])
        show_result(request)


def run_worker(requests):
    """Функция одного потока: имитирует выполнение одного запроса."""
    time.sleep(requests["duration"])
    return f"Запрос {requests['id']}: {requests['name']} — выполнен"


def run_threads(requests):
    """Запускает запросы в пуле потоков."""
    print("\nОбработка с помощью threads:")
    with ThreadPoolExecutor(max_workers=min(32, len(requests))) as pool:
        for result in pool.map(run_worker, requests):
            print(result)


def run_subprocesses(requests):
    """Запускает по отдельному процессу для каждого типа запросов."""
    print("\nОбработка с помощью subprocess:")
    processes = []

    for action in ACTIONS:
        group = [request for request in requests if request["name"] == action["name"]]
        if not group:
            continue

        # Один дочерний процесс последовательно выполняет свою группу запросов.
        process = subprocess.Popen(
            [
                sys.executable,
                str(WORKER_FILE),
                action["name"],
                str(group[0]["id"]),
                str(len(group)),
                str(group[0]["duration"]),
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        processes.append(process)

    for process in processes:
        output, errors = process.communicate()
        if process.returncode != 0:
            raise RuntimeError(errors.strip() or "Ошибка дочернего процесса.")
        print(output, end="")


async def async_worker(request):
    """Асинхронно имитирует выполнение одного запроса."""
    await asyncio.sleep(request["duration"])
    return f"Запрос {request['id']}: {request['name']} — выполнен"


async def run_async(requests):
    """Запускает все запросы как асинхронные задачи."""
    print("\nОбработка с помощью asyncio:")
    results = await asyncio.gather(
        *(async_worker(request) for request in requests)
    )
    for result in results:
        print(result)


def measure_time(title, function):
    """Запускает функцию и возвращает время её выполнения."""
    print(f"\n--- {title} ---")
    start = time.perf_counter()
    function()
    elapsed = time.perf_counter() - start
    print(f"Время: {elapsed:.3f} сек.")
    return elapsed


def main():
    print("ЛАБОРАТОРНАЯ РАБОТА №4 — ПАРАЛЛЕЛЬНЫЕ АЛГОРИТМЫ")
    counts, durations = get_settings()
    requests = create_requests(counts, durations)

    if not requests:
        print("Запросов нет. Завершение программы.")
        return

    print("\nПараметры модели:")
    for action, count, duration in zip(ACTIONS, counts, durations):
        print(
            f"- {action['name']}: {count} пользователей, "
            f"{duration:.2f} сек., условная нагрузка CPU — {action['cpu']}%"
        )

    # Это имитация времени ожидания. Реальная загрузка CPU не измеряется.
    results = []
    results.append(("Последовательно", measure_time(
        "Последовательный режим", lambda: run_sequential(requests)
    )))
    results.append(("subprocess", measure_time(
        "Дочерние процессы", lambda: run_subprocesses(requests)
    )))
    results.append(("threads", measure_time(
        "Потоки", lambda: run_threads(requests)
    )))
    results.append(("asyncio", measure_time(
        "Асинхронный режим", lambda: asyncio.run(run_async(requests))
    )))

    print("\nСРАВНЕНИЕ ВРЕМЕНИ")
    for name, elapsed in results:
        print(f"{name:18} {elapsed:.3f} сек.")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, RuntimeError) as error:
        print(f"Ошибка: {error}", file=sys.stderr)
        sys.exit(1)
