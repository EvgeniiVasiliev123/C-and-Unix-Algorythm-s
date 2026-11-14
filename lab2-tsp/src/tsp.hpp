// ЛР №2. Задача коммивояжёра: точное динамическое программирование Held–Karp.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

struct Point {
    double x, y;
};

struct TspResult {
    double length;
    // Номера точек с нуля; первая точка (0) указана в начале и в конце.
    std::vector<int> route;
};

// Ограничение выбрано с учётом O(n * 2^n) ячеек DP и памяти программы.
constexpr int MAX_POINTS = 18;

inline double euclideanDistance(const Point& a, const Point& b) {
    return std::hypot(a.x - b.x, a.y - b.y);
}

inline TspResult solveTsp(const std::vector<Point>& points) {
    const int n = static_cast<int>(points.size());
    if (n == 0) throw std::invalid_argument("Нет точек");
    if (n > MAX_POINTS) throw std::invalid_argument("Слишком много точек (максимум 18)");
    if (n == 1) return {0.0, {0, 0}};

    // Матрица расстояний исключает повторное вычисление hypot в переходах DP.
    std::vector<double> cost(static_cast<std::size_t>(n) * n, 0.0);
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            cost[static_cast<std::size_t>(i) * n + j] = euclideanDistance(points[i], points[j]);

    // dp[mask][last] — минимальная длина пути из 0 в last,
    // посетив ровно вершины mask (включая 0).
    const std::size_t maskCount = std::size_t{1} << n;
    const std::size_t cellCount = maskCount * static_cast<std::size_t>(n);
    const double INF = std::numeric_limits<double>::infinity();
    std::vector<double> dp(cellCount, INF);
    // Для восстановления маршрута достаточно хранить предыдущую вершину (0..17).
    std::vector<std::int8_t> parent(cellCount, -1);
    const auto index = [n](std::size_t mask, int last) {
        return mask * static_cast<std::size_t>(n) + static_cast<std::size_t>(last);
    };

    dp[index(1, 0)] = 0.0;

    //1. Программа перебирает наборы уже посещённых точек.
    //2. Определяет последнюю точку текущего маршрута.
    //3. Рассматривает следующую ещё не посещённую точку.
    //4. Вычисляет длину нового пути.
    //5. Если новый путь короче найденного ранее, обновляет результат.
    for (std::size_t mask = 1; mask < maskCount; ++mask) {
        if ((mask & 1U) == 0) continue; // стартовая точка всегда входит в множество
        for (int last = 0; last < n; ++last) {
            const std::size_t current = index(mask, last);
            if ((mask & (std::size_t{1} << last)) == 0 || !std::isfinite(dp[current])) continue;
            for (int next = 1; next < n; ++next) {
                const std::size_t bit = std::size_t{1} << next;
                if (mask & bit) continue;
                const std::size_t nextMask = mask | bit;
                const std::size_t destination = index(nextMask, next);
                const double candidate = dp[current] + cost[static_cast<std::size_t>(last) * n + next];
                if (candidate < dp[destination]) {
                    dp[destination] = candidate;
                    parent[destination] = static_cast<std::int8_t>(last);
                }
            }
        }
    }

    const std::size_t full = maskCount - 1;
    double best = INF;
    int lastBest = -1;
    for (int last = 1; last < n; ++last) {
        const double candidate = dp[index(full, last)] + cost[static_cast<std::size_t>(last) * n];
        if (candidate < best) {
            best = candidate;
            lastBest = last;
        }
    }
    if (lastBest < 0) throw std::runtime_error("Не удалось построить маршрут");

    // Восстанавливаем открытый путь от lastBest к 0 по массиву parent.
    std::vector<int> reversed;
    std::size_t mask = full;
    int last = lastBest;
    while (last != 0) {
        reversed.push_back(last);
        const int previous = parent[index(mask, last)];
        if (previous < 0) throw std::runtime_error("Ошибка восстановления маршрута");
        mask ^= (std::size_t{1} << last);
        last = previous;
    }
    reversed.push_back(0);
    std::reverse(reversed.begin(), reversed.end());
    reversed.push_back(0);
    return {best, reversed};
}

// Эталон для тестирования на небольшом количестве точек: полный перебор (n-1)!.
inline double bruteForceLength(const std::vector<Point>& points) {
    const int n = static_cast<int>(points.size());
    if (n <= 1) return 0.0;
    std::vector<int> order;
    for (int i = 1; i < n; ++i) order.push_back(i);
    double best = std::numeric_limits<double>::infinity();
    do {
        double length = euclideanDistance(points[0], points[order[0]]);
        for (std::size_t i = 0; i + 1 < order.size(); ++i)
            length += euclideanDistance(points[order[i]], points[order[i + 1]]);
        length += euclideanDistance(points[order.back()], points[0]);
        best = std::min(best, length);
    } while (std::next_permutation(order.begin(), order.end()));
    return best;
}
