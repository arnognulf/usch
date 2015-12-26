DIR=build.release
rm -rf ${DIR}
mkdir ${DIR} 
cd ${DIR}
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSCH_LLVM_ROOT_PATH=/usr/lib/llvm-3.8/ || exit 22
make -j$(cat /proc/cpuinfo |grep processor|wc -l) || exit 32

