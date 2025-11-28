# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### 2025-11-28

#### Changed

- Updated default LLM model for Code completion, chat assistant and other settings
- Updated .qodeassistignore reference with .h2loopignore
- Added 'Refresh MCP servers' button to MCP settings

#### Fixed

- Fixed key handler on chat; Enter to send message & Ctrl+Enter to add new line
- Updated token count on each message, tool call input, output
- Moved MCP status check logic from app startup to background (faster app startup)

#### Added

- Added MCP tool connection using mcp-cpp library
- Provided an interface to connect/remove MCP server via MCP settings
- Displayed available MCP tools and sync the tools with chat
- Added Debug logger and write the logs on a file
- Synchronized our repo with the latest upstream repository code
