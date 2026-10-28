#pragma once

#include <vector>

#include "eol_rule.h"
#include "layout.h"

// Which way an EOL edge faces (its outward normal).
enum class Dir { Left, Right, Down, Up };

struct Segment {
    Coord x1 = 0, y1 = 0, x2 = 0, y2 = 0;
};

struct Box {
    Coord xl = 0, yl = 0, xh = 0, yh = 0;
};

// One violated EOL edge (an EOL edge is one violation, however many shapes are in front of it).
struct Violation {
    int layer = 0;
    Segment eolEdge;       // the EOL edge that is violated
    Dir dir = Dir::Up;     // direction the EOL edge faces
    Coord spacing = 0;     // spacing to the nearest offending shape (< rule.eolSpace)
    Box marker;            // region between the EOL edge and the offending shapes (this is what gets reported)
    int eolRectId = -1;    // optional: input rect owning the EOL edge (-1 if unknown / merged shape)
    int otherRectId = -1;  // optional: nearest offending input rect
};

// One EOL edge that was checked, with the regions that were looked at (for drawing / debugging).
struct EolCheck {
    int layer = 0;
    Segment eolEdge;
    Dir dir = Dir::Up;
    Box front;              // EOL spacing region in front of the edge
                            // ((edge length + 2 * eolWithin) x eolSpace)
    bool violated = false;  // a shape was found in the front region
};

// Checks the EOL rule on every layer and returns all violations, one per violated EOL edge.
// If `checks` is not null, every EOL edge found is also recorded there.
std::vector<Violation> checkEOL(const Layout& layout, const EolRule& rule,
                                std::vector<EolCheck>* checks = nullptr);
