// ЛР №2. Задача коммивояжёра - запуск программы.
//
// Использование:
//   ./tsp data/example.txt     - читать точки из файла
//   ./tsp --random 15          - сгенерировать 15 случайных точек
//   ./tsp                      - читать точки из стандартного ввода
//
// Формат входных данных: по одной точке в строке. Допустимы записи
//   X Y
//   номер X Y
//   Точка 1, X, Y   (запятые заменяются пробелами; слова игнорируются)
// Строки, начинающиеся с '#', считаются комментариями.
// Первая точка является начальной и конечной точкой маршрута.
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "tsp.hpp"

using namespace std;

// Читает точки из потока. Из каждой строки берутся все числа: два последних - X и Y.
vector<Point> readPoints(istream& in) {
    vector<Point> pts;
    string line;
    while (getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        for (char& c : line) if (c == ',' || c == ';') c = ' ';

        istringstream iss(line);
        vector<double> nums;
        string token;
        while (iss >> token) {
            char* end = nullptr;
            double v = strtod(token.c_str(), &end);
            if (*end == '\0') nums.push_back(v);   // токен - число
        }
        if (nums.size() >= 2) {
            pts.push_back({nums[nums.size() - 2], nums[nums.size() - 1]});
        }
    }
    return pts;
}

int main(int argc, char* argv[]) {
    vector<Point> pts;

    if (argc >= 3 && string(argv[1]) == "--random") {
        int n = atoi(argv[2]);
        mt19937 gen(42);
        uniform_real_distribution<double> coord(0, 100);
        for (int i = 0; i < n; i++) pts.push_back({coord(gen), coord(gen)});
    } else if (argc >= 2) {
        ifstream f(argv[1]);
        if (!f) {
            cerr << "Не удалось открыть файл " << argv[1] << endl;
            return 1;
        }
        pts = readPoints(f);
    } else {
        pts = readPoints(cin);
    }

    if (pts.empty()) {
        cerr << "Не задано ни одной точки" << endl;
        return 1;
    }
    if (pts.size() > MAX_POINTS) {
        cerr << "Слишком много точек (" << pts.size() << "), максимум " << MAX_POINTS << endl;
        return 1;
    }

    cout << fixed << setprecision(3);
    cout << "Точек: " << pts.size() << endl;
    if (pts.size() <= 12) {
        for (size_t i = 0; i < pts.size(); i++)
            cout << "  Точка " << i + 1 << ": (" << pts[i].x << ", " << pts[i].y << ")" << endl;
    }

    auto start = chrono::high_resolution_clock::now();
    TspResult res = solveTsp(pts);
    auto end = chrono::high_resolution_clock::now();
    double ms = chrono::duration<double, milli>(end - start).count();

    cout << "Оптимальный маршрут: ";
    for (size_t i = 0; i < res.route.size(); i++) {
        cout << res.route[i] + 1;
        if (i + 1 < res.route.size()) cout << " -> ";
    }
    cout << endl;
    cout << "Длина маршрута: " << res.length << endl;
    cout << "Таблица ДП: " << dpTableBytes(pts.size()) / 1024.0 << " КБ" << endl;
    cout << "Время: " << ms << " мс" << endl;
    return 0;
}
