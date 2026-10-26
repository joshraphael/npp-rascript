# Change Log

All notable changes to the "npp-rascript" extension will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [[0.5.0](https://github.com/joshraphael/npp-rascript/releases/tag/v0.5.0)] - 2026-MM-DD

[diff](https://github.com/joshraphael/npp-rascript/compare/v0.4.1...v0.5.0)

### Added

- Support for non-ASCII letters in names
- Begin/end tokens, with tokens inside them, that can span lines
- Separate styles for each capture group of a token, and for the begin and end of begin/end tokens
- Reusable groups of tokens with a repository, includes and `$self`
- Debug builds with timing output for the lexer, which release builds refuse to compile
- VS Code clangd settings and code formatting for editing the source

### Changed

- RAScript.xml uses the TextMate grammar's rule structure
- Tokens are matched left to right and one line at a time, like TextMate, instead of painting over each other
- Faster styling: all tokens are searched with one combined regex, only the edited lines are restyled, and typing no longer redraws the whole view
- fix last character in a document not being styled
- fix RAScript.xml not being written or read when the plugin config folder path has non-ASCII characters
- Build with C++20
- Bump Notepad++ to v8.9.8.1
- Bump boostregex to 1.92.0
- Bump rascript-syntax to v0.5.0

### Removed

- `NORMAL` and `DELIMETER` token types, and the old `<Token type="...">` XML format
- Unused `GetLexerStatusText` export
- Unused plugin template code

## [[0.4.1](https://github.com/joshraphael/npp-rascript/releases/tag/v0.4.1)] - 2025-08-30

[diff](https://github.com/joshraphael/npp-rascript/compare/v0.4.0...v0.4.1)

### Added

### Changed

- Expanded document restyling when changing tabs and editing in Style Configuration

### Removed

## [[0.4.0](https://github.com/joshraphael/npp-rascript/releases/tag/v0.4.0)] - 2025-08-29

[diff](https://github.com/joshraphael/npp-rascript/compare/v0.3.0...v0.4.0)

### Added

- Split light and dark themes
- Syntax highlighting config tokens for: header MinimumVersion

### Changed

- Improved styling for each mode
- Upgraded boostregex to 1.89.0
- Optimize local build binary performance
- Bump rascript-syntax to v0.1.0

### Removed

## [[0.3.0](https://github.com/joshraphael/npp-rascript/releases/tag/v0.3.0)] - 2025-08-28

[diff](https://github.com/joshraphael/npp-rascript/compare/v0.2.1...v0.3.0)

### Added

- boostregex inclusion from Notepad++ source
- Config stylizing engine parser implementation
- Syntax highlighting config tokens for: single line comments, block comments, variables, header ID, functions, fuction calls, classes, this and control keywords
- Set background color to match the users current default 

### Changed

- Run lexer on entire document for every change instead of partial document

### Removed

## [[0.2.1](https://github.com/joshraphael/npp-rascript/releases/tag/v0.2.1)] - 2025-08-25

[diff](https://github.com/joshraphael/npp-rascript/compare/v0.2.0...v0.2.1)

### Added

- tinyxml2 v11.0.0 to parse config file

### Changed

- Fix 32-bit binaries not activating Lexer highlighting

### Removed

## [[0.2.0](https://github.com/joshraphael/npp-rascript/releases/tag/v0.2.0)] - 2025-08-24

[diff](https://github.com/joshraphael/npp-rascript/compare/v0.1.0...v0.2.0)

### Added

- Basic Lexer Integration
- Automatic config file generation in configs directory
- Changelog

### Changed

### Removed

## [[0.1.0](https://github.com/joshraphael/npp-rascript/releases/tag/v0.1.0)] - 2025-08-22

[diff](https://github.com/joshraphael/npp-rascript/compare/v0.0.1...v0.1.0)

### Added

- Documentation
- Sync git tag version with release binary code

### Changed

- Improved pipeline building and testing

### Removed

## [[0.0.1](https://github.com/joshraphael/npp-rascript/releases/tag/v0.0.1)] - 2025-08-21

[diff](https://github.com/joshraphael/npp-rascript/compare/4002a3053441a78435f6a6429a292a1e65da20c3...v0.0.1)

### Added

- Base plugin loading to Notepad++ API
- Automatic building and releasing of binary in pipeline

### Changed

### Removed