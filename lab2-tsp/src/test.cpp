// ЛР №2. Проверка: алгоритм Дейкстры сравнивается с полным перебором на случайных данных.
#include <iostream>
#include <random>
#include <vector>

#include "tsp.hpp"

using namespace std;

int main() {
    mt19937 gen(123);
    uniform_int_distribution<int> coord(0, 100);
    int checked = 0;

    for (int n = 1; n <= 9; n++) {
        for (int trial = 0; trial < 200; trial++) {
            vector<Point> pts;
            for (int i = 0; i < n; i++) pts.push_back({double(coord(gen)), double(coord(gen))});

            TspResult res = solveTsp(pts);
            double brute = bruteForceLength(pts);

            // 1. Маршрут начинается и заканчивается в точке 0 и посещает каждую точку один раз
            bool ok = res.route.size() == size_t(n) + 1 && res.route.front() == 0 && res.route.back() == 0;
            vector<int> seen(n, 0);
            for (size_t i = 0; ok && i + 1 < res.route.size(); i++) seen[res.route[i]]++;
            for (int i = 0; i < n; i++) if (seen[i] != 1) ok = false;

            // 2. Длина совпадает с перебором
            if (abs(res.length - brute) > 1e-9) ok = false;

            if (!ok) {
                cout << "ОШИБКА: n = " << n << ", попытка " << trial
                     << ", Дейкстра = " << res.length << ", перебор = " << brute << endl;
                return 1;
            }
            checked++;
        }
    }
    cout << "Все проверки пройдены: " << checked << " случайных тестов (n = 1..9)." << endl;
    return 0;
}
