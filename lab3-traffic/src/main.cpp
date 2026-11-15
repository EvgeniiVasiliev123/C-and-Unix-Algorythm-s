// ЛР №3. Моделирование транспортной системы города: поиск заторов и способов их устранения.
//
// Запуск:  ./traffic            (данные берутся из папки data, результаты пишутся в results)
//          ./traffic data results
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "sim.hpp"

using namespace std;

// Минуты от полуночи -> "ЧЧ:ММ"
string hhmm(int t) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", (t / 60) % 24, t % 60);
    return buf;
}

// Описание участка для вывода: "C01->C02 Литейный пр."
string title(const Graph& g, int e) {
    return g.edgeName(e) + " " + g.edges[e].street;
}

// Результат одного сценария
struct ScenarioResult {
    string name;
    Summary sum;
    SimResult sim;
    double ms = 0;          // время расчёта сценария, мс
};

ScenarioResult runScenario(const string& name, const Graph& g, const vector<Flow>& flows,
                           const Profile& profile, bool dynamic) {
    ScenarioResult r;
    r.name = name;
    auto start = chrono::high_resolution_clock::now();
    r.sim = simulate(g, makeAgents(flows, profile, 2026), dynamic);
    r.ms = chrono::duration<double, milli>(chrono::high_resolution_clock::now() - start).count();
    r.sum = summarize(g, r.sim);
    return r;
}

int main(int argc, char* argv[]) {
    string dataDir = argc >= 2 ? argv[1] : "data";
    string outDir = argc >= 3 ? argv[2] : "results";

    Graph g = loadGraph(dataDir + "/nodes.csv", dataDir + "/edges.csv");
    vector<Flow> flows = loadFlows(g, dataDir + "/flows.csv");

    cout << fixed << setprecision(1);
    cout << "Перекрёстков: " << g.nodes.size() << ", участков (в обе стороны): " << g.edges.size()
         << ", групп поездок: " << flows.size() << endl;

    // =====================================================================
    // ЗАДАЧА 1. Базовый сценарий и TOP-10 самых загруженных участков
    // =====================================================================
    Profile base;
    ScenarioResult s0 = runScenario("S0 Базовый сценарий", g, flows, base, false);

    cout << "Агентов (автомобилей): " << s0.sim.agents.size() << ", шагов моделирования: " << SIM_MINUTES
         << ", пунктов назначения: " << s0.sim.destinations << ", переездов между участками: " << s0.sim.moves
         << ", время расчёта: " << s0.ms << " мс" << endl << endl;

    vector<EdgeStats> ranked = rankEdges(g, s0.sim);

    cout << "ЗАДАЧА 1. TOP-10 самых загруженных участков (порог высокой загрузки - 90%)" << endl;
    cout << " #  Участок                                   Вмест. Макс.маш. Макс.%  Мин>90%  Самый долгий затор  Рассасывание" << endl;
    ofstream top(outDir + "/top10_baseline.csv");
    top << "rank,edge,street,class,capacity,peak_count,peak_load_pct,peak_time,minutes_above_90,jam_start,jam_end,dissipation_min\n";
    for (int i = 0; i < 10; i++) {
        const EdgeStats& s = ranked[i];
        const Edge& e = g.edges[s.edge];
        string jam = s.epStart >= 0 ? hhmm(s.epStart) + "-" + hhmm(s.epEnd) : "-";
        printf("%2d  %-40s %6d %8d %7.1f %8d  %-18s %6d мин\n", i + 1, title(g, s.edge).c_str(), e.capacity,
               s.peakCount, s.peakLoad, s.minutesHigh, jam.c_str(), s.dissipate);
        top << i + 1 << "," << g.edgeName(s.edge) << "," << e.street << "," << e.cls << "," << e.capacity << ","
            << s.peakCount << "," << s.peakLoad << "," << hhmm(s.peakTime) << "," << s.minutesHigh << ","
            << (s.epStart >= 0 ? hhmm(s.epStart) : "-") << "," << (s.epStart >= 0 ? hhmm(s.epEnd) : "-") << ","
            << s.dissipate << "\n";
    }

    // Все участки (для построения карты загрузки) и временные ряды TOP-10 (для графиков)
    ofstream all(outDir + "/edges_baseline.csv");
    all << "edge,street,class,lanes,length_m,capacity,peak_count,peak_load_pct,minutes_above_90,avg_load_pct\n";
    for (const EdgeStats& s : ranked) {
        const Edge& e = g.edges[s.edge];
        all << g.edgeName(s.edge) << "," << e.street << "," << e.cls << "," << e.lanes << "," << e.length << ","
            << e.capacity << "," << s.peakCount << "," << s.peakLoad << "," << s.minutesHigh << "," << s.avgLoad << "\n";
    }
    ofstream series(outDir + "/series_top10.csv");
    series << "minute";
    for (int i = 0; i < 10; i++) series << "," << g.edgeName(ranked[i].edge);
    series << "\n";
    for (int t = 0; t < SIM_MINUTES; t++) {
        series << t;
        for (int i = 0; i < 10; i++)
            series << "," << 100.0 * s0.sim.count[ranked[i].edge][t] / g.edges[ranked[i].edge].capacity;
        series << "\n";
    }

    // =====================================================================
    // ЗАДАЧА 2. Варианты решения заторов - те же данные, другая конфигурация
    // =====================================================================
    // S1: водители пользуются навигатором (динамический выбор маршрута)
    ScenarioResult s1 = runScenario("S1 Навигатор (объезд заторов)", g, flows, base, true);

    // S2: +1 полоса на 5 самых загруженных участках (мосты расширять нельзя)
    Graph g2 = g;
    vector<int> widened;
    for (const EdgeStats& s : ranked) {
        if (widened.size() == 5) break;
        if (g.edges[s.edge].cls == "bridge") continue;
        widened.push_back(s.edge);
        setLanes(g2.edges[s.edge], g.edges[s.edge].lanes + 1);
    }
    ScenarioResult s2 = runScenario("S2 +1 полоса на 5 участках", g2, flows, base, false);

    // S3: адаптивные светофоры у Литейного моста - пропускная способность +25% на всех участках,
    // входящих в перекрёстки по обе стороны моста (V06 и C01)
    Graph g3 = g;
    for (Edge& e : g3.edges) {
        bool nearBridge = e.from == g.index.at("V06") || e.to == g.index.at("V06") ||
                          e.from == g.index.at("C01") || e.to == g.index.at("C01");
        if (nearBridge) e.flow = static_cast<int>(std::ceil(e.flow * 1.25));
    }
    ScenarioResult s3 = runScenario("S3 Светофоры у Литейного моста +25%", g3, flows, base, false);

    // S4: гибкий график работы - пик "размазан" по времени (разброс выездов больше)
    Profile flex = base;
    flex.sigmaMorning = 60;
    flex.sigmaEvening = 70;
    ScenarioResult s4 = runScenario("S4 Гибкий график работы", g, flows, flex, false);

    // S5: комплекс мер без строительства: навигатор + светофоры + гибкий график
    ScenarioResult s5 = runScenario("S5 Комплекс S1+S3+S4", g3, flows, flex, true);

    vector<const ScenarioResult*> all5 = {&s0, &s1, &s2, &s3, &s4, &s5};

    cout << endl << "ЗАДАЧА 2. Сравнение сценариев" << endl;
    cout << "Сценарий                          Участков>90%  Мин>90% (сумма)  Макс.загрузка%  Время в пути ср./p95, мин  Не доехали  Расчёт, мс" << endl;
    ofstream sc(outDir + "/scenarios.csv");
    sc << "scenario,segments_above_90,segment_minutes_above_90,max_load_pct,mean_travel_min,p95_travel_min,unfinished,calc_ms\n";
    for (const ScenarioResult* s : all5) {
        printf("%-32s %8d %14lld %16.1f %14.1f / %-8.1f %8d %10.1f\n", s->name.c_str(), s->sum.segmentsHigh,
               s->sum.segmentMinutes, s->sum.maxLoad, s->sum.meanTravel, s->sum.p95Travel, s->sum.unfinished, s->ms);
        sc << s->name << "," << s->sum.segmentsHigh << "," << s->sum.segmentMinutes << "," << s->sum.maxLoad << ","
           << s->sum.meanTravel << "," << s->sum.p95Travel << "," << s->sum.unfinished << "," << s->ms << "\n";
    }

    // Что стало с участками из TOP-10 в каждом сценарии (минут с загрузкой выше 90%)
    cout << endl << "Минуты с загрузкой выше 90% на участках TOP-10 базового сценария" << endl;
    cout << "Участок                                   ";
    for (const ScenarioResult* s : all5) cout << " " << s->name.substr(0, 2) << "   ";
    cout << endl;
    ofstream cmp(outDir + "/top10_by_scenario.csv");
    cmp << "edge";
    for (const ScenarioResult* s : all5) cmp << "," << s->name.substr(0, 2);
    cmp << "\n";
    for (int i = 0; i < 10; i++) {
        int e = ranked[i].edge;
        printf("%-42s", title(g, e).c_str());
        cmp << g.edgeName(e);
        for (const ScenarioResult* s : all5) {
            const Graph& gg = (s == &s2) ? g2 : (s == &s3 || s == &s5) ? g3 : g;
            int m = computeStats(gg, s->sim, e).minutesHigh;
            printf(" %5d", m);
            cmp << "," << m;
        }
        cout << endl;
        cmp << "\n";
    }
    cout << endl << "Расширены (S2): ";
    for (int e : widened) cout << g.edgeName(e) << " (" << g.edges[e].street << ")  ";
    cout << endl;
    return 0;
}
