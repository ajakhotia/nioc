# nioc - nioc.io core library

## Usage

### Dependencies
* Boost
  * headers
  * iostreams
* Cap'n' proto
* Eigen3
* GoogleTest (optional, mandatory for build unit tests)
* Nlohmann's Json
* Spdlog

It's recommended to build these dependencies using
[nioc-io/robotFarm](https://github.com/nioc-io/robotFarm). Follow the instructions in 
robotFarm's README. In addition, provide the following cache argument to robotFarm 
during the cmake configuration step:
```shell
-DROBOT_FARM_REQUESTED_BUILD_LIST="BoostExternalProject;CapnprotoExternalProject;Eigen3ExternalProject;GoogleTestExternalProject;NlohmannJsonExternalProject;SpdLogExternalProject"
```



## Tooling

### Clang Format
**nioc** will automatically find **clang-format-14** and create a custom cmake target
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
