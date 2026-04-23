#pragma once
#include <string>

namespace git {

// Shell-safe single-quote a path (handles embedded single quotes).
std::string shellQuote(const std::string& s);

// Run git with args in repoRoot, return stdout. Empty string on error.
std::string run(const std::string& repoRoot, const std::string& args);

} // namespace git
