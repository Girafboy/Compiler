# 生成汇编
./build/tiger-compiler build/testcases/tlink.tig
# 编译汇编
gcc -Wl,--wrap,getchar -m64 build/testcases/tlink.tig.s src/tiger/runtime/runtime.c -o test.out
# 执行结果
./test.out