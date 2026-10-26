// ЛР №2. Задача коммивояжёра: динамическое программирование (алгоритм Хелда-Карпа)
//
// Требуется найти кратчайший замкнутый маршрут, который начинается в первой
// точке, проходит через каждую из остальных точек ровно один раз и возвращается
// в первую точку.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

// ---------------------------------------------------------------------------
// Структуры данных
// ---------------------------------------------------------------------------

// Точка на плоскости.
struct Point {
    double x, y;
};

// Результат решения.
struct TspResult {
    double length;          // длина оптимального маршрута
    std::vector<int> route; // порядок обхода: индексы точек 0..n-1, начинается и заканчивается 0
};

// Ограничение на число точек: таблица ДП занимает (n-1)*2^(n-1)*4 байт.
// Для n = 22 это около 176 МБ.
const int MAX_POINTS = 22;

// Евклидово расстояние между двумя точками.
inline double distance(const Point& a, const Point& b) {
    return std::hypot(a.x - b.x, a.y - b.y);
}

// Длина замкнутого маршрута (для проверки результата), считается в double.
inline double routeLength(const std::vector<Point>& pts, const std::vector<int>& route) {
    double len = 0;
    for (size_t i = 0; i + 1 < route.size(); i++) {
        len += distance(pts[route[i]], pts[route[i + 1]]);
    }
    return len;
}

// Сколько байт займёт таблица ДП для n точек (для отчёта).
inline size_t dpTableBytes(int n) {
    if (n < 2) return 0;
    size_t m = n - 1;
    return (size_t(1) << m) * m * sizeof(float);
}

// ---------------------------------------------------------------------------
// Алгоритм Хелда-Карпа
// ---------------------------------------------------------------------------
//
// Точка 0 - начало и конец маршрута. Остальные m = n-1 точек (с номерами
// 1..m) кодируем номерами 0..m-1 и множество посещённых точек храним
// как битовую маску: бит j равен 1, если точка j+1 уже посещена.
//
// dp[mask][j] - длина кратчайшего пути, который начинается в точке 0,
//               посещает ровно точки из mask и заканчивается в точке j+1
//               (j обязательно входит в mask).
//
// Формула:
//   dp[{j}][j]    = d(0, j+1)
//   dp[mask][j]   = min по i из mask\{j} от ( dp[mask\{j}][i] + d(i+1, j+1) )
//   ответ         = min по j от ( dp[полная маска][j] + d(j+1, 0) )
inline TspResult solveTsp(const std::vector<Point>& pts) {
    const int n = static_cast<int>(pts.size());
    if (n == 0) throw std::invalid_argument("Нет точек");
    if (n > MAX_POINTS) throw std::invalid_argument("Слишком много точек для ДП");

    // Тривиальные случаи
    if (n == 1) return {0.0, {0, 0}};
    if (n == 2) return {2 * distance(pts[0], pts[1]), {0, 1, 0}};

    const int m = n - 1;                       // число точек, кроме стартовой
    const uint32_t full = (1u << m) - 1;       // маска "посещены все"

    // Матрица расстояний: n*n чисел float в одном плоском массиве.
    // Это всего несколько сотен байт, по сравнению с таблицей ДП - мелочь.
    std::vector<float> d(n * n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            d[i * n + j] = static_cast<float>(distance(pts[i], pts[j]));

    // Таблица ДП: один плоский массив float размером 2^m * m.
    // Элемент [mask][j] лежит по индексу mask * m + j.
    std::vector<float> dp((size_t(1) << m) * m);

    // Перебираем маски по возрастанию: подмножество (mask без одного бита)
    // всегда численно меньше mask, поэтому к моменту обработки mask
    // все нужные значения уже вычислены.
    for (uint32_t mask = 1; mask <= full; mask++) {
        for (int j = 0; j < m; j++) {
            if (!(mask & (1u << j))) continue;       // j должна входить в mask

            uint32_t prev = mask ^ (1u << j);        // множество без точки j
            if (prev == 0) {
                // Путь из одной точки: 0 -> j+1
                dp[size_t(mask) * m + j] = d[0 * n + (j + 1)];
                continue;
            }
            // Выбираем, из какой точки i мы пришли в j
            float best = std::numeric_limits<float>::infinity();
            for (int i = 0; i < m; i++) {
                if (!(prev & (1u << i))) continue;
                float cand = dp[size_t(prev) * m + i] + d[(i + 1) * n + (j + 1)];
                if (cand < best) best = cand;
            }
            dp[size_t(mask) * m + j] = best;
        }
    }

    // Замыкаем маршрут: выбираем последнюю точку перед возвратом в 0
    float bestLen = std::numeric_limits<float>::infinity();
    int last = 0;
    for (int j = 0; j < m; j++) {
        float cand = dp[size_t(full) * m + j] + d[(j + 1) * n + 0];
        if (cand < bestLen) { bestLen = cand; last = j; }
    }

    // Восстановление маршрута БЕЗ отдельной таблицы "родителей":
    // идём с конца и снова выбираем i, дающее минимум. Формулы те же,
    // поэтому результат совпадает с тем, что был получен при заполнении.
    std::vector<int> order;                    // точки с конца к началу
    uint32_t mask = full;
    int j = last;
    while (true) {
        order.push_back(j + 1);
        uint32_t prev = mask ^ (1u << j);
        if (prev == 0) break;
        float best = std::numeric_limits<float>::infinity();
        int bestI = -1;
        for (int i = 0; i < m; i++) {
            if (!(prev & (1u << i))) continue;
            float cand = dp[size_t(prev) * m + i] + d[(i + 1) * n + (j + 1)];
            if (cand < best) { best = cand; bestI = i; }
        }
        mask = prev;
        j = bestI;
    }
    std::reverse(order.begin(), order.end());

    TspResult res;
    res.route.push_back(0);
    for (int p : order) res.route.push_back(p);
    res.route.push_back(0);
    res.length = routeLength(pts, res.route);   // точная длина в double
    return res;
}

// ---------------------------------------------------------------------------
// Полный перебор (для проверки на малых n): (n-1)! маршрутов
// ---------------------------------------------------------------------------
inline double bruteForceLength(const std::vector<Point>& pts) {
    const int n = static_cast<int>(pts.size());
    if (n <= 1) return 0.0;
    std::vector<int> perm;
    for (int i = 1; i < n; i++) perm.push_back(i);
    double best = std::numeric_limits<double>::infinity();
    do {
        double len = distance(pts[0], pts[perm[0]]);
        for (size_t i = 0; i + 1 < perm.size(); i++)
            len += distance(pts[perm[i]], pts[perm[i + 1]]);
        len += distance(pts[perm.back()], pts[0]);
        best = std::min(best, len);
    } while (std::next_permutation(perm.begin(), perm.end()));
    return best;
}
