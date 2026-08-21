#pragma once

#include <cmath>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

namespace Nexus {

/**
 * @file JsonWriter.h
 * @brief Minimal JSON serialiser for importer output.
 *
 * The importers previously wrote no structured output at all - every converter
 * logged a line and returned true. Producing JSON by string concatenation is
 * how that ends up emitting files that will not parse the first time an asset
 * name contains a quote or a backslash, which on Windows-authored content is
 * routine (Source material paths are backslash-separated).
 *
 * This is deliberately small: objects, arrays, strings, numbers and booleans,
 * with correct escaping and enough indentation to be diffable. It tracks nesting
 * so commas land in the right places, which is the part that is tedious and
 * easy to get subtly wrong by hand.
 */
class JsonWriter {
public:
    JsonWriter() = default;

    JsonWriter& BeginObject() { OpenScope('{'); return *this; }
    JsonWriter& BeginObject(const std::string& key) { WriteKey(key); OpenScope('{'); return *this; }
    JsonWriter& EndObject() { CloseScope('}'); return *this; }

    JsonWriter& BeginArray(const std::string& key) { WriteKey(key); OpenScope('['); return *this; }
    JsonWriter& EndArray() { CloseScope(']'); return *this; }

    JsonWriter& Value(const std::string& key, const std::string& value) {
        WriteKey(key);
        out_ << Quote(value);
        return *this;
    }

    JsonWriter& Value(const std::string& key, const char* value) {
        return Value(key, std::string(value ? value : ""));
    }

    JsonWriter& Value(const std::string& key, bool value) {
        WriteKey(key);
        out_ << (value ? "true" : "false");
        return *this;
    }

    JsonWriter& Value(const std::string& key, double value) {
        WriteKey(key);
        out_ << Number(value);
        return *this;
    }

    JsonWriter& Value(const std::string& key, long long value) {
        WriteKey(key);
        out_ << value;
        return *this;
    }

    JsonWriter& Value(const std::string& key, size_t value) {
        return Value(key, static_cast<long long>(value));
    }

    JsonWriter& Value(const std::string& key, int value) {
        return Value(key, static_cast<long long>(value));
    }

    /// An element of the array currently being written.
    JsonWriter& Element(const std::string& value) {
        Separate();
        out_ << Quote(value);
        return *this;
    }

    JsonWriter& Vector3(const std::string& key, float x, float y, float z) {
        WriteKey(key);
        out_ << '[' << Number(x) << ", " << Number(y) << ", " << Number(z) << ']';
        return *this;
    }

    std::string ToString() const { return out_.str() + "\n"; }

private:
    void WriteKey(const std::string& key) {
        Separate();
        if (!key.empty()) {
            out_ << Quote(key) << ": ";
        }
    }

    /// Emits the comma and newline that precede an entry, except for the first
    /// entry in a scope. Tracking it here is what keeps trailing commas - the
    /// most common way hand-built JSON fails to parse - impossible.
    void Separate() {
        if (!scopes_.empty()) {
            if (scopes_.back().hasEntries) {
                out_ << ',';
            }
            scopes_.back().hasEntries = true;
            out_ << '\n' << Indent(scopes_.size());
        }
    }

    void OpenScope(char opener) {
        out_ << opener;
        scopes_.push_back(Scope{false});
    }

    void CloseScope(char closer) {
        if (scopes_.empty()) {
            return;
        }
        const bool hadEntries = scopes_.back().hasEntries;
        scopes_.pop_back();
        if (hadEntries) {
            out_ << '\n' << Indent(scopes_.size());
        }
        out_ << closer;
    }

    static std::string Indent(size_t depth) {
        return std::string(depth * 2, ' ');
    }

    static std::string Number(double value) {
        // JSON has no representation for NaN or infinity, and emitting the
        // literal text would produce a file no parser accepts. Degenerate
        // values become 0, which is wrong but readable, rather than fatal.
        if (!std::isfinite(value)) {
            return "0";
        }

        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%.6g", value);
        return buffer;
    }

    static std::string Quote(const std::string& text) {
        std::string result;
        result.reserve(text.size() + 2);
        result += '"';

        for (const char c : text) {
            switch (c) {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n";  break;
                case '\r': result += "\\r";  break;
                case '\t': result += "\\t";  break;
                case '\b': result += "\\b";  break;
                case '\f': result += "\\f";  break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        // Control characters must be escaped; anything at or
                        // above 0x20 is passed through, which keeps UTF-8
                        // sequences intact.
                        char buffer[8];
                        std::snprintf(buffer, sizeof(buffer), "\\u%04x",
                                      static_cast<unsigned int>(static_cast<unsigned char>(c)));
                        result += buffer;
                    } else {
                        result += c;
                    }
                    break;
            }
        }

        result += '"';
        return result;
    }

    struct Scope {
        bool hasEntries;
    };

    std::ostringstream out_;
    std::vector<Scope> scopes_;
};

} // namespace Nexus
