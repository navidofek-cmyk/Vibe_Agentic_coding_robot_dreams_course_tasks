#pragma once
#include <string>
#include <vector>

namespace mdpreview {

// Render markdown lines to display lines with ANSI codes.
// Each input line may produce one output line (no wrapping in MVP).
// Output lines are padded/clipped to `cols` visible characters.
std::vector<std::string> render(const std::vector<std::string>& lines, int cols);

} // namespace mdpreview
