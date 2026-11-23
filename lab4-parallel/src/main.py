#!/usr/bin/env python3
"""ЛР №4. Сравнение последовательной и параллельной обработки запросов."""
import asyncio
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ACTIONS = {
    1: ("Регистрация пользователя", 0.25),
    2: ("Получение главной страницы", 0.15),
    3: ("Просмотр списка пользователей", 0.01),
}
WORKER = Path(__file__).with_name("worker.py")


def read_int(prompt, default):
    value = input(f"{prompt} [{default}]: ").strip()
    return int(value) if value else default


def read_float(prompt, default):
    value = input(f"{prompt} [{default}]: ").strip().replace(",", ".")
    return float(value) if value else default


def make_requests(counts, times):
    requests = []
    request_id = 1
    for action_id in (1, 2, 3):
        name, cpu = ACTIONS[action_id]
        for _ in range(counts[action_id - 1]):
            requests.append((request_id, name, times[action_id - 1], cpu))
            request_id += 1
    return requests


def sequential(requests):
    print("\nПоследовательная обработка:")
    for request_id, name, duration, _ in requests:
        time.sleep(duration)
        print(f"Запрос {request_id}: {name} — выполнен")


def subprocess_mode(requests):
    print("\nПараллельная обработка (subprocess):")
    # Три процесса обрабатывают три вида действий одновременно.
    processes = []
    first_id = 1
    for action_id in (1, 2, 3):
        name, _ = ACTIONS[action_id]
        group = [r for r in requests if r[1] == name]
        if group:
            processes.append(subprocess.Popen(
                [sys.executable, str(WORKER), name, str(first_id),
                 str(len(group)), str(group[0][2])],
                stdout=subprocess.PIPE, text=True
            ))
        first_id += len(group)
    for process in processes:
        output, _ = process.communicate()
        print(output.strip())


def thread_task(request):
    request_id, name, duration, _ = request
    time.sleep(duration)
    return f"Запрос {request_id}: {name} — выполнен"


def threads_mode(requests):
    print("\nПараллельная обработка (threads):")
    with ThreadPoolExecutor(max_workers=max(1, min(32, len(requests)))) as executor:
        for result in executor.map(thread_task, requests):
            print(result)


async def async_task(request):
    request_id, name, duration, _ = request
    await asyncio.sleep(duration)
    return f"Запрос {request_id}: {name} — выполнен"


async def asyncio_mode(requests):
    print("\nПараллельная обработка (asyncio):")
    results = await asyncio.gather(*(async_task(request) for request in requests))
    for result in results:
        print(result)


def measure(function):
    start = time.perf_counter()
    function()
    elapsed = time.perf_counter() - start
    print(f"Время выполнения: {elapsed:.3f} сек.")
    return elapsed


def main():
    print("МОДЕЛИРОВАНИЕ НАГРУЗКИ СЕРВЕРА")
    print("Введите количество пользователей для каждого действия.")
    counts = [read_int("U1 — регистрация", 3),
              read_int("U2 — главная страница", 3),
              read_int("U3 — список пользователей", 3)]
    if any(value < 0 for value in counts):
        raise ValueError("Количество пользователей не может быть отрицательным")

    print("\nВведите время обработки одного запроса в секундах.")
    times = [read_float("T1 — регистрация", 1.0),
             read_float("T2 — главная страница", 0.7),
             read_float("T3 — список пользователей", 0.3)]
    if any(value < 0 for value in times):
        raise ValueError("Время обработки не может быть отрицательным")

    requests = make_requests(counts, times)
    if not requests:
        print("Нет запросов для обработки.")
        return

    print("\nИсходные параметры:")
    for i, action_id in enumerate((1, 2, 3)):
        name, cpu = ACTIONS[action_id]
        print(f"{name}: пользователей = {counts[i]}, T = {times[i]:.2f} сек., CPU ≈ {cpu * 100:.0f}%")

    results = []
    results.append(("Последовательно", measure(lambda: sequential(requests))))
    results.append(("subprocess", measure(lambda: subprocess_mode(requests))))
    results.append(("threads", measure(lambda: threads_mode(requests))))
    results.append(("asyncio", measure(lambda: asyncio.run(asyncio_mode(requests)))))

    print("\nСРАВНЕНИЕ ВРЕМЕНИ")
    for name, elapsed in results:
        print(f"{name:18} {elapsed:.3f} сек.")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError) as error:
        print(f"Ошибка: {error}", file=sys.stderr)
        sys.exit(1)
