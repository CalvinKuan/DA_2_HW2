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

// One EOL edge that was checked, with the regions that were looked at (for drawing / debugging).
struct EolCheck {
    int layer = 0;
    Segment eolEdge;
    Dir dir = Dir::Up;
    Box side[2];                       // parallel regions beside the edge (parSpace x parWithin)
    bool sideHit[2] = {false, false};  // a shape was found in side[i]
    Box front;                         // EOL spacing region in front of the edge
                                       // ((edge length + 2 * eolWithin) x eolSpace)
    bool applies = false;              // parallel condition holds, so the rule applies
    bool frontHit = false;             // a shape was found in the front region (checked either way)
    Coord frontSpacing = 0;            // smallest spacing to such a shape
    bool violated = false;             // applies && frontHit
};

// Checks the EOL rule on every layer and returns all violations.
// If `checks` is not null, every EOL edge found is also recorded there.
std::vector<Violation> checkEOL(const Layout& layout, const EolRule& rule,
                                std::vector<EolCheck>* checks = nullptr);
