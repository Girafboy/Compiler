# 启动环境
docker run -it --privileged -u root -v /tiger-compiler-2019-fall:/home/stu/tiger-compiler-2019-fall se302/tigerlabs_env:latest /bin/bash

# 做很多事情。。。
./gradelab5\&6.sh
# 生成汇编
./build/tiger-compiler build/testcases/bsearch.tig
# 编译汇编
gcc -Wl,--wrap,getchar -m64 build/testcases/bsearch.tig.s src/tiger/runtime/runtime.c -o test.out
# 执行结果
./test.out

# 手动make
cd build/
cmake ..
make tiger-compiler

# debug
gdb build/tiger-compiler
run build/testcases/bsearch.tig