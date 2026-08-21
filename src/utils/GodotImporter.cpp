/**
 * @file GodotImporter.cpp
 * @brief Godot .tscn / .tres / .gd parsing.
 *
 * GodotImporter::ParseSceneFile, ParseResourceFile and ParseGDScriptFile were
 * declared in GameImporter.h and never defined anywhere - calling any of them
 * was a link error, which is why nothing did, and why importing a Godot project
 * reported success while producing nothing.
 *
 * Godot's text scene format is a flat sequence of INI-style sections:
 *
 *   [gd_scene load_steps=3 format=3]
 *   [ext_resource type="PackedScene" path="res://player.tscn" id="1_abc"]
 *   [node name="Main" type="Node3D"]
 *   [node name="Player" type="CharacterBody3D" parent="."]
 *   position = Vector3(0, 2, 0)
 *
 * Hierarchy is expressed by the `parent` attribute holding a path relative to
 * the root ("." for a direct child of the root, "Player/Rig" deeper), not by
 * nesting - so the tree has to be rebuilt from those paths.
 */

#include "GameImporter.h"
#include "Logger.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace Nexus {

namespace {

std::string Trim(const std::string& text) {
    size_t begin = 0;
    size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin]))) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    return text.substr(begin, end - begin);
}

std::string StripQuotes(const std::string& text) {
    if (text.size() >= 2 && text.front() == '"' && text.back() == '"') {
        return text.substr(1, text.size() - 2);
    }
    return text;
}

/// One `[name key="value" ...]` section header plus the `key = value` lines
/// that follow it, up to the next header.
struct Section {
    std::string name;
    std::map<std::string, std::string> attributes;   ///< From the header line.
    std::map<std::string, std::string> properties;   ///< From the lines below it.
};

/// Splits `key="value" other=3` into pairs. Values may be quoted and contain
/// spaces, so this cannot be a whitespace split.
std::map<std::string, std::string> ParseAttributes(const std::string& text) {
    std::map<std::string, std::string> attributes;
    size_t pos = 0;

    while (pos < text.size()) {
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
            ++pos;
        }

        const size_t keyStart = pos;
        while (pos < text.size() && text[pos] != '=' &&
               !std::isspace(static_cast<unsigned char>(text[pos]))) {
            ++pos;
        }
        if (pos >= text.size() || keyStart == pos) {
            break;
        }

        const std::string key = text.substr(keyStart, pos - keyStart);
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
            ++pos;
        }
        if (pos >= text.size() || text[pos] != '=') {
            // A bare word with no value - not something Godot emits, but it
            // must not swallow the rest of the line.
            continue;
        }
        ++pos;
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) {
            ++pos;
        }

        std::string value;
        if (pos < text.size() && text[pos] == '"') {
            const size_t valueStart = ++pos;
            while (pos < text.size() && text[pos] != '"') {
                ++pos;
            }
            value = text.substr(valueStart, pos - valueStart);
            if (pos < text.size()) {
                ++pos;
            }
        } else {
            const size_t valueStart = pos;
            while (pos < text.size() && !std::isspace(static_cast<unsigned char>(text[pos]))) {
                ++pos;
            }
            value = text.substr(valueStart, pos - valueStart);
        }

        attributes[key] = value;
    }

    return attributes;
}

std::vector<Section> ParseSections(const std::string& text) {
    std::vector<Section> sections;
    std::istringstream stream(text);
    std::string line;

    while (std::getline(stream, line)) {
        // Files authored on Windows keep their CR through std::getline.
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        const std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == ';') {
            continue;
        }

        if (trimmed.front() == '[' && trimmed.back() == ']') {
            const std::string inner = trimmed.substr(1, trimmed.size() - 2);
            const size_t space = inner.find(' ');

            Section section;
            if (space == std::string::npos) {
                section.name = inner;
            } else {
                section.name = inner.substr(0, space);
                section.attributes = ParseAttributes(inner.substr(space + 1));
            }
            sections.push_back(std::move(section));
            continue;
        }

        if (sections.empty()) {
            continue;
        }

        const size_t equals = trimmed.find('=');
        if (equals == std::string::npos) {
            // Continuation of a multi-line value (Godot wraps long arrays).
            // Keeping only the first line would corrupt it, so it is appended
            // to whatever property was last read.
            if (!sections.back().properties.empty()) {
                auto last = std::prev(sections.back().properties.end());
                last->second += " " + trimmed;
            }
            continue;
        }

        const std::string key = Trim(trimmed.substr(0, equals));
        const std::string value = Trim(trimmed.substr(equals + 1));
        sections.back().properties[key] = value;
    }

    return sections;
}

/// Reads the numbers out of a constructor call such as `Vector3(0, 2, -1)` or
/// `Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 4, 5, 6)`.
std::vector<float> ParseNumberList(const std::string& text) {
    std::vector<float> numbers;

    const size_t open = text.find('(');
    const size_t close = text.rfind(')');
    if (open == std::string::npos || close == std::string::npos || close <= open) {
        return numbers;
    }

    const std::string inner = text.substr(open + 1, close - open - 1);
    size_t pos = 0;
    while (pos < inner.size()) {
        if (inner[pos] == ',' || std::isspace(static_cast<unsigned char>(inner[pos]))) {
            ++pos;
            continue;
        }

        const char* start = inner.c_str() + pos;
        char* end = nullptr;
        const float value = std::strtof(start, &end);
        if (end == start) {
            break;
        }
        numbers.push_back(value);
        pos = static_cast<size_t>(end - inner.c_str());
    }

    return numbers;
}

void ApplyTransform(const std::map<std::string, std::string>& properties,
                    GodotImporter::GodotNode& node) {
    node.position = XMFLOAT3{0.0f, 0.0f, 0.0f};
    node.rotation = XMFLOAT3{0.0f, 0.0f, 0.0f};
    node.scale = XMFLOAT3{1.0f, 1.0f, 1.0f};

    const auto transform = properties.find("transform");
    if (transform != properties.end()) {
        const std::vector<float> values = ParseNumberList(transform->second);

        // Transform3D is nine basis values (three column vectors) followed by
        // the origin; Transform2D is four plus two.
        if (values.size() >= 12) {
            node.position = XMFLOAT3{values[9], values[10], values[11]};

            const auto columnLength = [&values](size_t base) {
                return std::sqrt(values[base] * values[base] +
                                 values[base + 1] * values[base + 1] +
                                 values[base + 2] * values[base + 2]);
            };
            node.scale = XMFLOAT3{columnLength(0), columnLength(3), columnLength(6)};
        } else if (values.size() >= 6) {
            node.position = XMFLOAT3{values[4], values[5], 0.0f};
        }

        // Rotation is deliberately not derived from the basis: recovering Euler
        // angles requires knowing Godot's rotation order and handedness, and a
        // silently wrong orientation is worse than an absent one. An explicit
        // `rotation` property, read below, is unambiguous.
    }

    const auto position = properties.find("position");
    if (position != properties.end()) {
        const std::vector<float> values = ParseNumberList(position->second);
        if (values.size() >= 3) {
            node.position = XMFLOAT3{values[0], values[1], values[2]};
        } else if (values.size() >= 2) {
            node.position = XMFLOAT3{values[0], values[1], 0.0f};
        }
    }

    const auto rotation = properties.find("rotation");
    if (rotation != properties.end()) {
        const std::vector<float> values = ParseNumberList(rotation->second);
        if (values.size() >= 3) {
            node.rotation = XMFLOAT3{values[0], values[1], values[2]};
        } else if (values.size() >= 1) {
            // 2D nodes store a single angle.
            node.rotation = XMFLOAT3{0.0f, 0.0f, values[0]};
        }
    }

    const auto scale = properties.find("scale");
    if (scale != properties.end()) {
        const std::vector<float> values = ParseNumberList(scale->second);
        if (values.size() >= 3) {
            node.scale = XMFLOAT3{values[0], values[1], values[2]};
        } else if (values.size() >= 2) {
            node.scale = XMFLOAT3{values[0], values[1], 1.0f};
        }
    }
}

bool ReadFile(const std::string& path, std::string& outText) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    outText = buffer.str();
    return true;
}

} // namespace

bool GodotImporter::ParseSceneFile(const std::string& sceneFile, std::vector<GodotNode>& nodes) {
    std::string text;
    if (!ReadFile(sceneFile, text)) {
        Logger::Error("Could not open Godot scene: " + sceneFile);
        return false;
    }

    const std::vector<Section> sections = ParseSections(text);
    if (sections.empty()) {
        Logger::Error("Godot scene has no sections: " + sceneFile);
        return false;
    }

    // Path -> node, so a child's `parent` attribute can be resolved. Godot
    // guarantees a parent appears before its children, so one pass suffices.
    std::map<std::string, std::shared_ptr<GodotNode>> byPath;
    std::vector<std::shared_ptr<GodotNode>> roots;
    size_t nodeCount = 0;

    for (const Section& section : sections) {
        if (section.name != "node") {
            continue;
        }

        const auto nameAttr = section.attributes.find("name");
        if (nameAttr == section.attributes.end()) {
            continue;
        }

        auto node = std::make_shared<GodotNode>();
        node->name = StripQuotes(nameAttr->second);

        const auto typeAttr = section.attributes.find("type");
        if (typeAttr != section.attributes.end()) {
            node->type = StripQuotes(typeAttr->second);
        } else if (section.attributes.count("instance")) {
            // An instanced scene has no explicit type; naming it as such is more
            // useful than leaving the field blank.
            node->type = "InstancedScene";
        }

        ApplyTransform(section.properties, *node);

        for (const auto& property : section.properties) {
            node->properties[property.first] = property.second;
        }
        for (const auto& attribute : section.attributes) {
            if (attribute.first != "name" && attribute.first != "type" &&
                attribute.first != "parent") {
                node->properties[attribute.first] = StripQuotes(attribute.second);
            }
        }

        const auto parentAttr = section.attributes.find("parent");
        ++nodeCount;

        if (parentAttr == section.attributes.end()) {
            // No parent attribute means this is the scene root.
            byPath["."] = node;
            roots.push_back(node);
            continue;
        }

        const std::string parentPath = StripQuotes(parentAttr->second);
        const auto parent = byPath.find(parentPath);
        if (parent == byPath.end()) {
            // A parent this file never declared - an editable instance's
            // override, or a hand-edited file. Keeping the node as a root loses
            // its placement but not the node itself.
            Logger::Warning("Godot node '" + node->name + "' names unknown parent '" +
                            parentPath + "'; treating it as a root");
            roots.push_back(node);
        } else {
            parent->second->children.push_back(node);
        }

        // A child of the root is addressed as "Name"; deeper nodes as
        // "Parent/Name".
        const std::string path = (parentPath == ".") ? node->name : parentPath + "/" + node->name;
        byPath[path] = node;
    }

    if (nodeCount == 0) {
        Logger::Error("Godot scene declares no nodes: " + sceneFile);
        return false;
    }

    nodes.clear();
    nodes.reserve(roots.size());
    for (const std::shared_ptr<GodotNode>& root : roots) {
        nodes.push_back(*root);
    }

    Logger::Info("Parsed Godot scene " + sceneFile + ": " + std::to_string(nodeCount) +
                 " nodes, " + std::to_string(roots.size()) + " root(s)");
    return true;
}

bool GodotImporter::ParseResourceFile(const std::string& resourceFile, GodotResource& resource) {
    std::string text;
    if (!ReadFile(resourceFile, text)) {
        Logger::Error("Could not open Godot resource: " + resourceFile);
        return false;
    }

    const std::vector<Section> sections = ParseSections(text);
    if (sections.empty()) {
        return false;
    }

    resource.properties.clear();
    resource.type.clear();

    for (const Section& section : sections) {
        // The [resource] section holds the actual data; [gd_resource] holds the
        // declared type, and [sub_resource]/[ext_resource] are dependencies.
        if (section.name == "gd_resource") {
            const auto type = section.attributes.find("type");
            if (type != section.attributes.end()) {
                resource.type = StripQuotes(type->second);
            }
        } else if (section.name == "resource") {
            for (const auto& property : section.properties) {
                resource.properties[property.first] = property.second;
            }
        }
    }

    return !resource.type.empty() || !resource.properties.empty();
}

bool GodotImporter::ParseGDScriptFile(const std::string& scriptFile, std::string& code) {
    if (!ReadFile(scriptFile, code)) {
        Logger::Error("Could not open GDScript: " + scriptFile);
        return false;
    }
    return true;
}

} // namespace Nexus
