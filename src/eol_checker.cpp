#include "eol_checker.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <tuple>

// ============================================================================
//  Algorithm
//
//  The core routine only handles EOL edges that face up (+y). The other three
//  directions are handled by rotating the whole layer by 90 degrees, running
//  the same routine, and rotating the results back.
//
//  For every layer and every direction (rotation k = 0..3):
//    1. Build a uniform-grid spatial index over the rectangles, so every
//       "which rects touch this box" query only looks at nearby rects.
//    2. For each rect r, its top side (y = Y) is a candidate. Grow [xa, xb]
//       with every rect that is filled just below Y and touches the interval:
//       this is the edge of the merged metal that contains r's top side.
//       It is an EOL edge if
//         - its length xb - xa <= eolWidth, and
//         - no rect covers it from above (otherwise it is not on the boundary,
//           or one of its corners is concave).
//    3. Every EOL edge is checked (the spec's parallel-edge condition is not used).
//       EOL spacing region (Fig. 2a), eolSpace deep and eolWithin wider on both sides:
//         [xa - eolWithin, xb + eolWithin] x [Y, Y + eolSpace].
//       A rect whose interior overlaps it is too close; a rect only touching
//       its border is not (spacing == eolSpace is legal). Each EOL edge with
//       such rects is ONE violation, seen from that edge: two line ends facing
//       each other are two violations. The marker is the part of the region
//       between the edge level Y and the rects (their bounding box).
// ============================================================================

namespace {

struct Shape {
    Coord xl, yl, xh, yh;
    int id;
};

// Rotates 90 degrees counter-clockwise k times: (x, y) -> (-y, x).
Shape rotate(Shape s, int k) {
    for (int i = 0; i < k; ++i) s = {-s.yh, s.xl, -s.yl, s.xh, s.id};
    return s;
}

Box rotateBox(const Box& b, int k) {
    Shape s = rotate({b.xl, b.yl, b.xh, b.yh, -1}, k);
    return {s.xl, s.yl, s.xh, s.yh};
}

Segment rotateSegment(Segment g, int k) {
    for (int i = 0; i < k; ++i) g = {-g.y1, g.x1, -g.y2, g.x2};
    if (g.x1 > g.x2 || (g.x1 == g.x2 && g.y1 > g.y2)) {
        std::swap(g.x1, g.x2);
        std::swap(g.y1, g.y2);
    }
    return g;
}

// Uniform grid of buckets; each shape is stored in every cell it overlaps.
class GridIndex {
public:
    explicit GridIndex(const std::vector<Shape>& shapes) : shapes_(shapes), stamp_(shapes.size(), 0) {
        if (shapes_.empty()) return;
        x0_ = shapes_[0].xl;
        y0_ = shapes_[0].yl;
        Coord x1 = shapes_[0].xh, y1 = shapes_[0].yh;
        for (const Shape& s : shapes_) {
            x0_ = std::min(x0_, s.xl);
            y0_ = std::min(y0_, s.yl);
            x1 = std::max(x1, s.xh);
            y1 = std::max(y1, s.yh);
        }
        // about one cell per shape, at least 1 um
        double area = static_cast<double>(x1 - x0_) * static_cast<double>(y1 - y0_);
        cell_ = std::max<Coord>(DBU_PER_UM, static_cast<Coord>(std::sqrt(area / shapes_.size())));
        const long long maxCells = 4 * static_cast<long long>(shapes_.size()) + 1024;
        while (true) {
            nx_ = (x1 - x0_) / cell_ + 1;
            ny_ = (y1 - y0_) / cell_ + 1;
            if (nx_ * ny_ <= maxCells) break;
            cell_ *= 2;
        }
        cells_.assign(nx_ * ny_, {});
        for (int i = 0; i < static_cast<int>(shapes_.size()); ++i) {
            const Shape& s = shapes_[i];
            for (long long cy = row(s.yl); cy <= row(s.yh); ++cy)
                for (long long cx = col(s.xl); cx <= col(s.xh); ++cx) cells_[cy * nx_ + cx].push_back(i);
        }
    }

    // Calls f(shape) once for every shape intersecting the closed box (touching counts).
    template <class F>
    void query(Coord xl, Coord yl, Coord xh, Coord yh, F&& f) {
        if (shapes_.empty()) return;
        ++cur_;
        for (long long cy = row(yl); cy <= row(yh); ++cy) {
            for (long long cx = col(xl); cx <= col(xh); ++cx) {
                for (int i : cells_[cy * nx_ + cx]) {
                    if (stamp_[i] == cur_) continue;
                    stamp_[i] = cur_;
                    const Shape& s = shapes_[i];
                    if (s.xl <= xh && s.xh >= xl && s.yl <= yh && s.yh >= yl) f(s);
                }
            }
        }
    }

private:
    long long col(Coord x) const { return std::clamp<long long>((x - x0_) / cell_, 0, nx_ - 1); }
    long long row(Coord y) const { return std::clamp<long long>((y - y0_) / cell_, 0, ny_ - 1); }

    const std::vector<Shape>& shapes_;
    std::vector<int> stamp_;  // stamp_[i] == cur_ -> shape i already reported in this query
    int cur_ = 0;
    Coord x0_ = 0, y0_ = 0, cell_ = 1;
    long long nx_ = 0, ny_ = 0;
    std::vector<std::vector<int>> cells_;
};

// Checks all EOL edges facing up (+y) among `shapes` (all on one layer).
// `k` is the rotation applied to the shapes; results are rotated back.
void checkUpEdges(const std::vector<Shape>& shapes, const EolRule& rule, int layer, int k,
                  std::vector<Violation>& out, std::vector<EolCheck>* checks) {
    static const Dir kDir[4] = {Dir::Up, Dir::Right, Dir::Down, Dir::Left};
    const int back = (4 - k) % 4;

    GridIndex index(shapes);
    std::set<std::tuple<Coord, Coord, Coord>> seen;  // (Y, xa, xb) of EOL edges already checked

    for (const Shape& r : shapes) {
        const Coord Y = r.yh;
        Coord xa = r.xl, xb = r.xh;
        if (xb - xa > rule.eolWidth) continue;

        // 1. the merged edge: grow with every shape filled just below Y that touches [xa, xb]
        bool grown = true;
        while (grown && xb - xa <= rule.eolWidth) {
            grown = false;
            index.query(xa, Y, xb, Y, [&](const Shape& s) {
                if (s.yl < Y && Y <= s.yh) {
                    if (s.xl < xa) xa = s.xl, grown = true;
                    if (s.xh > xb) xb = s.xh, grown = true;
                }
            });
        }
        if (xb - xa > rule.eolWidth) continue;

        // 2. nothing may cover the edge from above
        bool covered = false;
        index.query(xa, Y, xb, Y, [&](const Shape& s) {
            if (s.yl <= Y && Y < s.yh && s.xl < xb && s.xh > xa) covered = true;
        });
        if (covered || !seen.emplace(Y, xa, xb).second) continue;

        // 3. EOL spacing region in front of the edge, eolWithin wider on both sides
        const Coord wl = xa - rule.eolWithin, wh = xb + rule.eolWithin, top = Y + rule.eolSpace;
        bool frontHit = false;
        Coord frontSpacing = 0;
        int nearestId = -1;
        Box marker{wh, Y, wl, Y};  // grows to the bounding box of the parts of all shapes found
        index.query(wl, Y, wh, top, [&](const Shape& s) {
            // interiors must overlap; only touching the border is legal
            if (s.xh <= wl || s.xl >= wh || s.yh <= Y || s.yl >= top) return;
            const Coord spacing = std::max({Coord(0), s.yl - Y, s.xl - xb, xa - s.xh});
            if (!frontHit || spacing < frontSpacing) frontSpacing = spacing, nearestId = s.id;
            frontHit = true;
            // in front: the gap up to the shape; beside the edge: the part of the shape inside
            const Coord mTop = s.yl > Y ? s.yl : std::min(s.yh, top);
            marker.xl = std::min(marker.xl, std::max(s.xl, wl));
            marker.xh = std::max(marker.xh, std::min(s.xh, wh));
            marker.yh = std::max(marker.yh, mTop);
        });

        // one violation per EOL edge, however many shapes are in front of it
        if (frontHit) {
            Violation v;
            v.layer = layer;
            v.dir = kDir[k];
            v.eolEdge = rotateSegment({xa, Y, xb, Y}, back);
            v.spacing = frontSpacing;
            v.marker = rotateBox(marker, back);
            v.eolRectId = r.id;
            v.otherRectId = nearestId;
            out.push_back(v);
        }

        if (checks) {
            EolCheck c;
            c.layer = layer;
            c.dir = kDir[k];
            c.eolEdge = rotateSegment({xa, Y, xb, Y}, back);
            c.front = rotateBox({wl, Y, wh, top}, back);
            c.violated = frontHit;
            checks->push_back(c);
        }
    }
}

}  // namespace

std::vector<Violation> checkEOL(const Layout& layout, const EolRule& rule,
                                std::vector<EolCheck>* checks) {
    std::vector<Violation> violations;
    for (const auto& [layer, rects] : layout.rectsByLayer()) {
        for (int k = 0; k < 4; ++k) {
            std::vector<Shape> shapes;
            shapes.reserve(rects.size());
            for (const Rect& r : rects) shapes.push_back(rotate({r.xl, r.yl, r.xh, r.yh, r.id}, k));
            checkUpEdges(shapes, rule, layer, k, violations, checks);
        }
    }
    return violations;
}
