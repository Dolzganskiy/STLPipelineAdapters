#include <processing.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <expected>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

struct Student {
    int group_id;
    std::string name;

    bool operator==(const Student&) const = default;
};

struct Group {
    int id;
    std::string title;

    bool operator==(const Group&) const = default;
};

struct Department {
    std::string name;

    bool operator==(const Department&) const = default;
};

std::expected<Department, std::string> ParseDepartmentMy(const std::string& str) {
    if (str.empty()) {
        return std::unexpected("Department name is empty");
    }
    if (str.contains(' ')) {
        return std::unexpected("Department name contains space");
    }
    return Department{str};
}

TEST(AsDataFlowTest, WorksWithStringStream) {
    std::vector<std::stringstream> data(1);
    data[0] << "hello";

    auto flow = AsDataFlow(data);

    auto first = flow.Next();
    ASSERT_TRUE(first.has_value());

    std::string s;
    Unwrap(*first) >> s;
    ASSERT_EQ(s, "hello");
}

TEST(FilterTest, FiltersNumbers) {
    std::vector<int> data = {1, 2, 3, 4, 5};

    auto result = AsDataFlow(data)
        | Filter([](int x) { return x % 2 == 0; })
        | AsVector();

    ASSERT_THAT(result, testing::ElementsAre(2, 4));
}

TEST(TransformTest, SquaresNumbers) {
    std::vector<int> data = {1, 2, 3};

    auto result = AsDataFlow(data)
        | Transform([](int x) { return x * x; })
        | AsVector();

    ASSERT_THAT(result, testing::ElementsAre(1, 4, 9));
}

TEST(TransformTest, ChangesType) {
    std::vector<int> data = {1, 2, 3};

    auto result = AsDataFlow(data)
        | Transform([](int x) { return std::to_string(x); })
        | AsVector();

    ASSERT_THAT(result, testing::ElementsAre("1", "2", "3"));
}

TEST(DropNulloptTest, DropsEmptyOptionals) {
    std::vector<std::optional<int>> data = {1, std::nullopt, 3, std::nullopt, 5};

    auto result = AsDataFlow(data)
        | DropNullopt()
        | AsVector();

    ASSERT_THAT(result, testing::ElementsAre(1, 3, 5));
}

TEST(SplitTest, SplitStringBySpace) {
    std::vector<std::string> data = {"one two three"};

    auto result = AsDataFlow(data) | Split(" ") | AsVector();

    ASSERT_THAT(result, testing::ElementsAre("one", "two", "three"));
}

TEST(SplitTest, SplitSeveralStrings) {
    std::vector<std::string> data = {"a b", "c d"};

    auto result = AsDataFlow(data) | Split(" ") | AsVector();

    ASSERT_THAT(result, testing::ElementsAre("a", "b", "c", "d"));
}

TEST(SplitTest, KeepsEmptyTokensBetweenDelimiters) {
    std::vector<std::string> data = {"a||b"};

    auto result = AsDataFlow(data) | Split("|") | AsVector();

    ASSERT_THAT(result, testing::ElementsAre("a", "", "b"));
}

TEST(SplitTest, KeepsEmptyTokenAtBegin) {
    std::vector<std::string> data = {"|abc"};

    auto result = AsDataFlow(data) | Split("|") | AsVector();

    ASSERT_THAT(result, testing::ElementsAre("", "abc"));
}

TEST(SplitTest, KeepsEmptyTokenAtEnd) {
    std::vector<std::string> data = {"abc|"};

    auto result = AsDataFlow(data) | Split("|") | AsVector();

    ASSERT_THAT(result, testing::ElementsAre("abc", ""));
}

TEST(SplitTest, OnlyDelimiters) {
    std::vector<std::string> data = {"||"};

    auto result = AsDataFlow(data) | Split("|") | AsVector();

    ASSERT_THAT(result, testing::ElementsAre("", "", ""));
}

TEST(SplitTest, SplitStringStreamByNewLine) {
    std::vector<std::stringstream> files(2);
    files[0] << "1\n2\n3";
    files[1] << "4\n5";

    auto result = AsDataFlow(files) | Split("\n") | AsVector();

    ASSERT_THAT(result, testing::ElementsAre("1", "2", "3", "4", "5"));
}

TEST(SplitTest, SplitStringStreamKeepsEmptyTokens) {
    std::vector<std::stringstream> files(1);
    files[0] << "a||b";

    auto result = AsDataFlow(files) | Split("|") | AsVector();

    ASSERT_THAT(result, testing::ElementsAre("a", "", "b"));
}

TEST(AggregateByKeyTest, CountsWords) {
    std::vector<std::string> data = {"a", "b", "a", "c", "b", "a"};

    auto result = AsDataFlow(data)
        | AggregateByKey(
            0,
            [](const std::string&, int& count) { ++count; },
            [](const std::string& s) { return s; }
        )
        | AsVector();

    ASSERT_THAT(
        result,
        testing::ElementsAre(
            std::pair<std::string, int>{"a", 3},
            std::pair<std::string, int>{"b", 2},
            std::pair<std::string, int>{"c", 1}
        )
    );
}

TEST(JoinTest, JoinKV) {
    std::vector<KV<int, std::string>> left = {
        {0, "a"}, {1, "b"}, {2, "c"}, {3, "d"}, {1, "e"}
    };

    std::vector<KV<int, std::string>> right = {
        {0, "f"}, {1, "g"}, {3, "i"}
    };

    auto result = AsDataFlow(left) | Join(AsDataFlow(right)) | AsVector();

    ASSERT_THAT(
        result,
        testing::ElementsAre(
            JoinResult<std::string, std::string>{"a", "f"},
            JoinResult<std::string, std::string>{"b", "g"},
            JoinResult<std::string, std::string>{"c", std::nullopt},
            JoinResult<std::string, std::string>{"d", "i"},
            JoinResult<std::string, std::string>{"e", "g"}
        )
    );
}

TEST(WriteTest, WritesWithDelimiter) {
    std::vector<int> data = {1, 2, 3};

    std::stringstream out;
    AsDataFlow(data) | Write(out, ',');

    ASSERT_EQ(out.str(), "1,2,3,");
}

TEST(OutTest, WritesWithoutDelimiter) {
    std::vector<std::string> data = {"ab", "cd"};

    std::stringstream out;
    AsDataFlow(data) | Out(out);

    ASSERT_EQ(out.str(), "abcd");
}

TEST(OpenFilesTest, ReadsFilesContent) {
    namespace fs = std::filesystem;

    fs::path p1 = "tmp_test_file_1.txt";
    fs::path p2 = "tmp_test_file_2.txt";

    {
        std::ofstream f1(p1);
        f1 << "hello";
        std::ofstream f2(p2);
        f2 << "world";
    }

    std::vector<fs::path> files = {p1, p2};

    auto result = AsDataFlow(files) | OpenFiles() | AsVector();

    ASSERT_THAT(result, testing::ElementsAre("hello", "world"));

    fs::remove(p1);
    fs::remove(p2);
}

TEST(SplitExpectedTest, SplitExpectedMy) {
    std::vector<std::stringstream> files(1);
    files[0] << "good-department|bad department||another-good-department";

    auto [unexpected_flow, good_flow] =
        AsDataFlow(files)
        | Split("|")
        | Transform(ParseDepartmentMy)
        | SplitExpected();

    std::stringstream unexpected_file;
    unexpected_flow | Write(unexpected_file, '.');

    auto expected_result = good_flow | AsVector();

    ASSERT_EQ(
        unexpected_file.str(),
        "Department name contains space.Department name is empty."
    );

    ASSERT_THAT(
        expected_result,
        testing::ElementsAre(
            Department{"good-department"},
            Department{"another-good-department"}
        )
    );
}