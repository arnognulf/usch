mkdir debugbuild
cd debugbuild
cmake .. -DUSCH_LLVM_ROOT_PATH=/usr/lib/llvm-3.6/ || exit 22
make -j$(cat /proc/cpuinfo |grep processor|wc -l) || exit 32
test/usch_tests ||exit 42

