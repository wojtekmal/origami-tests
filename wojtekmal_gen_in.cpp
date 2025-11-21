#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <optional>

namespace fs = std::filesystem;

// --- STAŁE ---

// Epsilon z zadania: minimalna bezpieczna odległość od obiektu, 
// jeśli punkt nie leży idealnie na nim.
const double DANGER_EPS = 0.05;

// Epsilon numeryczny: próg uznawania wartości za zero (lub równość).
// Jeśli odległość jest mniejsza niż to, uznajemy, że punkt LEŻY na obiekcie.
const double ZERO_TOLERANCE = 1e-9;

// Maksymalna liczba prób wygenerowania poprawnego kroku
const int MAX_TRIES = 100;

// --- STRUKTURY ---

struct Point {
    double x, y;
};

struct Line {
    Point p1, p2;
};

struct Circle {
    Point c;
    double r;
};

struct SheetGeometry {
    std::vector<Point> points;   
    std::vector<Line> lines;     
    std::vector<Circle> circles; 
};

// --- FUNKCJE MATEMATYCZNE ---

double distSq(Point p1, Point p2) {
    return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
}

double dist(Point p1, Point p2) {
    return std::sqrt(distSq(p1, p2));
}

// Odległość punktu p od prostej l
double distPointLine(Point p, Line l) {
    double A = p.x - l.p1.x;
    double B = p.y - l.p1.y;
    double C = l.p2.x - l.p1.x;
    double D = l.p2.y - l.p1.y;

    double dot = A * C + B * D;
    double len_sq = C * C + D * D;
    
    if (len_sq < ZERO_TOLERANCE) return dist(p, l.p1); // Zdegenerowana linia

    double param = dot / len_sq;
    
    double xx = l.p1.x + param * C;
    double yy = l.p1.y + param * D;
    
    return dist(p, {xx, yy});
}

// Odbicie punktu względem prostej l
Point reflectPoint(Point p, Line l) {
    double A = p.x - l.p1.x;
    double B = p.y - l.p1.y;
    double C = l.p2.x - l.p1.x;
    double D = l.p2.y - l.p1.y;

    double len_sq = C * C + D * D;
    if (len_sq < ZERO_TOLERANCE) return p;

    double dot = A * C + B * D;
    double param = dot / len_sq;

    double xx = l.p1.x + param * C;
    double yy = l.p1.y + param * D;

    return {2 * xx - p.x, 2 * yy - p.y};
}

// --- LOGIKA WALIDACJI TESTU ---

// Sprawdza "niezłośliwość" punktu względem geometrii
bool isSafePoint(Point p, const SheetGeometry& geom) {
    // 1. Sprawdź punkty
    for (const auto& pt : geom.points) {
        double d = dist(p, pt);
        // Jeśli d <= ZERO_TOLERANCE -> punkt pokrywa się (dozwolone)
        // Jeśli d >= DANGER_EPS -> punkt jest daleko (dozwolone)
        // Jeśli jest pomiędzy -> sytuacja złośliwa
        if (d > ZERO_TOLERANCE && d < DANGER_EPS) return false; 
    }

    // 2. Sprawdź linie
    for (const auto& l : geom.lines) {
        double d = distPointLine(p, l);
        if (d > ZERO_TOLERANCE && d < DANGER_EPS) return false;
    }

    // 3. Sprawdź koła (odległość od obwodu)
    for (const auto& c : geom.circles) {
        double d_center = dist(p, c.c);
        double d_boundary = std::abs(d_center - c.r);
        if (d_boundary > ZERO_TOLERANCE && d_boundary < DANGER_EPS) return false;
    }

    return true;
}

// Sprawdza "niezłośliwość" nowej prostej zgięcia
bool isSafeFold(Line l, const SheetGeometry& geom) {
    // 1. Sprawdź odległość od istniejących punktów
    for (const auto& pt : geom.points) {
        double d = distPointLine(pt, l);
        if (d > ZERO_TOLERANCE && d < DANGER_EPS) return false;
    }

    // 2. Sprawdź relację z kołami
    for (const auto& c : geom.circles) {
        double d = distPointLine(c.c, l);
        
        // Prosta przechodzi bardzo blisko środka koła (ale nie idealnie przez środek)
        if (d > ZERO_TOLERANCE && d < DANGER_EPS) return false; 

        // Prosta jest "prawie" styczna
        if (std::abs(d - c.r) > ZERO_TOLERANCE && std::abs(d - c.r) < DANGER_EPS) return false;
    }

    return true;
}

// --- GENERATOR ---

bool generate_test(fs::path path, std::mt19937& rng, int n, int q) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    std::vector<SheetGeometry> sheets;
    sheets.reserve(n + 1);
    sheets.push_back({}); // Indeks 0 pusty

    std::uniform_real_distribution<double> coord_dist(-10.0, 10.0);
    std::uniform_real_distribution<double> size_dist(1.0, 5.0);
    std::uniform_int_distribution<int> type_dist(0, 2); 
    
    file << n << " " << q << "\n";
    file << std::fixed << std::setprecision(6);

    // --- Generowanie N kartek ---
    for (int i = 1; i <= n; ++i) {
        bool success = false;
        for (int try_count = 0; try_count < MAX_TRIES; ++try_count) {
            int type = (i == 1) ? (rng() % 2) : type_dist(rng);
            if (type == 2 && i == 1) type = 0; 

            SheetGeometry newGeom;
            std::string lineOutput;

            if (type == 0) { // Prostokąt P
                double x1 = coord_dist(rng);
                double y1 = coord_dist(rng);
                double w = size_dist(rng);
                double h = size_dist(rng);
                double x2 = x1 + w;
                double y2 = y1 + h;
                
                newGeom.points = {{x1, y1}, {x2, y1}, {x2, y2}, {x1, y2}};
                newGeom.lines = {{{x1, y1}, {x2, y1}}, {{x2, y1}, {x2, y2}}, {{x2, y2}, {x1, y2}}, {{x1, y2}, {x1, y1}}};
                
                lineOutput = "P " + std::to_string(x1) + " " + std::to_string(y1) + " " + std::to_string(x2) + " " + std::to_string(y2);
                sheets.push_back(newGeom);
                file << lineOutput << "\n";
                success = true;
                break;
            } 
            else if (type == 1) { // Koło K
                double x = coord_dist(rng);
                double y = coord_dist(rng);
                double r = size_dist(rng);
                
                newGeom.points = {{x, y}};
                newGeom.circles = {{{x, y}, r}};
                
                lineOutput = "K " + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(r);
                sheets.push_back(newGeom);
                file << lineOutput << "\n";
                success = true;
                break;
            } 
            else { // Zgięcie Z
                std::uniform_int_distribution<int> sheet_idx_dist(1, i - 1);
                int k = sheet_idx_dist(rng);
                const auto& baseGeom = sheets[k];

                Line foldLine;
                int strategy = std::uniform_int_distribution<int>(0, 2)(rng);
                
                if (strategy == 0 || baseGeom.points.empty()) { 
                    foldLine.p1 = {coord_dist(rng), coord_dist(rng)};
                    foldLine.p2 = {coord_dist(rng), coord_dist(rng)};
                } else if (strategy == 1) { 
                    std::uniform_int_distribution<int> pt_idx(0, baseGeom.points.size() - 1);
                    Point p = baseGeom.points[pt_idx(rng)];
                    double angle = std::uniform_real_distribution<double>(0, 2 * M_PI)(rng);
                    foldLine.p1 = p;
                    foldLine.p2 = {p.x + std::cos(angle), p.y + std::sin(angle)};
                } else { 
                    if (!baseGeom.circles.empty()) {
                        std::uniform_int_distribution<int> c_idx(0, baseGeom.circles.size() - 1);
                        Circle c = baseGeom.circles[c_idx(rng)];
                        double angle = std::uniform_real_distribution<double>(0, 2 * M_PI)(rng);
                        Point tangentPoint = {c.c.x + c.r * std::cos(angle), c.c.y + c.r * std::sin(angle)};
                        foldLine.p1 = tangentPoint;
                        foldLine.p2 = {tangentPoint.x - std::sin(angle), tangentPoint.y + std::cos(angle)};
                    } else {
                        foldLine.p1 = {coord_dist(rng), coord_dist(rng)};
                        foldLine.p2 = {coord_dist(rng), coord_dist(rng)};
                    }
                }

                if (distSq(foldLine.p1, foldLine.p2) < ZERO_TOLERANCE) continue;
                if (!isSafeFold(foldLine, baseGeom)) continue;

                newGeom = baseGeom;
                
                for (const auto& p : baseGeom.points) newGeom.points.push_back(reflectPoint(p, foldLine));
                for (const auto& l : baseGeom.lines) newGeom.lines.push_back({reflectPoint(l.p1, foldLine), reflectPoint(l.p2, foldLine)});
                for (const auto& c : baseGeom.circles) newGeom.circles.push_back({reflectPoint(c.c, foldLine), c.r});
                
                newGeom.lines.push_back(foldLine);

                lineOutput = "Z " + std::to_string(k) + " " + std::to_string(foldLine.p1.x) + " " + 
                             std::to_string(foldLine.p1.y) + " " + std::to_string(foldLine.p2.x) + " " + 
                             std::to_string(foldLine.p2.y);
                
                sheets.push_back(newGeom);
                file << lineOutput << "\n";
                success = true;
                break;
            }
        }
        if (!success) {
            file.close();
            return false;
        }
    }

    // --- Generowanie Q zapytań ---
    for (int j = 0; j < q; ++j) {
        bool success = false;
        for (int try_count = 0; try_count < MAX_TRIES; ++try_count) {
            std::uniform_int_distribution<int> k_dist(1, n);
            int k = k_dist(rng);
            const auto& geom = sheets[k];

            Point p;
            int strategy = std::uniform_int_distribution<int>(0, 2)(rng);

            if (strategy == 0 || geom.points.empty()) {
                p = {coord_dist(rng), coord_dist(rng)};
            } else {
                std::uniform_int_distribution<int> pt_idx(0, geom.points.size() - 1);
                p = geom.points[pt_idx(rng)];
            }

            if (isSafePoint(p, geom)) {
                file << k << " " << p.x << " " << p.y << "\n";
                success = true;
                break;
            }
        }
        if (!success) {
            file.close();
            return false;
        }
    }

    file.close();
    return true;
}

bool generate_test_with_group_index(std::string group, int index, std::mt19937& rng, int n, int q)
{
    fs::path file_path = group + "/" + std::to_string(index) + ".in";
    return generate_test(file_path, rng, n, q);
}

int main()
{
    std::mt19937 rng{};

    for (int i = 0; i < 10000;)
    {
        if (generate_test_with_group_index("wojtekmal_tiny", i, rng, 3, 3)) i++;
    }

    for (int i = 0; i < 10000;)
    {
        if (generate_test_with_group_index("wojtekmal_small", i, rng, 10, 100)) i++;
    }
}