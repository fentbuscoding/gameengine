#pragma once

#include <cctype>
#include <string>
#include <vector>

namespace Nexus {

/**
 * Extracts the bodies of every top-level `keyword { ... }` block in @p text.
 *
 * Used for brace-nested key/value formats such as Valve's VMF and VDF.
 *
 * This replaces a regex of the form `keyword\s*\{(.*?)\}`. A non-greedy match
 * stops at the *first* closing brace, so any block containing a nested block -
 * which in VMF means every `world` and every `solid` - was silently truncated
 * at its first inner `}`. The importer reported success while writing out a
 * fraction of the geometry, which is the worst kind of failure: quiet and
 * plausible-looking. (The regex also used `std::regex::dotall`, which does not
 * exist in std::regex at all.)
 *
 * Counting brace depth is both correct under nesting and cheaper than regex
 * backtracking. Quoted strings are skipped so a brace inside a material name
 * or entity value cannot unbalance the scan.
 *
 * @param text     The document to scan.
 * @param keyword  Block name to match, as a whole word.
 * @return The inner text of each matching block, outermost blocks only, in the
 *         order they appear. Returns what was parsed so far if the input has
 *         unbalanced braces, rather than emitting a block that runs to EOF.
 */
inline std::vector<std::string> ExtractBracedBlocks(const std::string& text,
                                                    const std::string& keyword) {
    std::vector<std::string> blocks;
    if (keyword.empty()) {
        return blocks;
    }

    size_t searchPos = 0;

    while ((searchPos = text.find(keyword, searchPos)) != std::string::npos) {
        // Require the keyword to stand alone rather than matching the tail of a
        // longer identifier (e.g. "solid" inside "solids" or "mysolid").
        const bool boundaryBefore =
            searchPos == 0 || (!std::isalnum(static_cast<unsigned char>(text[searchPos - 1])) &&
                               text[searchPos - 1] != '_');

        size_t cursor = searchPos + keyword.size();
        const bool boundaryAfter =
            cursor >= text.size() || (!std::isalnum(static_cast<unsigned char>(text[cursor])) &&
                                      text[cursor] != '_');

        if (!boundaryBefore || !boundaryAfter) {
            searchPos = cursor;
            continue;
        }

        while (cursor < text.size() && std::isspace(static_cast<unsigned char>(text[cursor]))) {
            ++cursor;
        }

        // The keyword must actually introduce a block.
        if (cursor >= text.size() || text[cursor] != '{') {
            searchPos = cursor;
            continue;
        }

        const size_t bodyStart = cursor + 1;
        int depth = 1;
        bool inQuotes = false;
        ++cursor;

        while (cursor < text.size() && depth > 0) {
            const char c = text[cursor];
            if (c == '"' && (cursor == 0 || text[cursor - 1] != '\\')) {
                inQuotes = !inQuotes;
            } else if (!inQuotes) {
                if (c == '{') {
                    ++depth;
                } else if (c == '}') {
                    --depth;
                }
            }
            ++cursor;
        }

        if (depth != 0) {
            // Unbalanced input: stop rather than emit a block running to EOF.
            break;
        }

        blocks.push_back(text.substr(bodyStart, (cursor - 1) - bodyStart));

        // Resume after this block, so nested blocks of the same name are found
        // by recursing into the body rather than reported as siblings.
        searchPos = cursor;
    }

    return blocks;
}

} // namespace Nexus
