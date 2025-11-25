add_executable(QodeAssistTest
    ../CodeHandler.cpp
    ../LLMClientInterface.cpp
    ../tools/EditFileTool.cpp
    ../tools/FileSearchUtils.cpp
    CodeHandlerTest.cpp
    DocumentContextReaderTest.cpp
    EditFileToolTest.cpp
    # LLMClientInterfaceTests.cpp
    unittest_main.cpp
)

target_include_directories(QodeAssistTest PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(QodeAssistTest PRIVATE
    Qt::Core
    Qt::Test
    GTest::GTest
    GTest::gmock
    GTest::Main
    QtCreator::LanguageClient
    QtCreator::ProjectExplorer
    Context
    LLMCore
    QodeAssistSettings
)

target_compile_definitions(QodeAssistTest PRIVATE CMAKE_CURRENT_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

add_test(NAME QodeAssistTest COMMAND QodeAssistTest)
