// ЛР №2. Задача коммивояжёра: алгоритм Дейкстры на графе состояний
//
// Как это работает. Обычный алгоритм Дейкстры ищет кратчайший путь между двумя
// вершинами графа, но "пройти через все точки по одному разу" он сам не умеет.
// Поэтому строим вспомогательный граф СОСТОЯНИЙ.
//
//   Состояние = (mask, last)
//     mask - какие точки уже посещены (битовая маска, бит k = 1, если точка k посещена)
//     last - в какой точке мы сейчас стоим
//
//   Из состояния (mask, last) можно пойти в любую непосещённую точку k
//   и получить состояние (mask | бит k, k) со стоимостью ребра = расстояние(last, k).
//
//   Начало: (только точка 0, точка 0). Конец: (все точки, точка 0), то есть мы
//   посетили все точки и вернулись домой. Кратчайший путь от начала до конца
//   в этом графе и есть оптимальный маршрут коммивояжёра.
#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

// Точка на плоскости.
struct Point {
    double x, y;
};

// Результат: длина маршрута и порядок обхода (номера точек от 0, начинается и заканчивается 0).
struct TspResult {
    double length;
    std::vector<int> route;
};

// Ограничение на число точек: состояний 2^n * n, а их надо хранить в памяти.
const int MAX_POINTS = 15;

// Расстояние между двумя точками.
inline double distance(const Point& a, const Point& b) {
    return std::hypot(a.x - b.x, a.y - b.y);
}

inline TspResult solveTsp(const std::vector<Point>& pts) {
    const int n = static_cast<int>(pts.size());
    if (n == 0) throw std::invalid_argument("Нет точек");
    if (n > MAX_POINTS) throw std::invalid_argument("Слишком много точек");
    if (n == 1) return {0.0, {0, 0}};     // одна точка: маршрут нулевой длины

    const int full = (1 << n) - 1;                       // маска "посещены все точки"
    const double INF = std::numeric_limits<double>::infinity();

    // Состояние (mask, last) хранится под одним номером: mask * n + last.
    // dist   - лучшее найденное расстояние до состояния
    // parent - из какого состояния пришли (нужно для восстановления пути)
    const size_t states = static_cast<size_t>(1 << n) * n;
    std::vector<double> dist(states, INF);
    std::vector<int> parent(states, -1);

    const int start  = 1 * n + 0;        // посещена только точка 0, стоим в точке 0
    const int target = full * n + 0;     // посещены все, вернулись в точку 0

    // Очередь с приоритетом: сверху всегда состояние с наименьшим расстоянием.
    using Item = std::pair<double, int>;   // (расстояние, номер состояния)
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> queue;

    dist[start] = 0;
    queue.push({0.0, start});

    // Если нашли путь короче известного, запоминаем его и кладём состояние в очередь.
    auto relax = [&](int from, int to, double newDist) {
        if (newDist < dist[to]) {
            dist[to] = newDist;
            parent[to] = from;
            queue.push({newDist, to});
        }
    };

    while (!queue.empty()) {
        auto [d, s] = queue.top();
        queue.pop();

        if (d > dist[s]) continue;     // устаревшая запись: путь до s уже улучшили
        if (s == target) break;        // дошли до конца - это кратчайший путь

        int mask = s / n;
        int last = s % n;

        if (mask == full) {
            // Все точки посещены: возвращаемся в точку 0
            relax(s, target, d + distance(pts[last], pts[0]));
            continue;
        }
        // Иначе идём в любую ещё не посещённую точку k
        for (int k = 0; k < n; k++) {
            if (mask & (1 << k)) continue;
            relax(s, (mask | (1 << k)) * n + k, d + distance(pts[last], pts[k]));
        }
    }

    // Восстанавливаем маршрут: идём от конца к началу по parent
    TspResult res;
    res.length = dist[target];
    for (int s = target; s != -1; s = parent[s]) {
        res.route.push_back(s % n);
    }
    std::reverse(res.route.begin(), res.route.end());
    return res;
}

// Полный перебор всех маршрутов (для проверки на малых n): (n-1)! вариантов.
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
