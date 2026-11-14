// ЛР №2. Поиск кратчайшего замкнутого маршрута алгоритмом Held–Karp.
#include <chrono>
#include <cmath>
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

// Из строки извлекаются числа; в форматах с номером/подписью берутся последние два.
vector<Point> readPoints(istream& input) {
    vector<Point> points;
    string line;
    while (getline(input, line)) {
        const auto first = line.find_first_not_of(" \t\r");
        if (first == string::npos || line[first] == '#') continue;
        for (char& ch : line) if (ch == ',' || ch == ';') ch = ' ';
        istringstream stream(line);
        vector<double> numbers;
        string token;
        while (stream >> token) {
            char* end = nullptr;
            const double value = strtod(token.c_str(), &end);
            if (end != token.c_str() && *end == '\0') numbers.push_back(value);
        }
        if (numbers.size() < 2) {
            throw invalid_argument("Некорректная строка с координатами: " + line);
        }
        const double x = numbers[numbers.size() - 2];
        const double y = numbers[numbers.size() - 1];
        if (!isfinite(x) || !isfinite(y)) throw invalid_argument("Координаты должны быть конечными числами");
        points.push_back({x, y});
    }
    return points;
}

int main(int argc, char* argv[]) {
    try {
        vector<Point> points;
        if (argc == 3 && string(argv[1]) == "--random") {
            const int n = atoi(argv[2]);
            if (n < 1 || n > MAX_POINTS) throw invalid_argument("Для --random число точек должно быть от 1 до 18");
            mt19937 generator(42);
            uniform_real_distribution<double> coordinate(0.0, 100.0);
            for (int i = 0; i < n; ++i) points.push_back({coordinate(generator), coordinate(generator)});
        } else if (argc == 2) {
            ifstream file(argv[1]);
            if (!file) throw runtime_error("Не удалось открыть файл: " + string(argv[1]));
            points = readPoints(file);
        } else if (argc == 1) {
            points = readPoints(cin);
        } else {
            cerr << "Использование: ./tsp [файл] | ./tsp --random N\n";
            return 2;
        }

        if (points.empty()) throw invalid_argument("Не задано ни одной точки");
        if (points.size() > MAX_POINTS) throw invalid_argument("Слишком много точек: максимум 18");

        cout << fixed << setprecision(3);
        cout << "Количество точек: " << points.size() << '\n';
        if (points.size() <= 12) {
            for (size_t i = 0; i < points.size(); ++i)
                cout << "  Точка " << i + 1 << ": (" << points[i].x << ", " << points[i].y << ")\n";
        }

        const auto started = chrono::steady_clock::now();
        const TspResult result = solveTsp(points);
        const auto finished = chrono::steady_clock::now();
        const double milliseconds = chrono::duration<double, milli>(finished - started).count();

        cout << "Оптимальный маршрут: ";
        for (size_t i = 0; i < result.route.size(); ++i) {
            if (i) cout << " -> ";
            cout << result.route[i] + 1;
        }
        cout << "\nДлина маршрута: " << result.length
             << "\nВремя вычисления: " << milliseconds << " мс\n";
    } catch (const exception& error) {
        cerr << "Ошибка: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
