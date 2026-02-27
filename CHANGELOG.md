# Changelog

All notable changes to Simple Image Resizer are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [1.0.3] - 2026-02-27

### Added
- Simple/Advanced settings tabs — Simple mode offers casual-friendly dropdowns for format, resize presets, and quality; Advanced mode exposes all controls
- Configurable CPU thread pool — set the number of processing threads (defaults to cores minus one)
- Thread count and last active tab persist across sessions

### Fixed
- Open Output Folder button now works correctly when using the Simple settings tab
- Open Output Folder opens the first input file's folder when no explicit output directory is set
- Open Output Folder shows a status message when the folder doesn't exist or no input files are added

## [1.0.2] - 2026-02-26

### Added
- Cooperative cancellation — processing jobs now exit early at multiple checkpoints instead of running to completion when cancelled
- Cancellation summary showing how many files completed before cancellation
- libaom LICENSE copied to output for distribution compliance

### Changed
- AVIF encoding now uses YUV420 pixel format for significantly better compression
- AVIF encoder tuned for faster encodes (speed 8, 2 threads, auto tiling)

### Fixed
- AVIF encoder/decoder null-pointer checks for robustness
- AVIF file write now verifies all bytes were written
- AVIF encode failure at minimum quality now reports a proper error
- CI: NASM added to PATH after install so libaom can find it
- Forward-declare `FormatGuideDialog` to fix header dependency

## [1.0.1] - 2026-02-26

### Fixed
- CI: Add `qtimageformats` module for WebP support in release builds

## [1.0.0] - 2026-02-26

### Added
- Batch image resizing and compression with drag-and-drop support
- Resize modes: Percentage, Fit Width, Fit Height, Fit Bounding Box, No Resize (convert-only)
- Output formats: JPEG, PNG, WebP
- Quality slider and target file size with binary-search compression
- RAW camera image loading (CR2, NEF, ARW, DNG, etc.) via LibRaw
- Persistent settings between sessions
- Cross-platform CI/CD for Windows, macOS, and Linux

[1.0.3]: https://github.com/Thanolion/simpleImageResizer/compare/v1.0.2...v1.0.3
[1.0.2]: https://github.com/Thanolion/simpleImageResizer/compare/v1.0.1...v1.0.2
[1.0.1]: https://github.com/Thanolion/simpleImageResizer/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/Thanolion/simpleImageResizer/releases/tag/v1.0.0
