#pragma once

#include <ostream>
#include <vector>

#include "eol_checker.h"
#include "layout.h"

// Writes the violation markers in the result format (one per violated EOL edge, sorted):
//   number_of_violations <k>
//   <layer>  <xl> <yl>,  <xh> <yh>     (unit = um, 3 decimals)
//   end_of_violation
// Returns the number of markers written.
size_t writeViolations(std::ostream& out, const std::vector<Violation>& violations);

// Human-readable details of every violation (for debugging / the report).
void printDetails(std::ostream& out, const std::vector<Violation>& violations);

// Writes every checked EOL edge and the regions looked at, one per line:
//   <layer> <dir>  edge x1 y1 x2 y2  front xl yl xh yh  violation <yes|no>
void writeChecks(std::ostream& out, const std::vector<EolCheck>& checks);
