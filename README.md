# aab2apk - Ultra-Fast AAB to APK Converter

A production-ready, high-performance CLI tool for converting Android App Bundles (.aab) to APK files. Built with C++17 for maximum speed and reliability.

## Features

- ⚡ **Ultra-fast conversion** - Minimal overhead, optimized execution
- 🔒 **APK signing support** - Integrated keystore-based signing
- 📦 **Multiple output modes** - Universal APK or split APKs
- 🔐 **Secure credential handling** - Environment variable support for passwords
- 🖥️ **Cross-platform** - Windows, macOS, and Linux support
- ✅ **Production-ready** - Zero critical errors, deterministic output
- 🚀 **CI/CD friendly** - Machine-readable errors, proper exit codes

## Requirements

### Build Dependencies

- **C++17 compatible compiler:**
  - GCC 7+ (Linux)
  - Clang 5+ (macOS/Linux)
  - MSVC 2017+ (Windows)

- **CMake 3.15+**

- **C++17 Standard Library** with filesystem support

### Runtime Dependencies

- **Java 8+** (JRE or JDK)
  - Must be in PATH or set via `JAVA_HOME`
  - Can be specified with `--java` flag

- **bundletool.jar**
  - Official Google bundletool
  - Download from: https://github.com/google/bundletool/releases
  - Can be auto-detected or specified with `--bundletool` flag

- **apksigner** (for APK signing)
  - Part of Android SDK Build Tools
  - Must be in PATH or `ANDROID_HOME` environment variable set
  - Required only when using `--keystore` option

## Building

### Linux/macOS

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Windows

```powershell
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

Or using MinGW:

```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build . --config Release
```

### Installation

#### Linux/macOS

```bash
cmake --install . --prefix /usr/local
```

#### Windows

**Option 1: Using Installer (Recommended)**

1. Download `aab2apk-setup-1.0.0.exe` from releases
2. Run the installer as Administrator
3. Open a new Command Prompt
4. Test: `aab2apk --version`

The installer automatically adds `aab2apk.exe` to your system PATH.

**Option 2: Manual Installation**

```powershell
cmake --install . --prefix "C:\Program Files\aab2apk"
```

Then add `C:\Program Files\aab2apk\bin` to your system PATH manually.

## Usage

### Basic Conversion

Convert AAB to universal APK:

```bash
aab2apk -i app.aab -o ./dist --mode universal
```

Convert AAB to split APKs:

```bash
aab2apk -i app.aab -o ./dist --mode split
```

### With APK Signing

Using direct passwords:

```bash
aab2apk \
  -i app.aab \
  -o ./dist \
  --mode universal \
  --keystore release.jks \
  --ks-pass "mykeystorepassword" \
  --key-alias release \
  --key-pass "mykeypassword"
```

Using environment variables (recommended for CI/CD):

```bash
export KS_PASS="mykeystorepassword"
export KEY_PASS="mykeypassword"

aab2apk \
  -i app.aab \
  -o ./dist \
  --mode universal \
  --keystore release.jks \
  --ks-pass env:KS_PASS \
  --key-alias release \
  --key-pass env:KEY_PASS
```

### Advanced Options

Specify custom Java or bundletool paths:

```bash
aab2apk \
  -i app.aab \
  -o ./dist \
  --java /usr/lib/jvm/java-11-openjdk/bin/java \
  --bundletool /opt/bundletool/bundletool.jar
```

Verbose output:

```bash
aab2apk -i app.aab -o ./dist -v
```

Quiet mode (errors only):

```bash
aab2apk -i app.aab -o ./dist -q
```

## Command-Line Options

### Required

- `-i, --input <path>` - Input .aab file path

### Optional

- `-o, --output <path>` - Output directory (default: `./dist`)
- `-m, --mode <mode>` - Output mode: `universal` or `split` (default: `universal`)
- `--keystore <path>` - Keystore file path for signing
- `--ks-pass <password>` - Keystore password (or `env:VAR_NAME`)
- `--key-alias <alias>` - Key alias
- `--key-pass <password>` - Key password (or `env:VAR_NAME`)
- `--bundletool <path>` - Path to bundletool.jar (auto-detected if not specified)
- `--java <path>` - Path to java executable (auto-detected if not specified)
- `-v, --verbose` - Verbose output
- `-q, --quiet` - Quiet mode (errors only)
- `-h, --help` - Show help message
- `--version` - Show version information

## Environment Variables

The tool supports reading passwords from environment variables using the `env:` prefix:

```bash
--ks-pass env:MY_KEYSTORE_PASSWORD
--key-pass env:MY_KEY_PASSWORD
```

This is the recommended approach for CI/CD pipelines to avoid exposing secrets in command history or logs.

## Auto-Detection

The tool automatically detects:

1. **Java executable:**
   - Checks `JAVA_HOME` environment variable
   - Searches in `PATH`

2. **bundletool.jar:**
   - Current directory
   - `./bundletool/bundletool.jar`
   - Common installation locations
   - `PATH` environment variable

3. **apksigner:**
   - `ANDROID_HOME/build-tools/*/lib/apksigner`
   - `PATH` environment variable

## Output

### Universal APK Mode

Creates a single APK file in the output directory:
```
dist/
  └── app.apk
```

### Split APK Mode

Creates multiple APK files for different ABIs, densities, and languages:
```
dist/
  ├── base.apk
  ├── base-arm64_v8a.apk
  ├── base-armeabi-v7a.apk
  ├── base-xxhdpi.apk
  └── ...
```

## Error Handling

The tool follows POSIX conventions for exit codes:

- `0` - Success
- `1` - General error (invalid arguments, file not found, etc.)
- Non-zero - Process execution failure

All errors are written to `stderr` with clear, actionable messages. In quiet mode (`-q`), only errors are displayed.

## Security Considerations

1. **Password Handling:**
   - Passwords are never logged or displayed
   - Use environment variables for CI/CD pipelines
   - Passwords are passed securely to subprocesses

2. **File Validation:**
   - AAB files are validated before processing
   - Keystore files are verified for existence
   - Output directories are created securely

3. **Temporary Files:**
   - Temporary directories are created in system temp location
   - All temporary files are cleaned up automatically
   - Uses RAII for guaranteed cleanup

## Performance Considerations

The tool is optimized for speed:

- **Minimal process overhead** - Single Java process invocation
- **Efficient file I/O** - Direct filesystem operations
- **No redundant validations** - Validates only when necessary
- **Deterministic execution** - Predictable runtime
- **Zero memory leaks** - RAII and modern C++ practices

Typical conversion times:
- Universal APK: 2-5 seconds (depending on AAB size)
- Split APKs: 3-8 seconds (depending on AAB size and splits)

## Architecture

### Design Principles

- **Separation of Concerns** - Clear module boundaries
- **RAII** - Automatic resource management
- **Move Semantics** - Efficient object transfers
- **Zero Leaks** - Guaranteed cleanup
- **Fail Fast** - Early validation and error reporting

### Module Structure

- `config.h/cpp` - CLI argument parsing and configuration
- `file_utils.h/cpp` - File system operations and validation
- `process_runner.h/cpp` - Cross-platform subprocess execution
- `signing.h/cpp` - APK signing integration
- `aab_converter.h/cpp` - Core conversion logic
- `main.cpp` - Entry point and orchestration

### Cross-Platform Support

The tool uses platform-specific implementations for:

- **Process execution** - Windows (`CreateProcess`) vs Unix (`fork/exec`)
- **Path handling** - Windows (`\`) vs Unix (`/`)
- **Temporary directories** - Platform-specific temp locations
- **Executable detection** - `.exe` extension on Windows

## Troubleshooting

### "bundletool.jar not found"

1. Download bundletool from: https://github.com/google/bundletool/releases
2. Place it in the current directory, or
3. Add it to PATH, or
4. Specify path with `--bundletool`

### "Java executable not found"

1. Install Java 8+ (JRE or JDK)
2. Set `JAVA_HOME` environment variable, or
3. Add Java to PATH, or
4. Specify path with `--java`

### "apksigner not found" (when signing)

1. Install Android SDK Build Tools
2. Set `ANDROID_HOME` environment variable, or
3. Add `build-tools/*/lib` to PATH

### "Invalid AAB file"

- Ensure the file has `.aab` extension
- Verify the file is a valid ZIP archive (AAB files are ZIP-based)
- Check file permissions

### "Keystore file does not exist"

- Verify the keystore path is correct
- Use absolute paths if relative paths fail
- Check file permissions

## Examples

### CI/CD Pipeline (GitHub Actions)

```yaml
- name: Convert AAB to APK
  env:
    KS_PASS: ${{ secrets.KEYSTORE_PASSWORD }}
    KEY_PASS: ${{ secrets.KEY_PASSWORD }}
  run: |
    aab2apk \
      -i app-release.aab \
      -o ./apks \
      --mode universal \
      --keystore release.jks \
      --ks-pass env:KS_PASS \
      --key-alias release \
      --key-pass env:KEY_PASS
```

### CI/CD Pipeline (GitLab CI)

```yaml
convert_aab:
  script:
    - aab2apk -i app.aab -o ./dist --mode universal
  artifacts:
    paths:
      - dist/
```

### Local Development

```bash
# Quick test conversion
aab2apk -i debug.aab -o ./test-apk

# Production release with signing
aab2apk \
  -i release.aab \
  -o ./release-apks \
  --mode split \
  --keystore ~/keystores/release.jks \
  --ks-pass env:RELEASE_KS_PASS \
  --key-alias release \
  --key-pass env:RELEASE_KEY_PASS \
  -v
```

## License

This project is provided as-is for professional use. Ensure compliance with Android SDK and bundletool licensing terms.

## Contributing

This is a production tool. All contributions must:

- Maintain zero critical errors
- Pass all compiler warnings
- Follow C++17 best practices
- Include appropriate error handling
- Maintain cross-platform compatibility

## Version

Current version: 1.0.0

## Support

For issues related to:
- **bundletool**: See https://github.com/google/bundletool
- **apksigner**: See Android SDK documentation
- **This tool**: Check error messages and troubleshooting section

---

**Note**: This tool uses the official Google bundletool. It does not reverse-engineer or modify the AAB format. All conversions are performed through the official bundletool API.

