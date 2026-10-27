#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

#include "eol_checker.h"
#include "eol_rule.h"
#include "layout.h"
#include "report.h"

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " <layout_file> [result_file]\n"
                  << "  result_file defaults to stdout\n";
        return 1;
    }

    auto t0 = std::chrono::steady_clock::now();

    Layout layout;
    std::string err;
    if (!parseLayout(argv[1], layout, err)) {
        std::cerr << "Parse error: " << err << "\n";
        return 1;
    }
    auto t1 = std::chrono::steady_clock::now();

    EolRule rule;
    std::vector<Violation> violations = checkEOL(layout, rule);
    auto t2 = std::chrono::steady_clock::now();

    if (argc == 3) {
        std::ofstream fout(argv[2]);
        if (!fout) {
            std::cerr << "Cannot write " << argv[2] << "\n";
            return 1;
        }
        size_t written = writeViolations(fout, violations);

        // Summary goes to stdout only when the result is written to a file.
        auto ms = [](auto a, auto b) {
            return std::chrono::duration<double, std::milli>(b - a).count();
        };
        std::cout << "Layers: " << layout.layers.size() << ", rectangles: " << layout.rects.size()
                  << "\n";
        std::cout << "EOL violations: " << written << " (checkEOL returned " << violations.size()
                  << " before dedup)\n";
        printDetails(std::cout, violations);
        std::cout << "Parse time: " << ms(t0, t1) << " ms, check time: " << ms(t1, t2) << " ms\n";
    } else {
        writeViolations(std::cout, violations);
    }
    return 0;
}
