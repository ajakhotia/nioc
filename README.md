# naksh

# Clang Tidy
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
