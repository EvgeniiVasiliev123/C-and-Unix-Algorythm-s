// ЛР №3. Граф дорожной сети: перекрёстки (вершины) и участки дорог между ними (рёбра).
#pragma once

#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Параметры модели (все данные придуманы для лабораторной работы)
// ---------------------------------------------------------------------------
const double METERS_PER_PIXEL = 3.5;  // масштаб карты (оценён по длине Троицкого и Литейного мостов)
const double CAR_SPACE = 7.5;         // сколько места занимает машина в заторе, м (длина + дистанция)
const int FLOW_PER_LANE = 10;         // сколько машин в минуту пропускает одна полоса (с учётом светофоров)

// Перекрёсток
struct Node {
    std::string id, name;
    double x, y;          // координаты на изображении, пиксели
};

// Участок дороги между двумя перекрёстками (в одну сторону)
struct Edge {
    int from, to;         // номера перекрёстков
    std::string cls;      // класс дороги: bridge, avenue, street, local
    std::string street;   // название улицы
    double length;        // длина, м
    int lanes;            // число полос в этом направлении
    int capacity;         // вместимость: сколько машин помещается на участке
    int flow;             // пропускная способность: сколько машин в минуту могут покинуть участок
    int t0;               // время проезда без затора, мин (не меньше 1)
};

// Число полос и скорость по классу дороги
inline int lanesOfClass(const std::string& c) {
    if (c == "bridge" || c == "avenue") return 3;
    if (c == "street") return 2;
    return 1;
}
inline double speedOfClass(const std::string& c) {   // км/ч
    if (c == "bridge" || c == "avenue") return 50;
    if (c == "street") return 40;
    return 30;
}

// Пересчитывает вместимость и пропускную способность участка по числу полос
inline void setLanes(Edge& e, int lanes) {
    e.lanes = lanes;
    e.capacity = std::max(4, static_cast<int>(lanes * e.length / CAR_SPACE));
    e.flow = lanes * FLOW_PER_LANE;
}

struct Graph {
    std::vector<Node> nodes;
    std::vector<Edge> edges;                  // направленные участки
    std::vector<std::vector<int>> out, in;    // номера исходящих и входящих участков для каждого перекрёстка
    std::map<std::string, int> index;         // "P05" -> номер вершины

    std::string edgeName(int e) const {
        return nodes[edges[e].from].id + "->" + nodes[edges[e].to].id;
    }
};

// Разбивает строку CSV по запятым
inline std::vector<std::string> splitCsv(const std::string& line) {
    std::vector<std::string> parts;
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, ',')) parts.push_back(item);
    return parts;
}

// Читает перекрёстки и дороги. Каждая дорога из edges.csv даёт два направленных участка.
inline Graph loadGraph(const std::string& nodesFile, const std::string& edgesFile) {
    Graph g;
    std::ifstream fn(nodesFile);
    if (!fn) throw std::runtime_error("Не открывается " + nodesFile);
    std::string line;
    std::getline(fn, line);                   // заголовок
    while (std::getline(fn, line)) {
        if (line.empty()) continue;
        auto p = splitCsv(line);
        g.index[p[0]] = static_cast<int>(g.nodes.size());
        g.nodes.push_back({p[0], p[1], std::stod(p[2]), std::stod(p[3])});
    }

    std::ifstream fe(edgesFile);
    if (!fe) throw std::runtime_error("Не открывается " + edgesFile);
    std::getline(fe, line);
    while (std::getline(fe, line)) {
        if (line.empty()) continue;
        auto p = splitCsv(line);
        int a = g.index.at(p[0]), b = g.index.at(p[1]);
        double len = std::hypot(g.nodes[a].x - g.nodes[b].x, g.nodes[a].y - g.nodes[b].y) * METERS_PER_PIXEL;
        for (int dir = 0; dir < 2; dir++) {
            Edge e;
            e.from = dir == 0 ? a : b;
            e.to = dir == 0 ? b : a;
            e.cls = p[2];
            e.street = p[3];
            e.length = len;
            setLanes(e, lanesOfClass(e.cls));
            double metersPerMin = speedOfClass(e.cls) * 1000.0 / 60.0;
            e.t0 = std::max(1, static_cast<int>(std::ceil(len / metersPerMin)));
            g.edges.push_back(e);
        }
    }

    g.out.assign(g.nodes.size(), {});
    g.in.assign(g.nodes.size(), {});
    for (int i = 0; i < static_cast<int>(g.edges.size()); i++) {
        g.out[g.edges[i].from].push_back(i);
        g.in[g.edges[i].to].push_back(i);
    }
    return g;
}

// Алгоритм Дейкстры "в обратную сторону": кратчайшее время от КАЖДОГО перекрёстка до перекрёстка dest.
// w[e] - время проезда участка e. Один запуск даёт ответ для всех начальных точек сразу.
inline std::vector<double> distancesTo(const Graph& g, int dest, const std::vector<double>& w) {
    const double INF = std::numeric_limits<double>::infinity();
    std::vector<double> dist(g.nodes.size(), INF);
    using Item = std::pair<double, int>;
    std::priority_queue<Item, std::vector<Item>, std::greater<Item>> pq;
    dist[dest] = 0;
    pq.push({0.0, dest});
    while (!pq.empty()) {
        auto [d, v] = pq.top();
        pq.pop();
        if (d > dist[v]) continue;
        for (int e : g.in[v]) {                   // идём против направления дорог
            int u = g.edges[e].from;
            if (d + w[e] < dist[u]) {
                dist[u] = d + w[e];
                pq.push({dist[u], u});
            }
        }
    }
    return dist;
}
