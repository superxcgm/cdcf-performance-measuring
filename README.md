```shell script
$ conan remote add inexorgame "https://api.bintray.com/conan/inexorgame/inexor-conan"
$ conan remote add hpc "https://api.bintray.com/conan/grandmango/cdcf"
$ mkdir build && cd build
$ conan install .. --build missing
$ cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_paths.cmake
$ cmake --build . -j
```