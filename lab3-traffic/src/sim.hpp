// ЛР №3. Моделирование движения: агенты (автомобили) едут по графу, на участках образуются очереди.
#pragma once

#include <algorithm>
#include <deque>
#include <random>

#include "graph.hpp"

const int DAY_MINUTES = 1440;        // сутки
const int SIM_MINUTES = 1440 + 300;  // + 5 часов, чтобы все машины успели доехать
const double HIGH_LOAD = 0.90;       // порог "высокой загруженности"

// ---------------------------------------------------------------------------
// Исходные данные: потоки и агенты
// ---------------------------------------------------------------------------
struct Flow {                // группа поездок "из A в B"
    int from, to;
    int morning, evening, day;   // сколько машин выезжает в утренний пик, вечерний пик и в течение дня
};

struct Agent {               // один автомобиль
    int origin, dest;        // откуда и куда едет
    int depart;              // минута выезда
    int arrive = -1;         // минута прибытия (-1 - ещё в пути)
    int ready = 0;           // с какой минуты может покинуть текущий участок
};

inline std::vector<Flow> loadFlows(const Graph& g, const std::string& file) {
    std::vector<Flow> flows;
    std::ifstream f(file);
    if (!f) throw std::runtime_error("Не открывается " + file);
    std::string line;
    std::getline(f, line);
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        auto p = splitCsv(line);
        flows.push_back({g.index.at(p[0]), g.index.at(p[1]), std::stoi(p[2]), std::stoi(p[3]), std::stoi(p[4])});
    }
    return flows;
}

// Параметры времени выезда (минуты от полуночи).
struct Profile {
    double morningPeak = 8 * 60 + 45;   // утренний пик: 08:45
    double eveningPeak = 18 * 60;       // вечерний пик: 18:00
    double sigmaMorning = 45;           // разброс времени выезда утром, мин
    double sigmaEvening = 55;           // разброс времени выезда вечером, мин
};

// Создаёт агентов: время выезда в пиках распределено нормально, днём - равномерно с 07:00 до 22:00.
inline std::vector<Agent> makeAgents(const std::vector<Flow>& flows, const Profile& pr, unsigned seed) {
    std::mt19937 gen(seed);
    std::vector<Agent> agents;
    auto normalTime = [&](double mean, double sigma) {
        std::normal_distribution<double> nd(mean, sigma);
        while (true) {
            int t = static_cast<int>(std::lround(nd(gen)));
            if (t >= 5 * 60 && t < 23 * 60) return t;   // выезды только с 05:00 до 23:00
        }
    };
    std::uniform_int_distribution<int> uni(7 * 60, 22 * 60);
    for (const Flow& f : flows) {
        for (int i = 0; i < f.morning; i++) agents.push_back({f.from, f.to, normalTime(pr.morningPeak, pr.sigmaMorning)});
        for (int i = 0; i < f.evening; i++) agents.push_back({f.from, f.to, normalTime(pr.eveningPeak, pr.sigmaEvening)});
        for (int i = 0; i < f.day; i++) agents.push_back({f.from, f.to, uni(gen)});
    }
    return agents;
}

// ---------------------------------------------------------------------------
// Результат моделирования
// ---------------------------------------------------------------------------
struct SimResult {
    std::vector<std::vector<short>> count;   // count[участок][минута] - сколько машин на участке
    std::vector<Agent> agents;
    long long moves = 0;                     // сколько раз машины переехали с участка на участок (для оценки сложности)
    int destinations = 0;                    // число различных пунктов назначения
};

// dynamic = false: водители едут по кратчайшему пути без учёта заторов.
// dynamic = true : каждую минуту время проезда участков пересчитывается с учётом загрузки (как у навигатора).
inline SimResult simulate(const Graph& g, std::vector<Agent> agents, bool dynamic) {
    const int E = static_cast<int>(g.edges.size());
    const int V = static_cast<int>(g.nodes.size());

    // Список пунктов назначения, для каждого из них хранится кратчайшее время от всех перекрёстков
    std::vector<int> destIndex(V, -1);
    std::vector<int> dests;
    for (const Agent& a : agents)
        if (destIndex[a.dest] < 0) { destIndex[a.dest] = static_cast<int>(dests.size()); dests.push_back(a.dest); }
    std::vector<std::vector<double>> dist(dests.size());
    long long moves = 0;

    std::vector<double> w(E);                       // текущее время проезда каждого участка
    auto updateRoutes = [&](const std::vector<std::deque<int>>& queues) {
        for (int e = 0; e < E; e++) {
            double load = static_cast<double>(queues[e].size()) / g.edges[e].capacity;
            w[e] = dynamic ? g.edges[e].t0 * (1.0 + 3.0 * load * load) : g.edges[e].t0;
        }
        for (size_t d = 0; d < dests.size(); d++) dist[d] = distancesTo(g, dests[d], w);
    };

    // Выбор следующего участка: тот, где (время проезда + оставшееся время до цели) минимально.
    // Разворот назад запрещён, если у перекрёстка есть другие выезды.
    auto chooseNext = [&](int v, int dest, int prevEdge) {
        int best = -1;
        double bestVal = std::numeric_limits<double>::infinity();
        for (int e : g.out[v]) {
            if (prevEdge >= 0 && g.edges[e].to == g.edges[prevEdge].from && g.out[v].size() > 1) continue;
            double val = w[e] + dist[destIndex[dest]][g.edges[e].to];
            if (val < bestVal - 1e-9) { bestVal = val; best = e; }
        }
        return best;
    };

    // Очереди автомобилей на участках (FIFO) - в них хранятся номера агентов
    std::vector<std::deque<int>> queues(E);
    std::vector<std::deque<int>> originQueue(V);          // ждут выезда на дорогу
    std::vector<std::vector<int>> departures(SIM_MINUTES);
    for (int i = 0; i < static_cast<int>(agents.size()); i++) departures[agents[i].depart].push_back(i);

    SimResult res;
    res.count.assign(E, std::vector<short>(SIM_MINUTES, 0));
    std::vector<int> edgeOf(agents.size(), -1);            // на каком участке сейчас агент

    updateRoutes(queues);
    for (int t = 0; t < SIM_MINUTES; t++) {
        if (dynamic && t > 0) updateRoutes(queues);

        // 1. Движение: с каждого участка уезжает не больше flow машин, и только те, кто уже проехал участок
        for (int e = 0; e < E; e++) {
            int moved = 0;
            while (!queues[e].empty() && moved < g.edges[e].flow) {
                int id = queues[e].front();
                Agent& a = agents[id];
                if (a.ready > t) break;                        // первая машина ещё едет по участку
                int v = g.edges[e].to;
                if (v == a.dest) {                             // приехал
                    queues[e].pop_front();
                    a.arrive = t;
                    moved++;
                    moves++;
                    continue;
                }
                int next = chooseNext(v, a.dest, e);
                if (static_cast<int>(queues[next].size()) >= g.edges[next].capacity) break;   // впереди затор
                queues[e].pop_front();
                queues[next].push_back(id);
                a.ready = t + g.edges[next].t0;
                edgeOf[id] = next;
                moved++;
                moves++;
            }
        }

        // 2. Новые машины выезжают и встают в очередь на въезд
        for (int id : departures[t]) {
            if (agents[id].origin == agents[id].dest) { agents[id].arrive = t; continue; }
            originQueue[agents[id].origin].push_back(id);
        }
        for (int v = 0; v < V; v++) {
            int entered = 0;
            while (!originQueue[v].empty() && entered < 30) {
                int id = originQueue[v].front();
                int next = chooseNext(v, agents[id].dest, -1);
                if (static_cast<int>(queues[next].size()) >= g.edges[next].capacity) break;
                originQueue[v].pop_front();
                queues[next].push_back(id);
                agents[id].ready = t + g.edges[next].t0;
                edgeOf[id] = next;
                entered++;
                moves++;
            }
        }

        // 3. Записываем, сколько машин на каждом участке
        for (int e = 0; e < E; e++) res.count[e][t] = static_cast<short>(queues[e].size());
    }
    res.agents = std::move(agents);
    res.moves = moves;
    res.destinations = static_cast<int>(dests.size());
    return res;
}

// ---------------------------------------------------------------------------
// Показатели участков (задача 1)
// ---------------------------------------------------------------------------
struct EdgeStats {
    int edge = 0;
    int peakCount = 0;        // максимум машин на участке
    double peakLoad = 0;      // максимальная загруженность, %
    int peakTime = 0;         // минута максимума
    int minutesHigh = 0;      // сколько минут за сутки загруженность выше 90%
    int epStart = -1, epEnd = -1;   // самый длинный непрерывный период загруженности выше 90%: [start, end)
    int epPeakTime = -1;      // минута, когда загрузка впервые достигла максимума внутри этого периода
    int dissipate = 0;        // время рассасывания: от момента максимальной загрузки до конца периода, мин
    double avgLoad = 0;       // средняя загруженность за сутки, %
};

inline EdgeStats computeStats(const Graph& g, const SimResult& r, int e) {
    EdgeStats s;
    s.edge = e;
    const int cap = g.edges[e].capacity;
    long long sum = 0;
    int curStart = -1, bestLen = 0;
    for (int t = 0; t <= SIM_MINUTES; t++) {
        int c = t < SIM_MINUTES ? r.count[e][t] : 0;
        if (t < SIM_MINUTES) {
            sum += c;
            if (c > s.peakCount) { s.peakCount = c; s.peakTime = t; }
        }
        bool high = t < SIM_MINUTES && c * 100 > static_cast<int>(HIGH_LOAD * 100) * cap;
        if (high) {
            s.minutesHigh++;
            if (curStart < 0) curStart = t;
        } else if (curStart >= 0) {                      // период закончился
            if (t - curStart > bestLen) { bestLen = t - curStart; s.epStart = curStart; s.epEnd = t; }
            curStart = -1;
        }
    }
    s.peakLoad = 100.0 * s.peakCount / cap;
    s.avgLoad = 100.0 * sum / SIM_MINUTES / cap;
    if (s.epStart >= 0) {
        int mx = -1;
        for (int t = s.epStart; t < s.epEnd; t++)
            if (r.count[e][t] > mx) { mx = r.count[e][t]; s.epPeakTime = t; }     // первая минута максимума
        s.dissipate = s.epEnd - s.epPeakTime;
    }
    return s;
}

// Статистика всех участков, отсортированная по "загруженности":
// сначала по числу минут с загрузкой выше 90%, затем по максимальной загрузке.
inline std::vector<EdgeStats> rankEdges(const Graph& g, const SimResult& r) {
    std::vector<EdgeStats> all;
    for (int e = 0; e < static_cast<int>(g.edges.size()); e++) all.push_back(computeStats(g, r, e));
    std::sort(all.begin(), all.end(), [](const EdgeStats& a, const EdgeStats& b) {
        if (a.minutesHigh != b.minutesHigh) return a.minutesHigh > b.minutesHigh;
        return a.peakLoad > b.peakLoad;
    });
    return all;
}

// ---------------------------------------------------------------------------
// Итоги сценария (задача 2)
// ---------------------------------------------------------------------------
struct Summary {
    int segmentsHigh = 0;        // сколько участков хотя бы раз были загружены выше 90%
    long long segmentMinutes = 0;// суммарно минут высокой загрузки по всем участкам
    double maxLoad = 0;          // максимальная загрузка среди всех участков, %
    double meanTravel = 0;       // среднее время в пути (с учётом ожидания выезда), мин
    double p95Travel = 0;        // 95-й процентиль времени в пути, мин
    int unfinished = 0;          // не доехали до конца моделирования
};

inline Summary summarize(const Graph& g, const SimResult& r) {
    Summary s;
    for (int e = 0; e < static_cast<int>(g.edges.size()); e++) {
        EdgeStats st = computeStats(g, r, e);
        if (st.minutesHigh > 0) s.segmentsHigh++;
        s.segmentMinutes += st.minutesHigh;
        s.maxLoad = std::max(s.maxLoad, st.peakLoad);
    }
    std::vector<int> times;
    for (const Agent& a : r.agents) {
        if (a.arrive < 0) { s.unfinished++; continue; }
        times.push_back(a.arrive - a.depart);
    }
    if (!times.empty()) {
        double sum = 0;
        for (int t : times) sum += t;
        s.meanTravel = sum / times.size();
        std::sort(times.begin(), times.end());
        s.p95Travel = times[static_cast<size_t>(0.95 * (times.size() - 1))];
    }
    return s;
}
