# MGSPlayer

MGSPlayer is a high-fidelity, cross-platform console-mode application designed to play MSX music files (`.MGS`, `.KSS`, `.BGM`, `.OPX`) on modern operating systems like Linux and Windows without the need for actual MSX hardware.

It utilizes the powerful **libkss** emulation core to accurately reproduce the sound of classic MSX chips, including PSG (AY-3-8910), SCC/SCC+, and OPLL (YM2413).

## Key Features

- **High-Quality Audio**: Optimized for 44.1kHz stereo output with high-quality interpolation and DC filtering.
- **Japanese Title Support**: Automatically converts Shift-JIS titles found in MGS files to UTF-8 for correct display on Linux and Windows consoles.
- **Debug Mode**: Includes a `-debug` flag to render audio directly to a `.wav` file for analysis without real-time playback.
- **Cross-Platform**: Seamlessly works on Linux and Windows.
- **Portable Binary**: The Windows version is built as a fully static, single `.exe` file with no external DLL dependencies.

## Architecture

- **Audio Engine**: Powered by [libkss](https://github.com/digital-sound-antiques/libkss) for Z80 and sound chip emulation.
- **Output Layer**: Uses **SDL2** for low-latency audio callback and device management.
- **Encoding**: Native Windows API and `iconv` are used for Japanese text handling.

## Build Instructions

### Prerequisites

- **CMake** (3.10 or higher)
- **C++17 Compiler** (GCC, Clang, or MSVC)
- **SDL2 Development Libraries**
- **Git**

### Building on Linux

1. **Install SDL2 development headers**:
   ```bash
   sudo apt-get install libsdl2-dev
   ```

2. **Clone the repository**:
   ```bash
   git clone --recursive https://github.com/meesokim/mgsplayer.git
   cd mgsplayer
   ```

3. **Build libkss**:
   ```bash
   cd libkss && mkdir build && cd build
   cmake .. && make
   cd ../..
   ```

4. **Build MGSPlayer**:
   ```bash
   mkdir build && cd build
   cmake .. && make
   ```

### Cross-Compiling for Windows (from Linux)

The project includes a pre-configured toolchain and script for generating a standalone Windows binary.

1. Ensure `x86_64-w64-mingw32-g++` is installed.
2. Run the provided build script:
   ```bash
   ./build-windows.sh
   ```
3. The standalone binary will be generated at `dist-win/mgsplayer.exe`.

## Usage

Run the player from the console by providing an MGS file path:

```bash
./mgsplayer your_music.mgs
```

### Options

- `-debug`: Renders the audio directly to `debug_output.wav` and exits immediately (useful for analysis).

```bash
./mgsplayer -debug your_music.mgs
```

## Credits

- **Emulation Core**: [libkss](https://github.com/digital-sound-antiques/libkss) by digital-sound-antiques.
- **Developer**: [meesokim](https://github.com/meesokim)
