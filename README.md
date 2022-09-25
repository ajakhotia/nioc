# `nioc` - naksh.io core library
Core utilities for developing connected naksh.io applications.


# Clone, Build, and Install
Before proceeding, come-up with values for the following:

* `<SOURCE_TREE>`: Path where you would like to clone the source tree of `nioc`.
  Recommendations:
  * `${HOME}/sandbox/nioc`
  * `${HOME}/work/nioc`
  * etc ...

* `<BUILD_TREE>`: Path were you'd like to host the build tree for `nioc`. You
  may want to have multiple build trees depending on the configurations. 
  Recommendation:
  * `<SOURCE_TREE>/build/<config_name>`
  * The `<config_name>` token is to be replaced with a name for the config
    you are trying to build. Eg: `release`, `debug`, etc ...

* `<INSTALL_TREE>`: Path where you'd like to install the built binaries, 
  libraries, header, executables, etc. Recommendation:
  * `${HOME}/opt/nioc`
  * `/opt/nioc` (Requires super user privileges during install step)
  * `/usr` (default install prefix for cmake. Requires super user privileges 
    during install step)
  * etc ...


## Dependencies
`nioc` depends on following libraries which must be made available before
building `nioc`.

### System Dependencies
* clang-14 (alternative to gcc / g++)
* clang-format-14 (optional)
* clang-tidy-14 (optional)
* cmake
* g++-12
* gcc-12
* git
* ninja-build

On `ubuntu 22.04`, these may be installed using:
```shell
sudo apt install -y clang-14 clang-format-14 clang-tidy-14 cmake g++-12 gcc-12 git ninja-build
```

### External Dependencies
* Boost
  * headers
  * iostreams
* Cap'n' proto
* Eigen3
* GoogleTest (optional, mandatory for build unit tests)
* Nlohmann's Json
* Spdlog

It's recommended to build these dependencies using
[naksh-io/robotFarm](https://github.com/naksh-io/robotFarm). Follow the instructions in 
`robotFarm`'s README. Use the following cache argument during the cmake configuration step
of `robotFarm` to build and install the necessary dependencies of `nioc`:

```shell
-DROBOT_FARM_REQUESTED_BUILD_LIST="BoostExternalProject;CapnprotoExternalProject;Eigen3ExternalProject;GoogleTestExternalProject;NlohmannJsonExternalProject;SpdLogExternalProject"
```

During the process of building and installation of external dependencies, **robotFarm** will
require you to provide an installation path (via `-DCMAKE_INSTALL_PREFIX:PATH=...` cmake 
cache argument). Remember this path, which will be referred to as `<ROBOT_FARM_INSTALL_TREE>`
throughout the rest of this document. 


## Clone
Clone the source tree using the following command:
```shell
git clone git@github.com:naksh-io/nioc.git <SOURCE_TREE>/..
```


## Build
`nioc` uses cmake as its build system. Configure it using the following command:
```shell
cmake -S <SOURCE_TREE> -B <BUILD_TREE>          \
    -G Ninja                                    \
    -DCMAKE_BUILD_TYPE:STRING="Release"         \
    -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_TREE>  \
    -DBUILD_SHARED_LIBS:BOOL=ON                 \
    -DCMAKE_PREFIX_PATH:PATH=<ROBOT_FARM_INSTALL_TREE>
```
* You may change "Release" to "Debug" if you would like debug builds instead of release builds.
* You may set `-DBUILD_SHARED_LIBS` to `OFF` if you want to build static libraries instead
  * This requires that the static version of the external dependencies are also available.

Once the configuration is complete, you can build `nioc` using the following command:
```shell
cmake --build <BUILD_TREE>
```


## Install
To install, use the following command:
```shell
cmake --install <BUILD_TREE>
```

# Contribution Guidelines

## Naming Conventions
* Member variables: mCamelCase
* Compile time constants: kCamelCase
* Types: PascalCase
* Everything else, including filenames: camelCase


# Tooling

### Clang Format
`nioc` will automatically find `clang-format-14` and create a custom cmake target(`niocClangFormat`)
which when built will format C/C++ code throughout the repository.


### Clang Tidy
Command used to generate .clang-tidy config file:
```shell
clang-tidy-12 -checks=android-*,bugprone-*,cert-*,cppcoreguidelines-*,hicpp-*,misc-*,modernize-*,openmp-*,performance-*,portability-*,readability-* --dump-config >> .clang-tidy
```

Following CMake cache arguments can be used to run clang-tidy with
compilation database:
```shell
-DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON
-DCMAKE_CXX_CLANG_TIDY:PATH=/path/to/clang-tidy-exe
```
