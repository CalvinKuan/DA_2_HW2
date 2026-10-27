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

struct Violation {
    int layer = 0;
    Segment eolEdge;       // the EOL edge that is violated
    Dir dir = Dir::Up;     // direction the EOL edge faces
    Coord spacing = 0;     // actual spacing to the offending shape (< rule.eolSpace)
    Box marker;            // region between the EOL edge and the offending shape (this is what gets reported)
    int eolRectId = -1;    // optional: input rect owning the EOL edge (-1 if unknown / merged shape)
    int otherRectId = -1;  // optional: offending input rect
};

// Checks the EOL rule on every layer and returns all violations.
std::vector<Violation> checkEOL(const Layout& layout, const EolRule& rule);
