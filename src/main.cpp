#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

#include "eol_checker.h"
#include "eol_rule.h"
#include "layout.h"
#include "report.h"

namespace {

void usage(const char* prog) {
    std::cerr << "Usage: " << prog << " <layout_file> [result_file] [--regions <regions_file>]\n"
              << "  result_file   defaults to stdout\n"
              << "  --regions     also write every checked EOL edge and the regions looked at\n";
}

}  // namespace

int main(int argc, char** argv) {
    std::string inPath, outPath, regionsPath;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--regions" && i + 1 < argc) {
            regionsPath = argv[++i];
        } else if (inPath.empty()) {
            inPath = a;
        } else if (outPath.empty()) {
            outPath = a;
        } else {
            usage(argv[0]);
            return 1;
        }
    }
    if (inPath.empty()) {
        usage(argv[0]);
        return 1;
    }

    auto t0 = std::chrono::steady_clock::now();

    Layout layout;
    std::string err;
    if (!parseLayout(inPath, layout, err)) {
        std::cerr << "Parse error: " << err << "\n";
        return 1;
    }
    auto t1 = std::chrono::steady_clock::now();

    EolRule rule;
    std::vector<EolCheck> checks;
    std::vector<Violation> violations = checkEOL(layout, rule, regionsPath.empty() ? nullptr : &checks);
    auto t2 = std::chrono::steady_clock::now();

    if (!regionsPath.empty()) {
        std::ofstream frg(regionsPath);
        if (!frg) {
            std::cerr << "Cannot write " << regionsPath << "\n";
            return 1;
        }
        writeChecks(frg, checks);
    }

    if (!outPath.empty()) {
        std::ofstream fout(outPath);
        if (!fout) {
            std::cerr << "Cannot write " << outPath << "\n";
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
