set -e
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug
cd build/debug
LIBGL_ALWAYS_SOFTWARE=1 ./OM3D