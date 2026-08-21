// ---------------------------------------------------------------------------
// A very small test framework.
//
// The project has no test dependency and adding one (GoogleTest, Catch2) would
// mean either vendoring a large library or requiring network access at
// configure time - both at odds with a build that already has a lot of optional
// dependencies. What the engine's tests need is assertions, grouping and a
// non-zero exit code, which is about a hundred lines.
// ---------------------------------------------------------------------------
#pragma once

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace NexusTest {

struct Failure {
    std::string test;
    std::string detail;
};

inline std::vector<Failure>& Failures() {
    static std::vector<Failure> failures;
    return failures;
}

inline const char*& CurrentTest() {
    static const char* current = "<none>";
    return current;
}

inline int& TestCount() {
    static int count = 0;
    return count;
}

inline int& CheckCount() {
    static int count = 0;
    return count;
}

inline void RecordFailure(const std::string& detail) {
    Failures().push_back(Failure{CurrentTest(), detail});
}

/// Reports results and yields the process exit code.
inline int Summarize(const char* suiteName) {
    std::printf("\n%s: %d checks across %d tests\n", suiteName, CheckCount(), TestCount());

    if (Failures().empty()) {
        std::printf("  PASSED\n");
        return 0;
    }

    std::printf("  FAILED (%zu)\n", Failures().size());
    for (const Failure& f : Failures()) {
        std::printf("    [%s] %s\n", f.test.c_str(), f.detail.c_str());
    }
    return 1;
}

} // namespace NexusTest

// Declares a test function and registers it for the RUN_TEST macro below.
#define NEXUS_TEST(name) static void name()

#define RUN_TEST(name)                             \
    do {                                           \
        NexusTest::CurrentTest() = #name;          \
        ++NexusTest::TestCount();                  \
        name();                                    \
    } while (0)

#define CHECK(cond)                                                            \
    do {                                                                       \
        ++NexusTest::CheckCount();                                             \
        if (!(cond)) {                                                         \
            NexusTest::RecordFailure(std::string("CHECK(" #cond ") failed at " \
                                                 __FILE__ ":") +               \
                                     std::to_string(__LINE__));                \
        }                                                                      \
    } while (0)

#define CHECK_EQ(actual, expected)                                             \
    do {                                                                       \
        ++NexusTest::CheckCount();                                             \
        const auto nexus_a_ = (actual);                                        \
        const auto nexus_e_ = (expected);                                      \
        if (!(nexus_a_ == nexus_e_)) {                                         \
            NexusTest::RecordFailure(                                          \
                std::string(#actual " != " #expected " at " __FILE__ ":") +    \
                std::to_string(__LINE__));                                     \
        }                                                                      \
    } while (0)

/// Floating-point comparison with an absolute tolerance. Every value compared
/// in these tests is O(1)-O(100), so an absolute epsilon is well behaved and
/// avoids the relative-epsilon trap of comparing against zero.
#define CHECK_NEAR(actual, expected, eps)                                      \
    do {                                                                       \
        ++NexusTest::CheckCount();                                             \
        const double nexus_a_ = static_cast<double>(actual);                   \
        const double nexus_e_ = static_cast<double>(expected);                 \
        if (!(std::fabs(nexus_a_ - nexus_e_) <= (eps))) {                      \
            char nexus_buf_[256];                                              \
            std::snprintf(nexus_buf_, sizeof(nexus_buf_),                      \
                          "%s (%g) != %s (%g) at %s:%d", #actual, nexus_a_,    \
                          #expected, nexus_e_, __FILE__, __LINE__);            \
            NexusTest::RecordFailure(nexus_buf_);                              \
        }                                                                      \
    } while (0)
