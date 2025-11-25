/*
 * Copyright (C) 2025 Petr Mironychev
 *
 * This file is part of QodeAssist.
 *
 * QodeAssist is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * QodeAssist is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QodeAssist. If not, see <https://www.gnu.org/licenses/>.
 */

#include "TestUtils.hpp"
#include <tools/EditFileTool.hpp>

#include <gtest/gtest.h>
#include <QObject>
#include <QString>

using namespace QodeAssist::Tools;

class EditFileToolTest : public QObject, public testing::Test
{
    Q_OBJECT
};

TEST_F(EditFileToolTest, testParseSearchReplaceBlocksSingleBlock)
{
    QString content = R"(```cpp
<<<<<<< SEARCH
int main() {
    return 0;
}
=======
int main() {
    std::cout << "Hello";
    return 0;
}
>>>>>>> REPLACE
```)";

    auto blocks = EditFileTool::parseSearchReplaceBlocks(content);
    
    ASSERT_EQ(blocks.size(), 1);
    EXPECT_EQ(blocks[0].searchContent, "int main() {\n    return 0;\n}");
    EXPECT_EQ(blocks[0].replaceContent, "int main() {\n    std::cout << \"Hello\";\n    return 0;\n}");
}

TEST_F(EditFileToolTest, testParseSearchReplaceBlocksMultipleBlocks)
{
    QString content = R"(```python
<<<<<<< SEARCH
def foo():
    pass
=======
def foo():
    return 42
>>>>>>> REPLACE
```

```python
<<<<<<< SEARCH
def bar():
    pass
=======
def bar():
    return "hello"
>>>>>>> REPLACE
```)";

    auto blocks = EditFileTool::parseSearchReplaceBlocks(content);
    
    ASSERT_EQ(blocks.size(), 2);
    EXPECT_EQ(blocks[0].searchContent, "def foo():\n    pass");
    EXPECT_EQ(blocks[0].replaceContent, "def foo():\n    return 42");
    EXPECT_EQ(blocks[1].searchContent, "def bar():\n    pass");
    EXPECT_EQ(blocks[1].replaceContent, "def bar():\n    return \"hello\"");
}

TEST_F(EditFileToolTest, testParseSearchReplaceBlocksEmptySearch)
{
    QString content = R"(<<<<<<< SEARCH

=======
// New content to append
void newFunction() {}
>>>>>>> REPLACE)";

    auto blocks = EditFileTool::parseSearchReplaceBlocks(content);
    
    ASSERT_EQ(blocks.size(), 1);
    EXPECT_EQ(blocks[0].searchContent, "");
    EXPECT_EQ(blocks[0].replaceContent, "// New content to append\nvoid newFunction() {}");
}

TEST_F(EditFileToolTest, testParseSearchReplaceBlocksWithoutCodeFence)
{
    QString content = R"(<<<<<<< SEARCH
old content
=======
new content
>>>>>>> REPLACE)";

    auto blocks = EditFileTool::parseSearchReplaceBlocks(content);
    
    ASSERT_EQ(blocks.size(), 1);
    EXPECT_EQ(blocks[0].searchContent, "old content");
    EXPECT_EQ(blocks[0].replaceContent, "new content");
}

TEST_F(EditFileToolTest, testParseSearchReplaceBlocksNoBlocks)
{
    QString content = "This is just regular text without any SEARCH/REPLACE blocks";

    auto blocks = EditFileTool::parseSearchReplaceBlocks(content);
    
    EXPECT_EQ(blocks.size(), 0);
}

TEST_F(EditFileToolTest, testParseSearchReplaceBlocksWithWhitespace)
{
    QString content = R"(```
<<<<<<< SEARCH
    indented line 1
    indented line 2
=======
    modified line 1
    modified line 2
    added line 3
>>>>>>> REPLACE
```)";

    auto blocks = EditFileTool::parseSearchReplaceBlocks(content);
    
    ASSERT_EQ(blocks.size(), 1);
    EXPECT_EQ(blocks[0].searchContent, "    indented line 1\n    indented line 2");
    EXPECT_EQ(blocks[0].replaceContent, "    modified line 1\n    modified line 2\n    added line 3");
}

TEST_F(EditFileToolTest, testParseSearchReplaceBlocksWithSpecialCharacters)
{
    QString content = R"(<<<<<<< SEARCH
const regex = /[a-z]+/g;
const str = "Hello World";
=======
const regex = /[A-Za-z0-9]+/g;
const str = 'Hello World';
>>>>>>> REPLACE)";

    auto blocks = EditFileTool::parseSearchReplaceBlocks(content);
    
    ASSERT_EQ(blocks.size(), 1);
    EXPECT_EQ(blocks[0].searchContent, "const regex = /[a-z]+/g;\nconst str = \"Hello World\";");
    EXPECT_EQ(blocks[0].replaceContent, "const regex = /[A-Za-z0-9]+/g;\nconst str = 'Hello World';");
}

TEST_F(EditFileToolTest, testParseSearchReplaceBlocksMixedWithText)
{
    QString content = R"(Here is the first change:

```cpp
<<<<<<< SEARCH
int x = 1;
=======
int x = 2;
>>>>>>> REPLACE
```

And here is another change:

<<<<<<< SEARCH
int y = 3;
=======
int y = 4;
>>>>>>> REPLACE

Done!)";

    auto blocks = EditFileTool::parseSearchReplaceBlocks(content);
    
    ASSERT_EQ(blocks.size(), 2);
    EXPECT_EQ(blocks[0].searchContent, "int x = 1;");
    EXPECT_EQ(blocks[0].replaceContent, "int x = 2;");
    EXPECT_EQ(blocks[1].searchContent, "int y = 3;");
    EXPECT_EQ(blocks[1].replaceContent, "int y = 4;");
}

TEST_F(EditFileToolTest, testToolName)
{
    EditFileTool tool;
    EXPECT_EQ(tool.name(), "edit_file");
}

TEST_F(EditFileToolTest, testToolStringName)
{
    EditFileTool tool;
    EXPECT_EQ(tool.stringName(), "Editing file");
}

TEST_F(EditFileToolTest, testToolDescription)
{
    EditFileTool tool;
    QString desc = tool.description();
    
    // Verify key parts of the description are present
    EXPECT_TRUE(desc.contains("SEARCH/REPLACE"));
    EXPECT_TRUE(desc.contains("<<<<<<< SEARCH"));
    EXPECT_TRUE(desc.contains("======="));
    EXPECT_TRUE(desc.contains(">>>>>>> REPLACE"));
}

TEST_F(EditFileToolTest, testToolDefinitionHasRequiredProperties)
{
    EditFileTool tool;
    QJsonObject def = tool.getDefinition(QodeAssist::LLMCore::ToolSchemaFormat::OpenAI);
    
    EXPECT_TRUE(def.contains("properties"));
    QJsonObject props = def["properties"].toObject();
    
    EXPECT_TRUE(props.contains("filename"));
    EXPECT_TRUE(props.contains("content"));
    
    EXPECT_TRUE(def.contains("required"));
    QJsonArray required = def["required"].toArray();
    
    bool hasFilename = false;
    bool hasContent = false;
    for (const auto &val : required) {
        if (val.toString() == "filename") hasFilename = true;
        if (val.toString() == "content") hasContent = true;
    }
    EXPECT_TRUE(hasFilename);
    EXPECT_TRUE(hasContent);
}

#include "EditFileToolTest.moc"
