#pragma once

#include <cctype>
#include <cstdlib>
#include <string>
#include <vector>

namespace Nexus {

/**
 * @file ValveKeyValues.h
 * @brief Reader for Valve's KeyValues text format (VMF, VMT, VDF, gameinfo.txt).
 *
 * The format is a tree of `name { ... }` blocks whose leaves are `"key" "value"`
 * pairs. TextBlockParser.h already extracts the body of a block by name; this
 * adds the two things an importer also needs - the name of each block, so a
 * VMT's shader can be recovered, and the key/value pairs of a block without the
 * nested blocks' pairs leaking into them.
 *
 * Header-only and free of engine dependencies so tests can exercise it directly.
 */

struct ValveKeyValue {
    std::string key;
    std::string value;
};

struct ValveBlock {
    std::string name;   ///< Unquoted; a VMT's shader name arrives quoted and is stripped.
    std::string body;   ///< Text between the braces, exclusive.
};

namespace detail {

/// Advances past whitespace and `//` line comments, which Valve files use
/// freely - a comment containing a brace would otherwise unbalance the scan.
inline size_t SkipTrivia(const std::string& text, size_t pos) {
    while (pos < text.size()) {
        const unsigned char c = static_cast<unsigned char>(text[pos]);
        if (std::isspace(c)) {
            ++pos;
        } else if (text.compare(pos, 2, "//") == 0) {
            const size_t lineEnd = text.find('\n', pos);
            if (lineEnd == std::string::npos) {
                return text.size();
            }
            pos = lineEnd + 1;
        } else {
            return pos;
        }
    }
    return pos;
}

/// Reads one token: a quoted string (quotes stripped) or a bare word.
/// Returns false at end of input or on an unterminated quote.
inline bool ReadToken(const std::string& text, size_t& pos, std::string& token) {
    pos = SkipTrivia(text, pos);
    if (pos >= text.size()) {
        return false;
    }

    if (text[pos] == '"') {
        const size_t start = ++pos;
        while (pos < text.size() && text[pos] != '"') {
            // No escape processing. Valve's KeyValues parser has escape
            // sequences disabled for VMF and VMT, and their content depends on
            // it: material and model paths are written with literal backslashes
            // ("models\props\barrel.mdl"). Treating a backslash as an escape
            // corrupts every such path.
            ++pos;
        }
        if (pos >= text.size()) {
            return false;   // Unterminated quote: refuse rather than guess.
        }
        token = text.substr(start, pos - start);
        ++pos;
        return true;
    }

    if (text[pos] == '{' || text[pos] == '}') {
        token = text.substr(pos, 1);
        ++pos;
        return true;
    }

    const size_t start = pos;
    while (pos < text.size()) {
        const unsigned char c = static_cast<unsigned char>(text[pos]);
        if (std::isspace(c) || c == '{' || c == '}' || c == '"') {
            break;
        }
        ++pos;
    }
    if (pos == start) {
        return false;
    }
    token = text.substr(start, pos - start);
    return true;
}

/// Consumes a `{ ... }` starting at @p pos (which must be at the open brace) and
/// returns the body. Sets @p ok false on unbalanced input.
inline std::string ReadBlockBody(const std::string& text, size_t& pos, bool& ok) {
    ok = false;
    pos = SkipTrivia(text, pos);
    if (pos >= text.size() || text[pos] != '{') {
        return {};
    }

    const size_t bodyStart = ++pos;
    int depth = 1;

    while (pos < text.size() && depth > 0) {
        const char c = text[pos];
        if (c == '"') {
            // Skip the whole string: braces inside a material path or an entity
            // value must not affect the depth count. As in ReadToken, no escape
            // processing - see the comment there.
            ++pos;
            while (pos < text.size() && text[pos] != '"') {
                ++pos;
            }
            if (pos >= text.size()) {
                return {};
            }
        } else if (text.compare(pos, 2, "//") == 0) {
            const size_t lineEnd = text.find('\n', pos);
            pos = (lineEnd == std::string::npos) ? text.size() : lineEnd;
            continue;
        } else if (c == '{') {
            ++depth;
        } else if (c == '}') {
            --depth;
            if (depth == 0) {
                ok = true;
                const std::string body = text.substr(bodyStart, pos - bodyStart);
                ++pos;
                return body;
            }
        }
        ++pos;
    }

    return {};
}

} // namespace detail

/**
 * Every top-level `name { ... }` block in @p text, in order, with its name.
 *
 * Nested blocks are left inside their parent's body; recurse to reach them.
 * Stops at the first malformed block rather than returning a body that runs to
 * end of file, so a truncated download cannot look like a valid single block.
 */
inline std::vector<ValveBlock> ExtractNamedBlocks(const std::string& text) {
    std::vector<ValveBlock> blocks;
    size_t pos = 0;

    while (pos < text.size()) {
        const size_t tokenStart = detail::SkipTrivia(text, pos);
        if (tokenStart >= text.size()) {
            break;
        }

        std::string name;
        size_t cursor = tokenStart;
        if (!detail::ReadToken(text, cursor, name)) {
            break;
        }

        // A stray brace at this level means the file does not have the shape
        // this parser expects; skip it rather than treating it as a name.
        if (name == "{" || name == "}") {
            pos = cursor;
            continue;
        }

        const size_t afterName = detail::SkipTrivia(text, cursor);
        if (afterName >= text.size() || text[afterName] != '{') {
            // A key/value pair at the top level, not a block. Consume its value
            // so the key is not mistaken for a block name on the next pass.
            std::string ignored;
            size_t valueCursor = afterName;
            detail::ReadToken(text, valueCursor, ignored);
            pos = valueCursor;
            continue;
        }

        bool ok = false;
        size_t bodyCursor = afterName;
        const std::string body = detail::ReadBlockBody(text, bodyCursor, ok);
        if (!ok) {
            break;
        }

        blocks.push_back(ValveBlock{name, body});
        pos = bodyCursor;
    }

    return blocks;
}

/**
 * The `"key" "value"` pairs directly inside @p body, excluding those belonging
 * to nested blocks.
 *
 * Keeping nested pairs out matters: a VMF `solid` contains several `side`
 * blocks that each have their own "material", and a flat scan would attribute
 * all of them to the solid.
 */
inline std::vector<ValveKeyValue> ParseKeyValueBody(const std::string& body) {
    std::vector<ValveKeyValue> pairs;
    size_t pos = 0;

    while (pos < body.size()) {
        std::string key;
        size_t cursor = pos;
        if (!detail::ReadToken(body, cursor, key)) {
            break;
        }

        if (key == "}" || key == "{") {
            pos = cursor;
            continue;
        }

        const size_t afterKey = detail::SkipTrivia(body, cursor);
        if (afterKey < body.size() && body[afterKey] == '{') {
            // Nested block: skip its whole body.
            bool ok = false;
            size_t blockCursor = afterKey;
            detail::ReadBlockBody(body, blockCursor, ok);
            if (!ok) {
                break;
            }
            pos = blockCursor;
            continue;
        }

        std::string value;
        size_t valueCursor = afterKey;
        if (!detail::ReadToken(body, valueCursor, value)) {
            break;
        }

        pairs.push_back(ValveKeyValue{key, value});
        pos = valueCursor;
    }

    return pairs;
}

/// Case-insensitive lookup. Valve files are inconsistent about case
/// ("$baseTexture" and "$basetexture" both appear in shipped content), so an
/// exact match would miss real data.
inline bool FindValveValue(const std::vector<ValveKeyValue>& pairs,
                           const std::string& key,
                           std::string& outValue) {
    const auto equalsIgnoringCase = [](const std::string& a, const std::string& b) {
        if (a.size() != b.size()) {
            return false;
        }
        for (size_t i = 0; i < a.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(a[i])) !=
                std::tolower(static_cast<unsigned char>(b[i]))) {
                return false;
            }
        }
        return true;
    };

    for (const ValveKeyValue& pair : pairs) {
        if (equalsIgnoringCase(pair.key, key)) {
            outValue = pair.value;
            return true;
        }
    }
    return false;
}

/**
 * Reads a whitespace-separated numeric triple.
 *
 * Accepts the bracketed spellings Valve uses interchangeably - "0 64 -32",
 * "[0 64 -32]" and "(0 64 -32)" all appear - by treating the brackets as
 * separators. Returns false unless exactly three numbers were found, so a
 * malformed origin leaves the caller's values untouched rather than
 * half-written.
 */
inline bool ParseValveVector3(const std::string& text, float& x, float& y, float& z) {
    float values[3] = {0.0f, 0.0f, 0.0f};
    int count = 0;
    size_t pos = 0;

    while (pos < text.size() && count < 4) {
        const char c = text[pos];
        if (std::isspace(static_cast<unsigned char>(c)) || c == '[' || c == ']' ||
            c == '(' || c == ')' || c == ',') {
            ++pos;
            continue;
        }

        const char* start = text.c_str() + pos;
        char* end = nullptr;
        const float value = std::strtof(start, &end);
        if (end == start) {
            return false;   // Not a number: the whole triple is suspect.
        }

        if (count < 3) {
            values[count] = value;
        }
        ++count;
        pos = static_cast<size_t>(end - text.c_str());
    }

    if (count != 3) {
        return false;
    }

    x = values[0];
    y = values[1];
    z = values[2];
    return true;
}

} // namespace Nexus
