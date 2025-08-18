# nioc — Nerve IO Core Library  
Core C++ utilities for building high-performance robotics and AI applications on edge devices.

---

## 🚀 Getting Started

### Clone, Build, and Install

Before proceeding, decide on the following paths:

- **`<SOURCE_TREE>`**: Directory to clone the source tree.  
  Examples:  
  - `${HOME}/sandbox/nioc`  
  - `${HOME}/work/nioc`  

- **`<BUILD_TREE>`**: Directory to host the build tree (you may have multiple for different configurations).  
  Examples:  
  - `<SOURCE_TREE>/build/release`  
  - `<SOURCE_TREE>/build/debug`  

- **`<INSTALL_TREE>`**: Directory to install binaries, libraries, headers, etc.  
  Examples:  
  - `${HOME}/opt/nioc`  
  - `/opt/nioc` (requires `sudo`)  
  - `/usr` (default CMake install prefix, requires `sudo`)  

---

## 📦 Dependencies

`nioc` requires the following dependencies to be available before building.

### System Dependencies
- clang-14 (alternative to gcc/g++)
- clang-format-14 (optional)
- clang-tidy-14 (optional)
- cmake
- g++-12
- gcc-12
- git
- ninja-build

On **Ubuntu 22.04**, install them with:  
```bash
sudo apt install -y clang-14 clang-format-14 clang-tidy-14 cmake g++-12 gcc-12 git ninja-build
````

### External Dependencies

* Boost (headers + iostreams)
* Cap'n Proto
* Eigen3
* GoogleTest *(optional, required for unit tests)*
* Nlohmann JSON
* Spdlog

We recommend using [robotFarm](https://github.com/ajakhotia/robotFarm) to build these.
During CMake configuration for **robotFarm**, pass the following cache argument to build and install 
the dependencies required by `nioc`:

```bash
-DROBOT_FARM_REQUESTED_BUILD_LIST="BoostExternalProject;CapnprotoExternalProject;Eigen3ExternalProject;GoogleTestExternalProject;NlohmannJsonExternalProject;SpdLogExternalProject"
```

> 🔑 Remember the installation path you provide via `-DCMAKE_INSTALL_PREFIX:PATH=...` when building `robotFarm`.
> This path will be referred to as **`<ROBOT_FARM_INSTALL_TREE>`** in the rest of the README.

---

## 🛠️ Build Instructions

### Clone

```bash
git clone git@github.com:ajakhotia/nioc.git <SOURCE_TREE>
```

### Configure

Use CMake (with Ninja recommended):

```bash
cmake -S <SOURCE_TREE> -B <BUILD_TREE>          \
    -G Ninja                                    \
    -DCMAKE_BUILD_TYPE:STRING="Release"         \
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_TREE>  \
    -DBUILD_SHARED_LIBS:BOOL=ON                 \
    -DCMAKE_PREFIX_PATH:PATH=<ROBOT_FARM_INSTALL_TREE>
```

Notes:

* Set `CMAKE_BUILD_TYPE` to `"Debug"` for debug builds.
* Set `BUILD_SHARED_LIBS=OFF` for static builds (requires static versions of dependencies).

### Build

```bash
cmake --build <BUILD_TREE>
```

### Install

```bash
cmake --install <BUILD_TREE>
```

---

## 🤝 Contributing

We welcome contributions! Please follow the project conventions when submitting PRs.

### Naming Conventions

* **Member variables**: `mCamelCase`
* **Compile-time constants**: `kCamelCase`
* **Types**: `PascalCase`
* **Everything else (including filenames)**: `camelCase`

---

## 🧰 Tooling

### Clang Format

CMake automatically detects `clang-format-14` and creates the target `niocClangFormat`.
Run it to format all C/C++ code in the repository.

### Clang Tidy

Generate `.clang-tidy` with:

```bash
clang-tidy-12 -checks=android-*,bugprone-*,cert-*,cppcoreguidelines-*,hicpp-*,misc-*,modernize-*,openmp-*,performance-*,portability-*,readability-* --dump-config >> .clang-tidy
```

Enable clang-tidy in CMake with:

```bash
-DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON
-DCMAKE_CXX_CLANG_TIDY:PATH=/path/to/clang-tidy-exe
```

---

## 📜 License

[MIT](LICENSE) © 2025 Anurag Jakhotia
