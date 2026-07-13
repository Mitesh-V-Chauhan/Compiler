#include "nova/diagnostics/diagnostics.h"
#include <iostream>
#include <algorithm>

namespace nova {

Diagnostics::Diagnostics(std::string_view source) : source_(source) {}

void Diagnostics::reportError(SourceLocation loc, std::string_view message) {
    has_errors_ = true;

    // ANSI Escape codes for colored output
    const char* RED = "\033[31m";
    const char* RESET = "\033[0m";
    const char* BOLD = "\033[1m";
    const char* CYAN = "\033[36m";

    std::cerr << BOLD << "error: " << RESET << RED << message << RESET << "\n";
    std::cerr << " --> " << CYAN << "source.nv:" << loc.line << ":" << loc.column << RESET << "\n";

    std::string_view line = getLine(loc.line);
    if (!line.empty()) {
        std::cerr << "  |\n";
        std::cerr << loc.line << " | " << line << "\n";
        std::cerr << "  | ";
        
        // Print spaces up to the column, then a caret
        for (uint32_t i = 1; i < loc.column && i <= line.length(); ++i) {
            std::cerr << " ";
        }
        std::cerr << RED << "^" << RESET << "\n";
    }
    std::cerr << "\n";
}

std::string_view Diagnostics::getLine(uint32_t line_num) const {
    if (line_num == 0) return {};

    uint32_t current_line = 1;
    size_t start = 0;

    while (start < source_.length() && current_line < line_num) {
        size_t next_newline = source_.find('\n', start);
        if (next_newline == std::string_view::npos) {
            return {};
        }
        start = next_newline + 1;
        current_line++;
    }

    if (start >= source_.length()) {
        return {};
    }

    size_t end = source_.find('\n', start);
    if (end == std::string_view::npos) {
        end = source_.length();
    }

    return source_.substr(start, end - start);
}

} // namespace nova
