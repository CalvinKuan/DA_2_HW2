#pragma once

#include <map>
#include <string>
#include <vector>

// All coordinates are stored as integers in database units (DBU) to avoid
// floating-point trouble (e.g. 2.3 - 1.3 = 0.9999999999999998 in double).
// 1 um = 1000 DBU, i.e. 1 nm resolution.
using Coord = long long;
constexpr Coord DBU_PER_UM = 1000;

Coord umToDbu(double um);
double dbuToUm(Coord v);

struct Rect {
    int id = -1;     // 0-based index in input order
    int layer = 0;   // layer number, e.g. 1 for M1
    Coord xl = 0, yl = 0;  // lower-left corner
    Coord xh = 0, yh = 0;  // upper-right corner (xl < xh, yl < yh)

    Coord width() const { return xh - xl; }
    Coord height() const { return yh - yl; }
};

struct LayerInfo {
    std::string name;  // e.g. "M1"
    int number = 0;    // e.g. 1
};

struct Layout {
    std::vector<LayerInfo> layers;
    std::vector<Rect> rects;

    // Rectangles grouped by layer number (layer -> rects on that layer).
    std::map<int, std::vector<Rect>> rectsByLayer() const;
};

// Reads a layout file in the project-1 format:
//   number_of_layers <n>
//   <name> <number>          (n lines)
//   end_of_layer
//   number_of_rectangles <m>
//   <layer> <x1> <y1>, <x2> <y2>   (m lines, unit = um)
//   end_of_rectangle
// Returns false and fills `err` on a format error.
bool parseLayout(const std::string& path, Layout& layout, std::string& err);
