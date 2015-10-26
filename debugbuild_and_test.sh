mkdir debugbuild
cd debugbuild
cmake .. -DUSCH_LLVM_ROOT_PATH=/usr/lib/llvm-3.6/
make -j$(cat /proc/cpuinfo |grep processor|wc -l)
test/usch_tests ||exit 42

