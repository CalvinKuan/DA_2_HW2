#include "layout.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>

Coord umToDbu(double um) { return std::llround(um * DBU_PER_UM); }

double dbuToUm(Coord v) { return static_cast<double>(v) / DBU_PER_UM; }

std::map<int, std::vector<Rect>> Layout::rectsByLayer() const {
    std::map<int, std::vector<Rect>> m;
    for (const Rect& r : rects) m[r.layer].push_back(r);
    return m;
}

namespace {

// Splits the whole file into whitespace-separated tokens; commas count as whitespace.
std::vector<std::string> tokenize(std::istream& in) {
    std::vector<std::string> tokens;
    std::string line;
    while (std::getline(in, line)) {
        std::replace(line.begin(), line.end(), ',', ' ');
        std::istringstream ss(line);
        std::string t;
        while (ss >> t) tokens.push_back(t);
    }
    return tokens;
}

bool toInt(const std::string& s, int& v) {
    try {
        size_t pos = 0;
        v = std::stoi(s, &pos);
        return pos == s.size();
    } catch (...) {
        return false;
    }
}

bool toDouble(const std::string& s, double& v) {
    try {
        size_t pos = 0;
        v = std::stod(s, &pos);
        return pos == s.size();
    } catch (...) {
        return false;
    }
}

}  // namespace

bool parseLayout(const std::string& path, Layout& layout, std::string& err) {
    std::ifstream fin(path);
    if (!fin) {
        err = "cannot open " + path;
        return false;
    }
    std::vector<std::string> tok = tokenize(fin);
    size_t i = 0;
    auto expect = [&](const std::string& word) {
        if (i < tok.size() && tok[i] == word) {
            ++i;
            return true;
        }
        err = "expected '" + word + "' at token " + std::to_string(i) +
              (i < tok.size() ? " (got '" + tok[i] + "')" : " (got EOF)");
        return false;
    };

    // ---- layers ----
    int numLayers = 0;
    if (!expect("number_of_layers")) return false;
    if (i >= tok.size() || !toInt(tok[i++], numLayers)) {
        err = "bad number_of_layers";
        return false;
    }
    while (i < tok.size() && tok[i] != "end_of_layer") {
        LayerInfo L;
        L.name = tok[i++];
        if (i >= tok.size() || !toInt(tok[i++], L.number)) {
            err = "bad layer number for layer " + L.name;
            return false;
        }
        layout.layers.push_back(L);
    }
    if (!expect("end_of_layer")) return false;
    if (static_cast<int>(layout.layers.size()) != numLayers)
        std::cerr << "[warn] number_of_layers says " << numLayers << " but read "
                  << layout.layers.size() << "\n";

    // ---- rectangles ----
    int numRects = 0;
    if (!expect("number_of_rectangles")) return false;
    if (i >= tok.size() || !toInt(tok[i++], numRects)) {
        err = "bad number_of_rectangles";
        return false;
    }
    while (i < tok.size() && tok[i] != "end_of_rectangle") {
        if (i + 5 > tok.size()) {
            err = "truncated rectangle line";
            return false;
        }
        Rect r;
        double x1, y1, x2, y2;
        if (!toInt(tok[i], r.layer) || !toDouble(tok[i + 1], x1) || !toDouble(tok[i + 2], y1) ||
            !toDouble(tok[i + 3], x2) || !toDouble(tok[i + 4], y2)) {
            err = "bad rectangle near token " + std::to_string(i) + " ('" + tok[i] + "')";
            return false;
        }
        i += 5;
        Coord ax = umToDbu(x1), ay = umToDbu(y1), bx = umToDbu(x2), by = umToDbu(y2);
        r.xl = std::min(ax, bx);
        r.xh = std::max(ax, bx);
        r.yl = std::min(ay, by);
        r.yh = std::max(ay, by);
        if (r.xl == r.xh || r.yl == r.yh) {
            std::cerr << "[warn] skip zero-area rectangle #" << layout.rects.size() << "\n";
            continue;
        }
        r.id = static_cast<int>(layout.rects.size());
        layout.rects.push_back(r);
    }
    if (!expect("end_of_rectangle")) return false;
    if (static_cast<int>(layout.rects.size()) != numRects)
        std::cerr << "[warn] number_of_rectangles says " << numRects << " but read "
                  << layout.rects.size() << "\n";
    return true;
}
