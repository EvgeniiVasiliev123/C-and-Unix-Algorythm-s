// ЛР №1. [СОРТ 1] Сортировка подсчётом
//
// Задача: отсортировать оценки студентов за экзамен (целые числа от 0 до 100)
// для большого потока (100 000 работ), чтобы построить итоговую ведомость.
//
#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

using namespace std;

// Сортировка подсчётом.
// maxValue - максимальное возможное значение 
void countingSort(vector<int>& a, int maxValue) {
    // 1. Массив счётчиков: co
    vector<int> count(maxValue + 1, 0);
    // 2.  считаем вхождения. O(n)
    for (int x : a) {
        count[x]++;
    }

    // 3. значение v записываем count[v] раз. O(n + k)
    int pos = 0;
    for (int v = 0; v <= maxValue; v++) {
        for (int j = 0; j < count[v]; j++) {
            a[pos++] = v;
        }
    }
}

int main() {
    const int N = 100000;      // количество оценок (n)
    const int MAX_SCORE = 100; // (k = 101)

    // Генерируем случайные оценки
    mt19937 gen(42);
    uniform_int_distribution<int> dist(0, MAX_SCORE);
    vector<int> scores(N);
    for (int& s : scores) s = dist(gen);

    // Сортируем и замеряем время
    auto start = chrono::high_resolution_clock::now();
    countingSort(scores, MAX_SCORE);
    auto end = chrono::high_resolution_clock::now();

    double ms = chrono::duration<double, milli>(end - start).count();

    // Проверка 
    bool ok = is_sorted(scores.begin(), scores.end());

    cout << "Сортировка подсчётом" << endl;
    cout << "n = " << N << ", диапазон = 0.." << MAX_SCORE << endl;
    cout << "Первые 10 элементов: ";
    for (int i = 0; i < 10; i++) cout << scores[i] << " ";
    cout << endl;
    cout << "Последние 10 элементов: ";
    for (int i = N - 10; i < N; i++) cout << scores[i] << " ";
    cout << endl;
    cout << "Массив отсортирован: " << (ok ? "да" : "НЕТ") << endl;
    cout << "Время: " << ms << " мс" << endl;
    return 0;
}
