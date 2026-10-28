#pragma once

#include "layout.h"

// EOL rule parameters from the project spec (values in DBU, 1 um = 1000).
struct EolRule {
    Coord eolSpace = 1000;  // EOL spacing: space in front of an EOL edge must be >= 1.0 um
    Coord eolWidth = 600;   // length of end: an edge is an EOL edge if its length <= 0.6 um
    // The spec's parallel length (0.7 um) and parallel halo (0.4 um) are not used:
    // every EOL edge is checked, with or without metal beside it.
    Coord eolWithin = 400;  // EOLWithin ("halo" in the spec's figure): the EOL spacing region in
                            // front of the EOL edge is this much wider on both sides. The spec gives
                            // no value; 0.4 um is assumed, as wide as the halo in the spec's figure
};
