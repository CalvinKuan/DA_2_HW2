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
