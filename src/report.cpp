#include "report.h"

#include <algorithm>
#include <cstdio>
#include <tuple>

namespace {

std::string um(Coord v) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.3f", dbuToUm(v));
    return buf;
}

const char* dirName(Dir d) {
    switch (d) {
        case Dir::Left: return "left";
        case Dir::Right: return "right";
        case Dir::Down: return "down";
        case Dir::Up: return "up";
    }
    return "?";
}

using Key = std::tuple<int, Coord, Coord, Coord, Coord>;

Key keyOf(const Violation& v) {
    return Key(v.layer, v.marker.xl, v.marker.yl, v.marker.xh, v.marker.yh);
}

}  // namespace

size_t writeViolations(std::ostream& out, const std::vector<Violation>& violations) {
    std::vector<Key> keys;
    keys.reserve(violations.size());
    for (const Violation& v : violations) keys.push_back(keyOf(v));
    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());

    out << "number_of_violations " << keys.size() << "\n";
    for (const Key& k : keys) {
        out << std::get<0>(k) << "  " << um(std::get<1>(k)) << " " << um(std::get<2>(k)) << ",  "
            << um(std::get<3>(k)) << " " << um(std::get<4>(k)) << "\n";
    }
    out << "end_of_violation\n";
    return keys.size();
}

void printDetails(std::ostream& out, const std::vector<Violation>& violations) {
    for (size_t i = 0; i < violations.size(); ++i) {
        const Violation& v = violations[i];
        out << "  #" << i + 1 << " layer " << v.layer << ": EOL edge (" << um(v.eolEdge.x1) << ", "
            << um(v.eolEdge.y1) << ")-(" << um(v.eolEdge.x2) << ", " << um(v.eolEdge.y2)
            << ") facing " << dirName(v.dir) << ", spacing " << um(v.spacing) << " um, marker ("
            << um(v.marker.xl) << ", " << um(v.marker.yl) << ")-(" << um(v.marker.xh) << ", "
            << um(v.marker.yh) << ")";
        if (v.eolRectId >= 0) out << ", eol rect #" << v.eolRectId + 1;
        if (v.otherRectId >= 0) out << ", other rect #" << v.otherRectId + 1;
        out << "\n";
    }
}

void writeChecks(std::ostream& out, const std::vector<EolCheck>& checks) {
    auto box = [&](const Box& b) {
        out << um(b.xl) << " " << um(b.yl) << " " << um(b.xh) << " " << um(b.yh);
    };
    auto yesNo = [](bool b) { return b ? "yes" : "no"; };

    out << "number_of_eol_edges " << checks.size() << "\n";
    for (const EolCheck& c : checks) {
        out << c.layer << " " << dirName(c.dir) << "  edge " << um(c.eolEdge.x1) << " "
            << um(c.eolEdge.y1) << " " << um(c.eolEdge.x2) << " " << um(c.eolEdge.y2);
        for (int i = 0; i < 2; ++i) {
            out << "  side ";
            box(c.side[i]);
            out << " " << yesNo(c.sideHit[i]);
        }
        out << "  front ";
        box(c.front);
        out << "  applies " << yesNo(c.applies) << "  violation " << yesNo(c.violated) << "\n";
    }
    out << "end_of_eol_edges\n";
}
