#!/usr/bin/env bash
# 在 WSL / Linux 下编译并运行 firmware/common 单元测试
# 用法: bash build_and_test.sh
set -e
cd "$(dirname "$0")"
echo "== gcc version =="
gcc --version | head -1
echo "== compiling =="
gcc -Wall -Wextra -std=c99 -I.. test_main.c ../*.c -o run_tests
echo "== running =="
./run_tests
