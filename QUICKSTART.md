# Quick Start Guide

## Prerequisites Check

Before building, ensure you have:

1. **C++17 Compiler** - Check with:
   ```bash
   g++ --version  # or clang++ --version on macOS
   ```

2. **CMake 3.15+** - Check with:
   ```bash
   cmake --version
   ```

3. **Java 8+** - Check with:
   ```bash
   java -version
   ```

4. **bundletool.jar** - Download from:
   - https://github.com/google/bundletool/releases
   - Place in current directory or add to PATH

## 5-Minute Build

```bash
# 1. Create build directory
mkdir build && cd build

# 2. Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# 3. Build
cmake --build . --config Release

# 4. Test
./aab2apk --version
```

## First Conversion

```bash
# Basic conversion (universal APK)
./aab2apk -i your-app.aab -o ./output

# With signing
./aab2apk \
  -i your-app.aab \
  -o ./output \
  --keystore release.jks \
  --ks-pass env:KS_PASS \
  --key-alias release \
  --key-pass env:KEY_PASS
```

## Common Issues

### "bundletool.jar not found"
```bash
# Download and place in current directory
wget https://github.com/google/bundletool/releases/latest/download/bundletool-all.jar
mv bundletool-all.jar bundletool.jar

# Or specify path
./aab2apk -i app.aab --bundletool /path/to/bundletool.jar
```

### "Java not found"
```bash
# Set JAVA_HOME (Linux/macOS)
export JAVA_HOME=/usr/lib/jvm/java-11-openjdk

# Or specify path
./aab2apk -i app.aab --java /usr/bin/java
```

### "filesystem not found" (GCC < 9)
```bash
cmake .. -DCMAKE_CXX_FLAGS="-lstdc++fs"
```

## Next Steps

- See [README.md](README.md) for full documentation
- See [BUILD.md](BUILD.md) for detailed build instructions
- Check examples in README.md for CI/CD integration

