// ---------------------------------------------------------------------------
// Tests for the brace-block extractor used by the Source/Valve asset importers.
//
// The nesting cases below are the ones the previous regex-based implementation
// got wrong: it truncated every block at its first inner closing brace, so a
// VMF world containing solids imported as an empty world and reported success.
// ---------------------------------------------------------------------------

#include "TestFramework.h"

#include "TextBlockParser.h"

using namespace Nexus;

namespace {

NEXUS_TEST(ExtractsASimpleBlock) {
    const auto blocks = ExtractBracedBlocks("world { \"key\" \"value\" }", "world");
    CHECK_EQ(blocks.size(), size_t(1));
    CHECK(blocks[0].find("\"key\"") != std::string::npos);
}

NEXUS_TEST(ExtractsSeveralSiblingBlocks) {
    const auto blocks = ExtractBracedBlocks("solid { a } solid { b } solid { c }", "solid");
    CHECK_EQ(blocks.size(), size_t(3));
    CHECK(blocks[0].find('a') != std::string::npos);
    CHECK(blocks[2].find('c') != std::string::npos);
}

NEXUS_TEST(NestedBlocksAreNotTruncated) {
    // The regression this parser exists for. A non-greedy regex returned
    // " solid { side { " here and dropped the rest of the geometry.
    const std::string vmf = "world { solid { side { plane } } }";
    const auto blocks = ExtractBracedBlocks(vmf, "world");

    CHECK_EQ(blocks.size(), size_t(1));
    CHECK(blocks[0].find("plane") != std::string::npos);
    // The body must contain both inner closing braces it owns.
    CHECK_EQ(blocks[0], std::string(" solid { side { plane } } "));
}

NEXUS_TEST(NestedBlocksSurviveTheFullDescent) {
    // Walking world -> solid -> side is exactly what ConvertVMF does.
    const std::string vmf =
        "world { solid { side { \"material\" \"BRICK\" } side { \"material\" \"WOOD\" } } "
        "solid { side { \"material\" \"METAL\" } } }";

    const auto worlds = ExtractBracedBlocks(vmf, "world");
    CHECK_EQ(worlds.size(), size_t(1));

    const auto solids = ExtractBracedBlocks(worlds[0], "solid");
    CHECK_EQ(solids.size(), size_t(2));

    const auto firstSides = ExtractBracedBlocks(solids[0], "side");
    CHECK_EQ(firstSides.size(), size_t(2));
    CHECK(firstSides[0].find("BRICK") != std::string::npos);
    CHECK(firstSides[1].find("WOOD") != std::string::npos);

    const auto secondSides = ExtractBracedBlocks(solids[1], "side");
    CHECK_EQ(secondSides.size(), size_t(1));
    CHECK(secondSides[0].find("METAL") != std::string::npos);
}

NEXUS_TEST(TopLevelBlocksOnlyAtEachLevel) {
    // A nested block of the same name belongs to its parent's body, not to the
    // caller's sibling list.
    const auto blocks = ExtractBracedBlocks("group { group { inner } } group { second }", "group");
    CHECK_EQ(blocks.size(), size_t(2));
    CHECK(blocks[0].find("inner") != std::string::npos);
    CHECK(blocks[1].find("second") != std::string::npos);
}

NEXUS_TEST(BracesInsideQuotedStringsAreIgnored) {
    // Material and entity values can contain braces; counting them would
    // unbalance the scan and swallow the rest of the file.
    const auto blocks = ExtractBracedBlocks("entity { \"model\" \"props/{weird}.mdl\" }", "entity");
    CHECK_EQ(blocks.size(), size_t(1));
    CHECK(blocks[0].find("weird") != std::string::npos);
}

NEXUS_TEST(KeywordMustBeAWholeWord) {
    // "solid" must not match inside "solids" or "mysolid".
    CHECK_EQ(ExtractBracedBlocks("solids { a }", "solid").size(), size_t(0));
    CHECK_EQ(ExtractBracedBlocks("mysolid { a }", "solid").size(), size_t(0));
    CHECK_EQ(ExtractBracedBlocks("mysolid { a } solid { b }", "solid").size(), size_t(1));
}

NEXUS_TEST(WhitespaceAndNewlinesBetweenKeywordAndBraceAreAllowed) {
    // Real VMF files put the brace on the next line.
    const auto blocks = ExtractBracedBlocks("world\n\t{\n\t\"skyname\" \"sky_day\"\n\t}", "world");
    CHECK_EQ(blocks.size(), size_t(1));
    CHECK(blocks[0].find("sky_day") != std::string::npos);
}

NEXUS_TEST(KeywordWithoutABlockIsSkipped) {
    const auto blocks = ExtractBracedBlocks("world \"not a block\" world { real }", "world");
    CHECK_EQ(blocks.size(), size_t(1));
    CHECK(blocks[0].find("real") != std::string::npos);
}

NEXUS_TEST(UnbalancedInputDoesNotRunOffTheEnd) {
    // A truncated file must not yield a block swallowing the remaining text.
    const auto blocks = ExtractBracedBlocks("world { solid { side {", "world");
    CHECK_EQ(blocks.size(), size_t(0));
}

NEXUS_TEST(UnbalancedTailKeepsEarlierValidBlocks) {
    const auto blocks = ExtractBracedBlocks("world { ok } world { unterminated", "world");
    CHECK_EQ(blocks.size(), size_t(1));
    CHECK(blocks[0].find("ok") != std::string::npos);
}

NEXUS_TEST(EmptyInputsAreHandled) {
    CHECK_EQ(ExtractBracedBlocks("", "world").size(), size_t(0));
    CHECK_EQ(ExtractBracedBlocks("world { a }", "").size(), size_t(0));
    CHECK_EQ(ExtractBracedBlocks("nothing here", "world").size(), size_t(0));
}

NEXUS_TEST(EmptyBlockBodyIsExtracted) {
    const auto blocks = ExtractBracedBlocks("world {}", "world");
    CHECK_EQ(blocks.size(), size_t(1));
    CHECK_EQ(blocks[0], std::string(""));
}

NEXUS_TEST(DeeplyNestedBlocksAreCounted) {
    // Depth tracking must hold well past the two or three levels VMF uses.
    std::string deep = "root {";
    for (int i = 0; i < 64; ++i) {
        deep += " level {";
    }
    deep += " core ";
    for (int i = 0; i < 64; ++i) {
        deep += " }";
    }
    deep += " }";

    const auto blocks = ExtractBracedBlocks(deep, "root");
    CHECK_EQ(blocks.size(), size_t(1));
    CHECK(blocks[0].find("core") != std::string::npos);
}

} // namespace

int main() {
    RUN_TEST(ExtractsASimpleBlock);
    RUN_TEST(ExtractsSeveralSiblingBlocks);
    RUN_TEST(NestedBlocksAreNotTruncated);
    RUN_TEST(NestedBlocksSurviveTheFullDescent);
    RUN_TEST(TopLevelBlocksOnlyAtEachLevel);
    RUN_TEST(BracesInsideQuotedStringsAreIgnored);
    RUN_TEST(KeywordMustBeAWholeWord);
    RUN_TEST(WhitespaceAndNewlinesBetweenKeywordAndBraceAreAllowed);
    RUN_TEST(KeywordWithoutABlockIsSkipped);
    RUN_TEST(UnbalancedInputDoesNotRunOffTheEnd);
    RUN_TEST(UnbalancedTailKeepsEarlierValidBlocks);
    RUN_TEST(EmptyInputsAreHandled);
    RUN_TEST(EmptyBlockBodyIsExtracted);
    RUN_TEST(DeeplyNestedBlocksAreCounted);

    return NexusTest::Summarize("ParserTests");
}
