cmake_minimum_required(VERSION 3.16)

project(QodeAssist)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(QtCreator REQUIRED COMPONENTS Core)
find_package(Qt6 COMPONENTS Core Gui Quick Widgets Network Test LinguistTools REQUIRED)
find_package(GTest)

# Add cpp-mcp library
include(FetchContent)
FetchContent_Declare(
    cpp-mcp
    GIT_REPOSITORY https://github.com/hkr04/cpp-mcp.git
    GIT_TAG main
)
FetchContent_MakeAvailable(cpp-mcp)

# Add Universal Ctags (like cpp-mcp - built from source)
FetchContent_Declare(
    universal-ctags
    GIT_REPOSITORY https://github.com/universal-ctags/ctags.git
    GIT_TAG master
)
FetchContent_MakeAvailable(universal-ctags)

# Build ctags from source if autotools are available
find_program(AUTORECONF autoreconf)
find_program(AUTOMAKE automake)
find_program(ACLOCAL aclocal)

if(AUTORECONF AND AUTOMAKE AND ACLOCAL)
    set(CTAGS_SOURCE_DIR ${universal-ctags_SOURCE_DIR})
    set(CTAGS_BUILD_DIR ${CMAKE_BINARY_DIR}/ctags-build)
    set(CTAGS_INSTALL_DIR ${CMAKE_BINARY_DIR}/ctags-install)

    # Create build directory
    file(MAKE_DIRECTORY ${CTAGS_BUILD_DIR})

    # Configure ctags build
    execute_process(
        COMMAND ${AUTORECONF} -f -i
        WORKING_DIRECTORY ${CTAGS_SOURCE_DIR}
        RESULT_VARIABLE AUTORECONF_RESULT
    )

    if(AUTORECONF_RESULT EQUAL 0)
        execute_process(
            COMMAND ${CTAGS_SOURCE_DIR}/configure --prefix=${CTAGS_INSTALL_DIR}
            WORKING_DIRECTORY ${CTAGS_BUILD_DIR}
            RESULT_VARIABLE CONFIGURE_RESULT
        )

        if(CONFIGURE_RESULT EQUAL 0)
            # Build ctags
            execute_process(
                COMMAND make -j${CMAKE_BUILD_PARALLEL_LEVEL}
                WORKING_DIRECTORY ${CTAGS_BUILD_DIR}
                RESULT_VARIABLE MAKE_RESULT
            )

            if(MAKE_RESULT EQUAL 0)
                # Install ctags
                execute_process(
                    COMMAND make install
                    WORKING_DIRECTORY ${CTAGS_BUILD_DIR}
                    RESULT_VARIABLE INSTALL_RESULT
                )

                if(INSTALL_RESULT EQUAL 0)
                    set(CTAGS_BINARY_PATH "${CTAGS_INSTALL_DIR}/bin/ctags")
                    message(STATUS "Successfully built ctags from source: ${CTAGS_BINARY_PATH}")

                    # Copy ctags binary to plugin directory after build
                    add_custom_command(TARGET QodeAssist POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            ${CTAGS_BINARY_PATH}
                            $<TARGET_FILE_DIR:QodeAssist>/ctags
                        COMMENT "Copying ctags binary to plugin directory"
                    )
                else()
                    message(WARNING "Failed to install ctags")
                endif()
            else()
                message(WARNING "Failed to build ctags")
            endif()
        else()
            message(WARNING "Failed to configure ctags")
        endif()
    else()
        message(WARNING "Failed to run autoreconf for ctags")
    endif()
else()
    message(WARNING "Autotools not found (autoreconf, automake, aclocal). ctags will not be built from source.")
endif()

# Check for system ctags as fallback
if(NOT CTAGS_BINARY_PATH)
    find_program(CTAGS_EXECUTABLE ctags)
    if(CTAGS_EXECUTABLE)
        message(STATUS "Using system ctags: ${CTAGS_EXECUTABLE}")
        set(CTAGS_BINARY_PATH "${CTAGS_EXECUTABLE}")
    else()
        message(WARNING "ctags not found. The CtagsTool will not be functional. Install universal-ctags to enable this feature.")
        set(CTAGS_BINARY_PATH "")
    endif()
endif()

qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES en)

# IDE_VERSION is defined by QtCreator package
string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" version_match ${IDE_VERSION})
set(QODEASSIST_QT_CREATOR_VERSION_MAJOR ${CMAKE_MATCH_1})
set(QODEASSIST_QT_CREATOR_VERSION_MINOR ${CMAKE_MATCH_2})
set(QODEASSIST_QT_CREATOR_VERSION_PATCH ${CMAKE_MATCH_3})

if(NOT version_match)
    message(FATAL_ERROR "Failed to parse Qt Creator version string: ${IDE_VERSION}")
endif()

message(STATUS "Qt Creator Version: ${QODEASSIST_QT_CREATOR_VERSION_MAJOR}.${QODEASSIST_QT_CREATOR_VERSION_MINOR}.${QODEASSIST_QT_CREATOR_VERSION_PATCH}")

add_definitions(
  -DQODEASSIST_QT_CREATOR_VERSION_MAJOR=${QODEASSIST_QT_CREATOR_VERSION_MAJOR}
  -DQODEASSIST_QT_CREATOR_VERSION_MINOR=${QODEASSIST_QT_CREATOR_VERSION_MINOR}
  -DQODEASSIST_QT_CREATOR_VERSION_PATCH=${QODEASSIST_QT_CREATOR_VERSION_PATCH}
)

add_subdirectory(llmcore)
add_subdirectory(settings)
add_subdirectory(logger)
add_subdirectory(UIControls)
add_subdirectory(ChatView)
add_subdirectory(context)
add_subdirectory(mcp)
if(GTest_FOUND)
  add_subdirectory(test)
endif()

add_qtc_plugin(QodeAssist
  PLUGIN_DEPENDS
    QtCreator::Core
    QtCreator::LanguageClient
    QtCreator::TextEditor
    QtCreator::ProjectExplorer
    QtCreator::CppEditor
  DEPENDS
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Widgets
    Qt::Network
    QtCreator::ExtensionSystem
    QtCreator::Utils
    QtCreator::CPlusPlus
    QodeAssistChatViewplugin
    mcp
    qodeassist-mcp
  SOURCES
    .github/workflows/build_cmake.yml
    .github/workflows/README.md
    README.md
    qodeassist.cpp
    QodeAssistConstants.hpp
    QodeAssisttr.h
    LLMClientInterface.hpp LLMClientInterface.cpp
    templates/Templates.hpp
    templates/CodeLlamaFim.hpp
    templates/Ollama.hpp
    templates/Claude.hpp
    templates/OpenAI.hpp
    templates/MistralAI.hpp
    templates/StarCoder2Fim.hpp
    # templates/DeepSeekCoderFim.hpp
    # templates/CustomFimTemplate.hpp
    templates/Qwen25CoderFIM.hpp
    templates/OpenAICompatible.hpp
    templates/Llama3.hpp
    templates/ChatML.hpp
    templates/Alpaca.hpp
    templates/Llama2.hpp
    templates/CodeLlamaQMLFim.hpp
    templates/GoogleAI.hpp
    templates/LlamaCppFim.hpp
    templates/Qwen3CoderFIM.hpp
    providers/Providers.hpp
    providers/OllamaProvider.hpp providers/OllamaProvider.cpp
    providers/ClaudeProvider.hpp providers/ClaudeProvider.cpp
    providers/OpenAIProvider.hpp providers/OpenAIProvider.cpp
    providers/MistralAIProvider.hpp providers/MistralAIProvider.cpp
    providers/LMStudioProvider.hpp providers/LMStudioProvider.cpp
    providers/OpenAICompatProvider.hpp providers/OpenAICompatProvider.cpp
    providers/OpenRouterAIProvider.hpp providers/OpenRouterAIProvider.cpp
    providers/GoogleAIProvider.hpp providers/GoogleAIProvider.cpp
    providers/LlamaCppProvider.hpp providers/LlamaCppProvider.cpp
    providers/CodestralProvider.hpp providers/CodestralProvider.cpp
    settings/MCPSettings.hpp settings/MCPSettings.cpp
    QodeAssist.qrc
    LSPCompletion.hpp
    LLMSuggestion.hpp LLMSuggestion.cpp
    RefactorSuggestion.hpp RefactorSuggestion.cpp
    RefactorSuggestionHoverHandler.hpp RefactorSuggestionHoverHandler.cpp
    QodeAssistClient.hpp QodeAssistClient.cpp
    chat/ChatOutputPane.h chat/ChatOutputPane.cpp
    chat/NavigationPanel.hpp chat/NavigationPanel.cpp
    ConfigurationManager.hpp ConfigurationManager.cpp
    CodeHandler.hpp CodeHandler.cpp
    UpdateStatusWidget.hpp UpdateStatusWidget.cpp
    widgets/CompletionProgressHandler.hpp widgets/CompletionProgressHandler.cpp
    widgets/CompletionErrorHandler.hpp widgets/CompletionErrorHandler.cpp
    widgets/CompletionHintWidget.hpp widgets/CompletionHintWidget.cpp
    widgets/CompletionHintHandler.hpp widgets/CompletionHintHandler.cpp
    widgets/ProgressWidget.hpp widgets/ProgressWidget.cpp
    widgets/ErrorWidget.hpp widgets/ErrorWidget.cpp
    widgets/EditorChatButton.hpp widgets/EditorChatButton.cpp
    widgets/EditorChatButtonHandler.hpp widgets/EditorChatButtonHandler.cpp
    widgets/QuickRefactorDialog.hpp widgets/QuickRefactorDialog.cpp
    widgets/CustomInstructionsManager.hpp widgets/CustomInstructionsManager.cpp
    widgets/AddCustomInstructionDialog.hpp widgets/AddCustomInstructionDialog.cpp

    QuickRefactorHandler.hpp QuickRefactorHandler.cpp
    tools/ToolsFactory.hpp tools/ToolsFactory.cpp
    tools/ReadVisibleFilesTool.hpp tools/ReadVisibleFilesTool.cpp
    tools/ToolHandler.hpp tools/ToolHandler.cpp
    tools/ListProjectFilesTool.hpp tools/ListProjectFilesTool.cpp
    tools/ToolsManager.hpp tools/ToolsManager.cpp
    tools/GetIssuesListTool.hpp tools/GetIssuesListTool.cpp
    tools/CreateNewFileTool.hpp tools/CreateNewFileTool.cpp
    tools/EditFileTool.hpp tools/EditFileTool.cpp
    tools/BuildProjectTool.hpp tools/BuildProjectTool.cpp
    tools/ExecuteTerminalCommandTool.hpp tools/ExecuteTerminalCommandTool.cpp
    tools/ProjectSearchTool.hpp tools/ProjectSearchTool.cpp
    tools/FindAndReadFileTool.hpp tools/FindAndReadFileTool.cpp
    tools/ReadFileLinesTool.hpp tools/ReadFileLinesTool.cpp
    tools/CtagsTool.hpp tools/CtagsTool.cpp
    tools/FileSearchUtils.hpp tools/FileSearchUtils.cpp
    providers/ClaudeMessage.hpp providers/ClaudeMessage.cpp
    providers/OpenAIMessage.hpp providers/OpenAIMessage.cpp
    providers/OllamaMessage.hpp providers/OllamaMessage.cpp
    providers/GoogleMessage.hpp providers/GoogleMessage.cpp
)


get_target_property(QtCreatorCorePath QtCreator::Core LOCATION)
find_program(QtCreatorExecutable
  NAMES
    qtcreator "Qt Creator"
  PATHS
    "${QtCreatorCorePath}/../../../bin"
    "${QtCreatorCorePath}/../../../MacOS"
  NO_DEFAULT_PATH
)
if (QtCreatorExecutable)
  add_custom_target(RunQtCreator
    COMMAND ${QtCreatorExecutable} -pluginpath $<TARGET_FILE_DIR:QodeAssist>
    DEPENDS QodeAssist
  )
  set_target_properties(RunQtCreator PROPERTIES FOLDER "qtc_runnable")
endif()

#TODO change to TS_OUTPUT_DIRECTORY after removing Qt6.8
qt_add_translations(TARGETS QodeAssist
          TS_FILE_DIR ${CMAKE_CURRENT_LIST_DIR}/resources/translations
          RESOURCE_PREFIX "/translations"
          LUPDATE_OPTIONS -no-obsolete
)
