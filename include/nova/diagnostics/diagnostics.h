#pragma once
#include "nova/lexer/token.h"
#include <string_view>
#include <string>

namespace nova {

class Diagnostics {
public:
    explicit Diagnostics(std::string_view source);

    void reportError(SourceLocation loc, std::string_view message);
    bool hasErrors() const { return has_errors_; }

private:
    std::string_view source_;
    bool has_errors_ = false;

    std::string_view getLine(uint32_t line) const;
};

} // namespace nova
