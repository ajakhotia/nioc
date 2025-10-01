[![infra-congruency-check](https://github.com/ajakhotia/nioc/actions/workflows/infra-congruency-check.yaml/badge.svg)](https://github.com/ajakhotia/nioc/actions/workflows/infra-congruency-check.yaml) [![docker-image](https://github.com/ajakhotia/nioc/actions/workflows/docker-image.yaml/badge.svg)](https://github.com/ajakhotia/nioc/actions/workflows/docker-image.yaml)

# nioc — Nerve IO Core Library

Core C++ utilities for building high-performance robotics and AI applications on edge devices.

---

## 🛠️ Setup

**The following instructions have been tested on Ubuntu 22.04 and Ubuntu 24.04.
Read the docker/ubuntu.dockerfile for details.**

### 📂 Clone

Before getting started, define three paths and ensure you have read and write
permission for each. These paths are referenced throughout the rest of this
document using the following tokens. Substitute your actual paths wherever these
tokens appear.

#### SOURCE_DIR

Path where you will clone the nioc project. This may be a temporary
directory if you only plan to build once. Examples:

- `"${HOME}/sandbox/nioc"`
- `"/tmp/nioc"`

#### BUILD_DIR

Path where you will create the build tree. This may also be temporary if you are
not iterating on builds. Examples:

- `"${SOURCE_DIR}/build"`
- `"/tmp/nioc-build"`
- `"${HOME}/sandbox/nioc-build"`

#### INSTALL_DIR

Path where installation artifacts will be placed. Keep this directory long-term;
it will contain executables, libraries, and supporting files. Examples:

- `"${HOME}/usr"`
- `"${HOME}/opt"`
- `"/opt/nioc"` (requires `sudo` during the build step)
- `"/usr"` (requires `sudo` during the build step)

NOTE: The build step of nioc (which is a super-build) triggers the
download, configure, build, and install steps of all the child libraries. Hence,
`sudo` is needed during the build step when installing to a location that
requires superuser privileges to write to. As a general rule prefer to install
to locations that do not require extra privileges.

**NOTE: You may export these paths as environment variables in your current
terminal context if you prefer**

```shell
export SOURCE_TREE=${HOME}/sandbox/nioc
export BUILD_TREE=${HOME}/sandbox/nioc/build
export INSTALL_TREE=${HOME}/opt/nioc
```

Clone the `nioc` project using the following:

```shell
git clone git@github.com:ajakhotia/nioc.git ${SOURCE_TREE}
```

### 🔧 Install tools

Install `jq` so that we can extract the list of system dependencies from the
[systemDependencies.json](systemDependencies.json) file.

```shell
sudo apt install -y --no-install-recommends jq
```

Install `cmake`. You may skip this if your OS-default cmake version is > 3.27

```shell
sudo bash tools/installCMake.sh
```

Install basic build tools:

```shell
sudo apt install -y --no-install-recommends \
  $(sh tools/apt/extractDependencies.sh Basics systemDependencies.json)
```

Set up apt-sources for the latest compilers / toolchains. Prefer to skip this if
the default OS-provided compilers / toolchains are new enough. Note the
following constraints:

* GNU compilers >= version 12
* LLVM compilers >= version 19
* Cuda toolkit >= version 13

You are responsible for installing the appropriate compilers / toolchains
yourself if you are skipping the commands below.

```shell
sudo bash tools/apt/addGNUSources.sh -y
```

```shell
sudo bash tools/apt/addLLVMSources.sh -y
```

```shell
sudo bash tools/apt/addNvidiaSources.sh -y
```

```shell
sudo apt update && sudo apt install -y --no-install-recommends $(sh tools/apt/extractDependencies.sh Compilers systemDependencies.json)
```

### External Dependencies

`nioc` depends on the following external libraries:

* Boost (headers + iostreams)
* Cap'n Proto
* Eigen3
* GoogleTest
* Nlohmann JSON
* Spdlog

It's recommended to use [robotFarm](https://github.com/ajakhotia/robotFarm) to build these. Here is
how it can be done using the quick start instructions:

```shell
export ROBOT_FARM_INSTALL_TREE=/opt/robotFarm
```

```shell
curl -fsSL                                                                                          \
  https://raw.githubusercontent.com/ajakhotia/robotFarm/refs/heads/main/tools/quickBuild.sh |       \
  sudo bash -s --                                                                                   \
    --version v1.1.0                                                                                \
    --prefix ${ROBOT_FARM_INSTALL_TREE}                                                             \
    --build-list "BoostExternalProject;CapnprotoExternalProject;Eigen3ExternalProject;GoogleTestExternalProject;NlohmannJsonExternalProject;SpdLogExternalProject"
```

For more details, see the [robotFarm](https://github.com/ajakhotia/robotFarm) README.

---

## 🛠️ Build Instructions

### Clone

```bash
git clone git@github.com:ajakhotia/nioc.git <SOURCE_TREE>
```

### Configure

Use CMake (with Ninja recommended):

```bash
cmake                                                       \
    -G Ninja                                                \
    -S ${SOURCE_TREE}                                       \
    -B ${BUILD_TREE}                                        \
    -DCMAKE_BUILD_TYPE:STRING="Release"                     \
    -DCMAKE_INSTALL_PREFIX:PATH=${INSTALL_TREE}             \
    -DBUILD_SHARED_LIBS:BOOL=ON                             \
    -DCMAKE_PREFIX_PATH:PATH=${ROBOT_FARM_INSTALL_TREE}
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

[MIT](LICENSE) © 2025 Anurag Jakhotia with restrictions on commercial use.
