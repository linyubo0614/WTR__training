PID 类课后作业可提交版

文件：
  pid.h     PID 类声明，包含积分分离选做接口
  pid.cpp   PID 基础算法和积分分离实现
  main.cpp  基础 24 项测试 + 8 项积分分离测试

编译（使用支持 C++11 的 g++）：
  g++ -std=c++11 -Wall -Wextra -Wpedantic pid.cpp main.cpp -o pid_test.exe

运行：
  .\pid_test.exe

预期最后三行：
  Basic result: 24/24 checks passed.
  Optional result: 8/8 checks passed.
  Total result: 32/32 checks passed.

说明：题目原文件名是 pid.hpp，但题目提交要求和源码包含名统一为 pid.h。本文件夹已经使用 pid.h，三份源码应放在同一目录中编译。
