// Проверка Held–Karp сравнением с полным перебором на малых наборах.
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>
#include "tsp.hpp"

using namespace std;

void checkCase(const vector<Point>& points, int& checked) {
    const TspResult result = solveTsp(points);
    const int n = static_cast<int>(points.size());
    bool valid = result.route.size() == static_cast<size_t>(n + 1)
              && result.route.front() == 0 && result.route.back() == 0;
    vector<int> visits(n, 0);
    for (size_t i = 0; valid && i + 1 < result.route.size(); ++i) {
        const int vertex = result.route[i];
        if (vertex < 0 || vertex >= n) valid = false;
        else ++visits[vertex];
    }
    for (int count : visits) if (count != 1) valid = false;
    const double brute = bruteForceLength(points);
    if (abs(result.length - brute) > 1e-9 || !valid) {
        cerr << "FAIL: n=" << n << ", DP=" << result.length << ", brute=" << brute << '\n';
        throw runtime_error("Проверка алгоритма не пройдена");
    }
    ++checked;
}

int main() {
    try {
        int checked = 0;
        checkCase({{0, 0}}, checked);
        checkCase({{0, 0}, {3, 4}}, checked);
        checkCase({{0, 0}, {1, 0}, {2, 0}, {3, 0}}, checked);
        checkCase({{-2, 5}, {0, 0}, {4, -1}, {4, 4}, {-3, -2}}, checked);
        checkCase({{1, 1}, {1, 1}, {2, 2}, {2, 2}}, checked);

        mt19937 generator(123);
        uniform_int_distribution<int> coordinate(-100, 100);
        for (int n = 1; n <= 9; ++n) {
            for (int trial = 0; trial < 200; ++trial) {
                vector<Point> points;
                for (int i = 0; i < n; ++i)
                    points.push_back({static_cast<double>(coordinate(generator)),
                                      static_cast<double>(coordinate(generator))});
                checkCase(points, checked);
            }
        }
        cout << "Все проверки пройдены: " << checked
             << " тестов (граничные случаи и 1800 случайных наборов, n=1..9).\n";
    } catch (const exception& error) {
        cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
