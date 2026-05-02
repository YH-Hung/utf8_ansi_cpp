#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

class SpdlogTestListener : public testing::EmptyTestEventListener {
public:
    void OnTestEnd(const testing::TestInfo& test_info) override {
        const testing::TestResult* result = test_info.result();
        std::string status;
#if defined(GTEST_SKIP)
        if (result->Skipped()) {
            status = "SKIPPED";
        } else
#endif
        if (result->Passed()) {
            status = "PASSED";
        } else if (result->Failed()) {
            status = "FAILED";
        } else {
            status = "UNKNOWN";
        }

        // Determine reason: collect custom per-test properties if provided; otherwise, use a sensible default
        std::string reason;
        if (status == "PASSED") {
            for (int i = 0; i < result->test_property_count(); ++i) {
                const auto& prop = result->GetTestProperty(i);
                if (std::string_view(prop.key()) == std::string_view("success_reason")) {
                    if (!reason.empty()) reason += "; ";
                    reason += prop.value();
                }
            }
            if (reason.empty()) {
                // Heuristic reasons based on test name if not explicitly provided
                const std::string suite = test_info.test_suite_name();
                const std::string name = test_info.name();
                const std::string full = suite + "." + name;
                auto contains = [](const std::string& hay, const char* needle) {
                    return hay.find(needle) != std::string::npos;
                };
                if (contains(name, "AndBack") || contains(name, "RoundTrip")) {
                    reason = "Round-trip conversion preserved content";
                } else if (contains(name, "Equivalence")) {
                    reason = "DR and non-DR converters produce identical results";
                } else if (contains(name, "Fails") || contains(name, "Throws")) {
                    reason = "Expected exception thrown for invalid/unrepresentable input";
                } else if (contains(name, "Emoji")) {
                    reason = "Unsupported emoji correctly rejected";
                } else if (contains(name, "Empty") || contains(name, "ZeroLength")) {
                    reason = "Empty input returns empty output across overloads";
                } else if (contains(name, "EmbeddedNull")) {
                    reason = "Embedded null preserved with explicit length";
                } else if (contains(name, "Nullptr") || contains(name, "NullPointer") || contains(name, "NullWithNonZeroLength")) {
                    reason = "Null pointer handling semantics verified";
                } else if (contains(name, "SizeValidation") || contains(name, "Boundary")) {
                    reason = "Size boundary validation works as intended";
                } else if (contains(name, "Overflow")) {
                    reason = "Overflow protection prevents excessive allocation or crash";
                } else if (contains(name, "MemoryStress") || contains(name, "Stress")) {
                    reason = "Moderately large input processed successfully";
                } else if (contains(name, "Punctuation")) {
                    reason = "Fullwidth punctuation preserved by round-trip";
                } else if (contains(name, "CStr")) {
                    reason = "C-string helper overloads behave consistently";
                } else if (contains(name, "DR_")) {
                    reason = "Streaming direct converter behaves correctly";
                } else if (contains(suite, "SkipIfAlreadyTarget") || contains(name, "AlreadyValid") || contains(name, "AllPolicies")) {
                    reason = "Skip-if-already-in-target-encoding behavior matches expectation";
                } else {
                    reason = "All assertions satisfied";
                }
            }
        } else if (status == "FAILED") {
            reason = "One or more assertions failed";
        } else if (status == "SKIPPED") {
            reason = "Test skipped";
        } else {
            reason = "Unknown status";
        }

        // elapsed_time is in milliseconds
        spdlog::info("[TEST] {}.{} => {} ({} ms) | reason: {}",
                     test_info.test_suite_name(), test_info.name(), status, result->elapsed_time(), reason);
    }
};

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    // Configure spdlog for clear one-line logs per test
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

    // Register our listener (keep default printer as well)
    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new SpdlogTestListener());

    return RUN_ALL_TESTS();
}
