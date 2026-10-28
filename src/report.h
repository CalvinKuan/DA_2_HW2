#pragma once

#include <ostream>
#include <vector>

#include "eol_checker.h"
#include "layout.h"

// Writes the violation markers in the result format (deduplicated and sorted):
//   number_of_violations <k>
//   <layer>  <xl> <yl>,  <xh> <yh>     (unit = um, 3 decimals)
//   end_of_violation
// Returns the number of markers written (after deduplication).
size_t writeViolations(std::ostream& out, const std::vector<Violation>& violations);

// Human-readable details of every violation (for debugging / the report).
void printDetails(std::ostream& out, const std::vector<Violation>& violations);

// Lists the EOL edges with metal too close in front where the rule does not
// apply (no metal beside the end), so they are not violations.
void printNotApplied(std::ostream& out, const std::vector<EolCheck>& checks);

// Writes every checked EOL edge and the regions looked at, one per line:
//   <layer> <dir>  edge x1 y1 x2 y2  side xl yl xh yh <hit>  side xl yl xh yh <hit>
//   front xl yl xh yh  applies <yes|no>  metal_in_front <yes|no>  violation <yes|no>
void writeChecks(std::ostream& out, const std::vector<EolCheck>& checks);
