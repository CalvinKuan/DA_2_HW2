#pragma once

#include "layout.h"

// EOL rule parameters from the project spec (values in DBU, 1 um = 1000).
struct EolRule {
    Coord eolSpace = 1000;  // EOL spacing: space in front of an EOL edge must be >= 1.0 um
    Coord eolWidth = 600;   // length of end: an edge is an EOL edge if its length <= 0.6 um
    Coord parWithin = 700;  // parallel length: measured back from the EOL edge along its sides, 0.7 um
    Coord parSpace = 400;   // parallel halo: distance away from the sides, 0.4 um
    Coord eolWithin = 400;  // EOLWithin ("halo" in the spec's figure): the EOL spacing region in
                            // front of the EOL edge is this much wider on both sides. The spec gives
                            // no value; 0.4 um is assumed, as wide as the halo in the spec's figure
};
