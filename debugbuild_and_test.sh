DIR=build.debug
rm -rf ${DIR}
mkdir ${DIR}
cd ${DIR}
CC=/usr/bin/clang cmake .. -DCMAKE_BUILD_TYPE=Debug -DUSCH_LLVM_ROOT_PATH=/usr/lib/llvm-3.8/ || exit 22
make -j$(cat /proc/cpuinfo |grep processor|wc -l) || exit 32
test/usch_tests ||exit 42

