// Сортировка вставкой
//Массив остаётся почти отсортированным,
// нужно быстро привести его в порядок "на месте", без доп. памяти.
#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace std;

// Счётчики операций (нужны для отчёта)
struct Stats {
    long long comparisons = 0; // сколько сравнений выполнено
    long long shifts = 0;      // сколько сдвигов элементов выполнено
};

// Сортировка вставкой (по возрастанию).
void insertionSort(vector<int>& a, Stats& st) {
    int n = a.size();
    for (int i = 1; i < n; i++) {
        int key = a[i];   // элемент, который вставляем
        int j = i - 1;

        // Сдвигаем вправо все элементы левой (отсортированной) части,
        // которые больше key
        while (j >= 0) {
            st.comparisons++;
            if (a[j] > key) {
                a[j + 1] = a[j];
                st.shifts++;
                j--;
            } else {
                break;
            }
        }
        a[j + 1] = key;   // ставим key на своё место
    }
}

// Запуск сортировки с замером времени и выводом результата
void run(const string& title, vector<int> a) {
    Stats st;
    auto start = chrono::high_resolution_clock::now();
    insertionSort(a, st);
    auto end = chrono::high_resolution_clock::now();
    double ms = chrono::duration<double, milli>(end - start).count();

    cout << title << endl;
    cout << "  отсортирован: " << (is_sorted(a.begin(), a.end()) ? "да" : "НЕТ") << endl;
    cout << "  сравнений: " << st.comparisons << endl;
    cout << "  сдвигов:   " << st.shifts << endl;
    cout << "  время:     " << ms << " мс" << endl << endl;
}

int main() {
    const int N = 10000;
    mt19937 gen(42);

    // 1. Почти отсортированный массив: сначала идеальный порядок,
    //    затем 1% случайных пар соседних элементов меняем местами.
    vector<int> nearly(N);
    for (int i = 0; i < N; i++) nearly[i] = i;
    uniform_int_distribution<int> pos(0, N - 2);
    for (int i = 0; i < N / 100; i++) {
        int p = pos(gen);
        swap(nearly[p], nearly[p + 1]);
    }

    // 2. Случайный массив 
    vector<int> random_arr(N);
    uniform_int_distribution<int> val(0, 1000000);
    for (int& x : random_arr) x = val(gen);

    // 3. Массив в обратном порядке
    vector<int> reversed(N);
    for (int i = 0; i < N; i++) reversed[i] = N - i;

    cout << "Сортировка вставкой, n = " << N << endl << endl;
    run("Почти отсортированный массив (лучший практический случай):", nearly);
    run("Случайный массив (средний случай):", random_arr);
    run("Обратный порядок (худший случай):", reversed);
    return 0;
}
