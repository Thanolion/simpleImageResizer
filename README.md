# Simple Image Resizer

A fast, easy-to-use desktop application for batch image resizing and compression. Built with Qt 6 and C++.

<!-- ![Screenshot](docs/screenshot.png) -->

## Features

- **Batch processing** — resize and compress hundreds of images at once
- **Multiple resize modes** — percentage, fit width, fit height, fit box, or convert-only
- **Format conversion** — output to JPEG, PNG, WebP, or AVIF
- **RAW camera support** — load CR2, CR3, NEF, ARW, DNG, RAF, ORF, and more via LibRaw
- **Target file size** — automatically adjust quality to hit a specific file size (JPEG, WebP, AVIF)
- **Simple & Advanced modes** — Simple mode for quick presets, Advanced mode for full control
- **Configurable thread pool** — set the number of processing threads to balance speed and system load
- **Drag & drop** — drag files or folders directly into the app
- **Cross-platform** — builds on Windows, macOS, and Linux

## Building from Source

### Prerequisites

| Tool | Version |
|------|---------|
| Qt | 6.x (Widgets + Concurrent modules) |
| CMake | 3.16+ |
| Ninja | (recommended generator) |
| C++ compiler | C++20 support required |

LibRaw and libavif (with libaom) are fetched automatically by CMake — no manual installation needed.

Optionally install **NASM** or **YASM** for libaom SIMD optimizations (faster AVIF encoding). Without them, libaom falls back to generic C.

### Windows

1. Install [Qt 6](https://www.qt.io/download) (MSVC build), [Visual Studio](https://visualstudio.microsoft.com/) (or Build Tools), and ensure CMake + Ninja are on your PATH (Qt installer includes both).
2. Open a **Developer Command Prompt** or run `vcvarsall.bat x64`.
3. Set the `QTDIR` environment variable to your Qt installation (e.g. `C:\Qt\6.8.0\msvc2022_64`).
4. Build:
   ```bat
   cmake --preset x64-release
   cmake --build out/build/x64-release
   ```
   Or simply run `build.bat` / `build.ps1` from the project root.

#### Build Configuration

The build scripts auto-detect Visual Studio, Qt, CMake, and Ninja from common
installation paths. If auto-detection fails (e.g. custom install locations),
copy `build.env.example` to `build.env` and set paths for your machine:

```
cp build.env.example build.env
# Edit build.env with your paths
```

`build.env` is gitignored and will not be committed.

Available settings:
| Variable | Description | Example |
|----------|-------------|---------|
| `VS_PATH` | Visual Studio installation root | `C:\Program Files\Microsoft Visual Studio\2022\Community` |
| `QTDIR` | Qt SDK (version + compiler variant) | `C:\Qt\6.10.2\msvc2022_64` |
| `QT_TOOLS_ROOT` | Qt root (parent of `Tools\CMake_64`) | `C:\Qt` |

### macOS

1. Install Qt 6 via Homebrew:
   ```bash
   brew install qt@6 cmake ninja
   ```
2. Build:
   ```bash
   export QTDIR=$(brew --prefix qt@6)
   cmake --preset macos-release
   cmake --build out/build/macos-release
   ```
   Or run `./build.sh release`.

### Linux

1. Install Qt 6 development packages (e.g. on Ubuntu/Debian):
   ```bash
   sudo apt install qt6-base-dev libqt6concurrent6 cmake ninja-build g++
   ```
2. Build:
   ```bash
   export QTDIR=/usr  # or wherever Qt 6 is installed
   cmake --preset linux-release
   cmake --build out/build/linux-release
   ```
   Or run `./build.sh release`.

## Dependencies

| Library | Version | License |
|---------|---------|---------|
| [Qt](https://www.qt.io/) | 6.x | LGPL v3 |
| [LibRaw](https://www.libraw.org/) | 0.21.3 (fetched automatically) | LGPL v2.1 / CDDL v1.0 |
| [libavif](https://github.com/AOMediaCodec/libavif) | 1.1.1 (fetched automatically) | BSD 2-Clause |
| [libaom](https://aomedia.googlesource.com/aom/) | bundled with libavif | BSD 2-Clause |

## License

This project is licensed under the **GNU General Public License v3.0** — see the [LICENSE](LICENSE) file for details.

Qt is used under the LGPL v3 license. LibRaw is used under the LGPL v2.1 license. libavif and libaom are used under BSD 2-Clause licenses. Full license texts are included in the `licenses/` directory.

## Support Development

If you find Simple Image Resizer useful, consider supporting its development:

- [GitHub Sponsors](https://github.com/sponsors/thanolion)
- [Ko-fi](https://ko-fi.com/cullen38127)
