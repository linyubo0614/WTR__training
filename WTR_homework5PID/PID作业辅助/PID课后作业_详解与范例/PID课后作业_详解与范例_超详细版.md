# PID 课后作业详解与范例（C 语言学习者超详细版）

> **适用对象**：只系统学过 C 语言、刚接触 PID、还没有学过 C++ 类的同学。  
> **作业来源**：`C:\Users\32871\Downloads\PID课后作业.zip`。  
> **本文目标**：把作业要求、PID 公式、必需的 C++ 知识、完整参考代码、运行结果和提交说明一次讲清楚。  
> **参考代码位置**：`桌面\PID课后作业_详解与范例\基础参考代码` 与 `桌面\PID课后作业_详解与范例\选做_积分分离参考代码`。  
> **实测结果**：基础参考代码 **24/24 checks passed**；选做参考代码 **9/9 bonus checks passed**。

---

## 0. 先给结论：这份作业到底要你做什么

一句话：**用 C++ 完成一个 PID 类的 `update()` 和 `reset()`，让给定的 24 个自动测试全部通过，并能手算解释结果；学有余力再做“积分分离”选做题。**

你可以把任务拆成下面 6 件事：

1. **先解决文件后缀问题**：压缩包里的头文件实际叫 `pid.hpp`，但 `pid.cpp` 和 `main.cpp` 里写的是 `#include "pid.h"`。直接编译会报 `fatal error: pid.h: No such file or directory`。最简单、最符合作业说明的做法是把 `pid.hpp` 改名为 `pid.h`；也可以把两个 `#include "pid.h"` 改成 `#include "pid.hpp"`，但提交要求写的是 `pid.h`，所以推荐改名。
2. **读懂 `pid.h`**：知道 `kp_、ki_、kd_、dt_、outputLimit_、integralLimit_` 是配置参数；`integral_、lastFeedback_、firstUpdate_` 是历史状态。
3. **完成 `pid.cpp` 里的 `update()`**：按作业规定的 8 个步骤算 P、I、D，做积分限幅和输出限幅，保存历史状态，返回最终输出。
4. **完成 `pid.cpp` 里的 `reset()`**：只清积分、历史反馈和首次标记，不改配置参数。
5. **编译运行 `main.cpp`**：看到 `24/24 checks passed`，并能解释为什么每个测试的预期值是这样算出来的。
6. **提交**：`pid.h`、`pid.cpp`、`main.cpp`、测试输出或截图，以及一份 300～500 字说明。选做题另附新增测试和结果。

基础部分满分 100 分，积分分离选做最多加 20 分。老师明确说了：**不要只交一张通过截图，要能逐行解释代码。**

---

## 1. 解压后的文件和“文件后缀坑”

压缩包解压后目录结构是：

```text
PID课后作业/
└─ PID类课后作业/
   ├─ main.cpp       // 自动测试程序，提供 24 个检查点
   ├─ pid.cpp        // 构造函数、clip()、update() 和 reset() 的框架
   ├─ pid.hpp        // 类的声明、成员变量说明；注意后缀是 .hpp
   └─ 作业说明.md     // 规则、公式、验收和提交要求
```

`pid.cpp` 第一行是：

```cpp
#include "pid.h"
```

`main.cpp` 第一行也是：

```cpp
#include "pid.h"
```

但压缩包里没有 `pid.h`，只有 `pid.hpp`。所以在 Windows 资源管理器里先把 `pid.hpp` 重命名为 `pid.h`，然后再编译。或者在命令行里执行：

```bash
mv pid.hpp pid.h
```

> 注意：这不改变代码接口，只是修正文件名。很多同学卡在第一步，其实是文件扩展名的问题，不是 C++ 语法问题。

---

## 2. 作业要求、提交内容和评分

### 2.1 基础任务（必做）

- 完成 `PID::update(float target, float feedback)`。
- 完成 `PID::reset()`。
- 不修改 `pid.h` 的公开接口（基础版），不修改 `main.cpp` 的预期值。
- 用 C++11 或更高标准编译，运行后应显示 `24/24 checks passed`。
- 能够手算并解释教材给出的三行数据：
  - `Kp=2, Ki=1, Kd=0.1, dt=0.1, outputLimit=100, integralLimit=10`；
  - 第一次 `update(10, 8)` 返回 `4.2`；
  - 第二次 `update(10, 9)` 返回 `1.3`；
  - `reset()` 后 `update(10, 8)` 返回 `4.2`。

### 2.2 选做任务（加分）

增加“积分分离”功能：

```cpp
void setIntegralSeparation(float errorThreshold); // 开启，阈值 >= 0
void disableIntegralSeparation();                 // 关闭
```

规则：

- 默认关闭，保证基础 24 项测试仍然全过。
- 开启后，只有 `|error| <= errorThreshold` 才更新积分；否则保留旧 `integral_`。
- 暂停积分时，旧 `integral_` 仍然参加总输出，不清零、不屏蔽。
- 积分限幅和输出限幅始终保留。
- `reset()` 只清历史状态，保留积分分离的启用状态和阈值。
- 自己补测试：大误差、阈值边界、重新进入积分区、关闭分离后恢复、复位。

### 2.3 提交清单

1. 完成后的 `pid.h`、`pid.cpp`、`main.cpp`。
2. 测试输出记录或截图。
3. 约 300～500 字说明，回答四个问题：
   - 为什么 `integral_` 和 `lastFeedback_` 要作为对象的成员变量保存？
   - 为什么误差为 0 时，积分输出可能不为 0？
   - 为什么输出限幅不能代替积分限幅？
   - 为什么创建两个 PID 对象时，它们的状态必须独立？这与串级 PID 有什么关系？
4. 选做题：新增测试和运行结果。

### 2.4 评分重点

| 项目 | 分值 | 重点 |
| --- | ---: | --- |
| P/I/D 与周期计算 | 30 | 误差方向、显式 `dt`、测量微分、首次 D=0 |
| 两种限幅 | 20 | 积分状态写回限幅、最终输出单独限幅 |
| 历史状态与 reset | 20 | 跨调用保存、更新顺序、多对象独立 |
| 测试与手算解释 | 20 | 24 项通过，能解释关键数据和失败原因 |
| 命名、注释、提交完整性 | 10 | 代码可读、说明完整 |
| 积分分离选做 | +20 | 规则与配置管理 10 分，自编测试与解释 10 分 |

---

## 3. 只学过 C 语言，完成作业必需的 C++ 知识

这一节是本文的重点补充。你不需要系统学完整本《C++ Primer》，只需要看懂作业用到的这一小部分。下面每一条都用 C 语言的概念来类比。

### 3.1 文件后缀：`.c/.h` 与 `.cpp/.hpp/.h`

- C 源文件常用 `.c`，头文件常用 `.h`。
- C++ 源文件常用 `.cpp`、`.cc`、`.cxx`，头文件常用 `.h`、`.hpp`。
- `.hpp` 只是“C++ 头文件”的一种常见后缀，不是新语法。
- 编译器看到 `.cpp` 会按 C++ 编译；看到 `.c` 通常按 C 编译。作业要求用 `g++`，它会自动按 C++ 处理 `.cpp`。
- **文件名必须和 `#include` 里写的名字一致。** 这就是本作业 `pid.hpp` 与 `pid.h` 不匹配会报错的原因。

### 3.2 `class` 与 C 的 `struct`

C 的结构体只能装数据：

```c
typedef struct {
    float kp;
    float ki;
    float integral;
} PID_C;
```

C++ 的类可以同时装数据（成员变量）和函数（成员函数）：

```cpp
class PID {
public:
    float update(float target, float feedback);
private:
    float kp_;
    float integral_;
};
```

从内存角度看，C++ 对象里仍然有一块数据；成员函数不会在每个对象里各存一份，它们只是“知道该操作哪个对象的数据”。

`class` 和 `struct` 在 C++ 里几乎一样，唯一明显区别是：

- `class` 默认访问权限是 `private`；
- `struct` 默认访问权限是 `public`。

本作业使用 `class`，所以需要显式写 `public:` 和 `private:`。

### 3.3 对象、实例化，以及为什么不用 `new`

在 `main.cpp` 里看到：

```cpp
PID normal(2.0f, 1.0f, 0.1f, 0.1f, 100.0f, 10.0f);
```

这行叫**实例化一个对象**，也可以说“创建了一个 PID 对象”。`normal` 是对象名。

C 语言里你可能会写：

```c
PID_C pid;
pid_init(&pid, 2.0f, 1.0f, 0.1f);
```

C++ 里把“初始化”写进了构造函数，所以创建对象时直接把参数放在括号里：

```cpp
PID pid(2.0f, 1.0f, 0.1f, 0.1f, 100.0f, 10.0f);
```

这里不需要 `malloc`，也不需要 `new`。`normal` 是栈上的自动变量，离开作用域会自动销毁。本作业不需要动态内存、不需要指针、不需要 `delete`。

**重要：控制器对象应该在循环外创建一次。** 如果每轮循环都写一次 `PID pid(...)`，对象的历史状态每次都重新开始，积分和上次反馈都会丢失。

### 3.4 `public` 和 `private`：封装

```cpp
class PID {
public:
    float update(float target, float feedback);
    void reset();
private:
    float kp_;
    float integral_;
    float lastFeedback_;
};
```

- `public:` 下面的内容外部可以访问，比如 `main()` 里可以调用 `pid.update(...)`、`pid.reset()`。
- `private:` 下面的内容只有类的成员函数内部可以访问。外部不能写 `pid.integral_ = 0`，这是一种保护。
- 这样设计的目的：外部只通过 `update()` 和 `reset()` 操作对象，防止别人随便改内部状态，保证状态变化可控。

### 3.5 成员变量、`this` 和末尾的下划线

在成员函数内部，可以直接写 `integral_`，不需要像 C 那样每次都传结构体指针：

```cpp
integral_ += ki_ * error * dt_;
```

编译器会把它理解为“当前对象的 `integral_`”。成员函数有一个隐藏指针 `this`，上面这行本质上类似：

```cpp
this->integral_ += this->ki_ * error * this->dt_;
```

末尾的下划线 `_` 不是 C++ 的语法，也不会让变量变得特殊，只是命名习惯，用来区分：

```cpp
float kp_;   // 成员变量
float kp;    // 构造函数参数
```

### 3.6 成员函数的定义与 `类名::函数名`

在类里面写：

```cpp
float update(float target, float feedback);
```

这是**声明**：告诉编译器“PID 类有一个这样的成员函数”。

在 `pid.cpp` 里写：

```cpp
float PID::update(float target, float feedback) {
    // 函数体
}
```

`PID::` 叫作用域运算符，意思是“这个 `update` 是 `PID` 类的成员函数”，不是普通全局函数。

调用时：

- 通过对象调用：`pid.update(10, 8);`
- 在类外访问静态成员函数：`PID::clip(...)`。`clip` 是 `static` 成员函数，所以函数体里直接写 `clip(...)` 就可以。

`.` 和 `::` 的区别：

- `对象.成员`：例如 `pid.update(...)`；
- `类名::成员`：例如 `PID::update`、`PID::clip`。

### 3.7 构造函数和初始化列表

```cpp
PID::PID(float kp, float ki, float kd,
         float dt, float outputLimit, float integralLimit)
    : kp_(kp), ki_(ki), kd_(kd),
      dt_(dt), outputLimit_(outputLimit), integralLimit_(integralLimit) {
}
```

逐个解释：

- 函数名和类名相同、没有返回类型，这样的函数叫**构造函数**。
- 创建对象时自动调用，用来设置初始参数。
- 冒号 `:` 后面的叫**初始化列表**。
- `kp_(kp)` 的意思是：用参数 `kp` 初始化成员变量 `kp_`。左边有下划线的是成员，右边没有下划线的是传进来的参数。
- 这里把 6 个配置参数保存进对象，之后 `update()` 里就能使用 `kp_`、`ki_` 等。
- 作业说明说“配置创建后不改变”，所以这 6 个成员变量在 `update()` 和 `reset()` 里都不应该被修改。

### 3.8 默认成员初始值（C++11）

在 `pid.h` 里看到：

```cpp
float integral_ = 0.0f;
float lastFeedback_ = 0.0f;
bool firstUpdate_ = true;
```

这是 C++11 的**非静态数据成员默认初始化**：每次创建一个 `PID` 对象时，如果没有在构造函数初始化列表里另外指定，这三个成员就自动取这些值。

你可以把它类比成 C 里的：

```c
PID_C pid = {0};  // integral=0, lastFeedback=0
pid.firstUpdate = true;
```

但 C++ 写法把默认值直接放在类里面，更清楚。

### 3.9 `static` 成员函数

```cpp
static float clip(float value, float minValue, float maxValue);
```

`static` 在这里表示这个成员函数不依赖某个具体对象，不需要访问 `kp_`、`integral_` 等成员变量。它只是挂在 `PID` 类名下、和 PID 有关的工具函数，所以可以直接写：

```cpp
return PID::clip(value, -limit, limit);
```

在 `update()` 内部也可以直接写 `clip(...)`。

### 3.10 `bool`、`true/false` 和 `float` 常量后缀 `f`

- `bool` 是 C++ 的布尔类型，取值 `true` 或 `false`。C 语言在 C99 后需要 `<stdbool.h>` 才有 `bool`，C++ 直接支持。
- `firstUpdate_` 用来记录“是不是第一次调用”。第一次没有上一次的反馈，所以 D 必须取 0。
- `0.1f`、`2.0f` 末尾的 `f` 表示 `float` 常量；如果不写 `f`，`0.1` 默认是 `double`。本作业全程用 `float`，所以加 `f` 更一致，也避免一些类型转换警告。

### 3.11 头文件保护宏

`pid.h` 开头结尾有：

```cpp
#ifndef PID_H
#define PID_H

// 类的声明

#endif
```

这叫 include guard（头文件保护）。如果同一个 `.cpp` 间接包含了 `pid.h` 多次，宏 `PID_H` 已经定义，第二次就不会重复展开，避免类被重复定义。C 语言中也常用这种写法。

### 3.12 多文件编译：声明和定义分开

本作业有三个源码文件：

- `pid.h`：类的声明、成员变量声明、函数声明；
- `pid.cpp`：成员函数的具体实现；
- `main.cpp`：测试代码，包含 `pid.h` 并使用 `PID` 类。

编译命令：

```bash
g++ -std=c++11 -Wall -Wextra -Wpedantic -finput-charset=UTF-8 pid.cpp main.cpp -o pid_test.exe
```

`g++` 实际做了两件事：

1. 分别编译 `pid.cpp` 和 `main.cpp`；
2. 把它们链接成一个可执行文件 `pid_test.exe`。

链接时，`main.cpp` 里调用的 `PID::PID`、`PID::update`、`PID::reset` 会在 `pid.cpp` 生成的目标代码里找到。所以：

- `pid.h` 里只放声明，不要放普通函数的定义;
- `pid.cpp` 里写成员函数实现；
- 不要让两个 `.cpp` 都定义同一个非内联函数，否则会“重复定义”链接错误。

### 3.13 `main.cpp` 中你只需要“看懂”的 C++ 语法

`main.cpp` 已经给好了，不需要你改。它用到的 C++ 知识主要有：

```cpp
#include <iostream>
#include <cmath>

std::cout << "hello" << '\n';
std::fabs(x);
std::isfinite(x);
const float tolerance = 0.00001f;
```

补充说明：

- `std::` 是 C++ 标准库命名空间。`std::cout` 是标准输出流，类似 C 的 `printf`，但用 `<<` 把内容“送”出去。
- `<<` 在这里被重载为输出运算符；你不用自己实现，只要会读 `std::cout << a << b << '\n';`。
- `'\n'` 是换行字符；也可以用 `std::endl`，但 `'\n'` 更轻量。
- `std::fabs` 是浮点绝对值，相当于 C 的 `fabs`。
- `std::isfinite` 判断浮点数是否有限（不是 inf、nan）。这是 C++11 标准库函数。
- `const float tolerance = 0.00001f;` 表示常量，和 C 的 `const float` 类似。
- 测试中不能用 `==` 直接比较两个浮点数，因为浮点计算有舍入误差，所以 `main.cpp` 用 `fabs(actual - expected) <= tolerance`。

### 3.14 C 与 C++ 对照表

| C 里的做法 | 本作业 C++ 的做法 | 说明 |
| --- | --- | --- |
| `typedef struct { ... } PID;` | `class PID { ... };` | 类把数据和操作放在一起 |
| `void pid_update(PID* p, float t, float f);` | `float PID::update(float t, float f);` | 成员函数自带隐藏的 `this` |
| `pid->integral += ...;` | `integral_ += ...;` | 成员函数内直接访问成员变量 |
| `PID pid; pid_init(&pid, ...);` | `PID pid(...);` | 构造函数在创建时自动执行 |
| 手动写 `pid_reset(&pid);` | `pid.reset();` | 通过对象调用成员函数 |
| 用 `#define` 或全局函数做工具 | `static float PID::clip(...)` | 类内工具函数 |
| `0.1` 默认 double | `0.1f` 明确 float | 浮点字面量后缀 |
| `int flag; /* 0/1 */` | `bool firstUpdate_;` | C++ 布尔类型更清晰 |

### 3.15 本作业不需要的 C++ 知识

以下内容**都不需要**，看到也不用慌：

- 继承、多态、虚函数；
- 模板、STL 容器、迭代器；
- `new`/`delete`、智能指针；
- Lambda 表达式；
- 异常处理；
- 移动语义、右值引用。

作业说明也明确说了：重点是控制计算和状态管理，不是复杂 C++ 语法。

---

## 4. PID 控制原理：用最少的话讲清楚

### 4.1 五个基本量

- `target`：目标值，你希望被控量达到的值。
- `feedback`：反馈值，传感器实际测到的被控量。
- `error`：误差，作业规定 `error = target - feedback`。
- `u`：控制器输出，送给执行器（电机、电调、加热器等）的指令。
- `dt`：两次 `update()` 之间的固定时间，单位是秒。

单位提醒：

- `target` 和 `feedback` 必须使用同一个单位，比如都是 `rad/s` 或都是 `m`。
- `u` 可以和它们不同，比如目标单位是 `rad/s`，输出可能是 PWM 占空比或电流指令。
- `dt = 0.01` 表示 10 ms，不能写成 `10`。

### 4.2 P：比例项

```text
P = Kp * error
```

直观理解：误差越大，控制力度越大；误差为 0，比例项也变为 0。

- 只有 P 时，系统往往会稳定在一个略小于目标值的点：因为要对抗摩擦力、重力等负载，必须保留一点误差才能产生输出。这叫稳态误差。
- `Kp` 越大响应越快，但过大容易振荡。

### 4.3 I：积分项

```text
integral_ += Ki * error * dt
```

积分项把历史误差累加起来：

- 误差长期为正，积分不断增大，输出也慢慢增大；
- 误差长期为负，积分不断减小；
- 当误差为 0 时，已有积分**不会自动清零**，它继续提供输出，用来维持目标。
- 积分可以消除稳态误差，但积分过强会导致超调、振荡，甚至积分饱和。

本作业特别规定：**`integral_` 保存的是已经乘过 `Ki` 的积分输出 I，不是原始的误差和。** 总输出里直接加 `integral_`，不要再乘一次 `ki_`。

### 4.4 D：微分项

连续形式：

```text
D = Kd * de/dt
```

但在数字控制中，不能真的求导，只能用两次采样的差分近似。本作业采用“测量值微分”并对反馈变化率取负号：

```text
D = -Kd * (feedback - lastFeedback_) / dt
```

为什么前面有负号？因为 `error = target - feedback`，当目标不变、反馈增大时，误差在减小。为了阻尼这种变化，D 应该产生负输出，阻止反馈冲过头。所以写成反馈的负变化率。

这种写法的另一个好处：当只有目标突然变化、反馈还没变时，D 不会因为目标阶跃而突然跳变，避免“微分冲击”。

### 4.5 本作业使用的完整离散公式

按顺序：

```text
error = target - feedback
P = Kp * error
integral_ = integral_ + Ki * error * dt
integral_ = clamp(integral_, -integralLimit, +integralLimit)
D = 0                                      （第一次调用）
D = -Kd * (feedback - lastFeedback) / dt  （其余调用）
u = P + integral_ + D
u = clamp(u, -outputLimit, +outputLimit)
lastFeedback = feedback
firstUpdate = false
return u
```

### 4.6 为什么积分状态要“先累加再限幅并写回”

正确顺序：

```cpp
integral_ += ki_ * error * dt_;
integral_ = clip(integral_, -integralLimit_, integralLimit_);
```

不是只把 `P + integral_ + D` 的总和限幅。区别在于：

- 只限制总输出：`integral_` 内部可能已经累加到很大，比如 1000；虽然返回的 `u` 被限制到 100，但内部状态没有被限制。等到误差反向时，这个巨大的积分需要很长时间才能退回来，造成超调或失控。
- 对 `integral_` 本身限幅并写回：内部状态最多是 `integralLimit_`，更安全、更可预测。

这就是“积分限幅”和“输出限幅”必须分别做、不能互相替代的原因。

---
## 5. 基础任务一：完成 `update()`

### 5.1 八个步骤

作业说明中已经给出顺序，照做即可：

1. 计算误差 `error = target - feedback`。
2. 计算比例项 `P = kp_ * error`。
3. 更新积分状态 `integral_ += ki_ * error * dt_`。
4. 把 `integral_` 截断到 `[-integralLimit_, +integralLimit_]`，并写回 `integral_`。
5. 计算微分项。首次调用 `D = 0`；其他时候 `D = -kd_ * (feedback - lastFeedback_) / dt_`。
6. 合成 `u = P + integral_ + D`，然后对 `u` 做 `outputLimit_` 限幅。
7. 在本次微分计算完成后，保存本次 `feedback`，并更新 `firstUpdate_` 标记。
8. 返回最终的 `u`。

### 5.2 每一步的代码和解释

#### 第 1 步：计算误差

```cpp
const float error = target - feedback;
```

- 方向必须是目标减反馈。
- 如果写反，整个系统会向相反方向控制。
- `const` 表示这个局部变量不会再被修改。

#### 第 2 步：计算比例项

```cpp
const float p = kp_ * error;
```

- 直接使用成员变量 `kp_`。
- 比例项不需要 `dt`。

#### 第 3 步：更新积分

```cpp
integral_ += ki_ * error * dt_;
```

- 显式乘 `dt_`，不能漏。
- 这里累加的是“已经乘过 `ki_` 的积分输出”。
- 不要在总输出合成时再乘一次 `ki_`。

#### 第 4 步：积分限幅并写回

```cpp
integral_ = clip(integral_, -integralLimit_, integralLimit_);
```

- `clip` 已经提供：小于下界取下界，大于上界取上界。
- 一定要赋值回 `integral_`，只调用 `clip` 不接收返回值没有用。
- 这一步限制的是内部积分状态，不是最终输出。

#### 第 5 步：计算微分项

```cpp
float d = 0.0f;
if (!firstUpdate_) {
    d = -kd_ * (feedback - lastFeedback_) / dt_;
}
```

- 第一次调用时 `firstUpdate_ == true`，所以 `D = 0`。
- 之后用本次反馈减上次反馈的差，除以 `dt_`。
- 前面有负号。
- 不能把 `lastFeedback_` 提前更新，否则差永远是 0。

#### 第 6 步：合成并限幅

```cpp
float u = p + integral_ + d;
u = clip(u, -outputLimit_, outputLimit_);
```

- `integral_` 已经是 I，不要再乘 `ki_`。
- 输出限幅是最后一道保护，限制的是返回给执行器的指令。
- 下限是 `-outputLimit_`，上限是 `+outputLimit_`。

#### 第 7 步：保存历史

```cpp
lastFeedback_ = feedback;
firstUpdate_ = false;
```

- 必须在本次微分计算完成之后再保存。
- 更新后，下一次调用才有“上一次反馈”。
- 首次标记只在第一次调用后变为 `false`，`reset()` 时再恢复为 `true`。

#### 第 8 步：返回输出

```cpp
return u;
```

### 5.3 基础版完整 `pid.cpp`

下面是完整、可编译的参考实现。你可以对照自己的代码，但建议先自己写一遍再对照。

```cpp
#include "pid.h"

// 构造函数：用初始化列表把传入参数保存到当前对象的成员变量中。
PID::PID(float kp, float ki, float kd,
         float dt, float outputLimit, float integralLimit)
    : kp_(kp), ki_(ki), kd_(kd),
      dt_(dt), outputLimit_(outputLimit), integralLimit_(integralLimit) {
    // 历史状态 integral_、lastFeedback_、firstUpdate_ 已在 pid.h 中给定初值。
}

// 将 value 截断到 [minValue, maxValue]。
float PID::clip(float value, float minValue, float maxValue) {
    if (value > maxValue) {
        return maxValue;
    }
    if (value < minValue) {
        return minValue;
    }
    return value;
}

float PID::update(float target, float feedback) {
    // 1. 误差方向：目标值 - 反馈值。
    const float error = target - feedback;

    // 2. 比例项 P = Kp * e。
    const float p = kp_ * error;

    // 3. 积分项 I 累加：I += Ki * e * dt。
    //    注意：integral_ 保存的是已经乘过 Ki 的“积分输出”，合成时直接加。
    integral_ += ki_ * error * dt_;

    // 4. 对积分状态本身限幅，并把限幅后的值写回 integral_。
    integral_ = clip(integral_, -integralLimit_, integralLimit_);

    // 5. 微分项 D。首次调用没有上一次反馈，因此 D=0。
    //    采用“测量值微分”：D = -Kd * (feedback - lastFeedback) / dt。
    float d = 0.0f;
    if (!firstUpdate_) {
        d = -kd_ * (feedback - lastFeedback_) / dt_;
    }

    // 6. 合成总输出，并做输出限幅。
    float u = p + integral_ + d;
    u = clip(u, -outputLimit_, outputLimit_);

    // 7. 本次微分计算完成后再保存本次 feedback，并取消首次标记。
    lastFeedback_ = feedback;
    firstUpdate_ = false;

    // 8. 返回限幅后的最终输出。
    return u;
}

void PID::reset() {
    // 只清除历史状态，配置参数保持不变。
    integral_ = 0.0f;
    lastFeedback_ = 0.0f;
    firstUpdate_ = true;
}
```

### 5.4 顺序上的四个坑

1. **先加积分再限幅**：不能先限幅再累加，否则上限会被一次次突破。
2. **先算 D 再更新 `lastFeedback_`**：顺序反了 D 会恒为 0。
3. **先更新 `firstUpdate_` 再算 D**：第一次 D 也会变成非 0。
4. **`reset()` 不能放在 `update()` 开头**：否则每次调用都把历史清掉，积分永远累积不起来。

---

## 6. 基础任务二：完成 `reset()`

`reset()` 只做三件事：

```cpp
void PID::reset() {
    integral_ = 0.0f;
    lastFeedback_ = 0.0f;
    firstUpdate_ = true;
}
```

要求：

- 积分清零；
- 历史反馈清零；
- `firstUpdate_` 恢复为 `true`，下一次调用 D=0；
- **所有配置参数保持不变**，包括 `kp_、ki_、kd_、dt_、outputLimit_、integralLimit_`；
- 选做版还要保留“积分分离是否开启、阈值是多少”。

不要做这些错事：

- 不要重置 `kp_`、`ki_`、`kd_`；
- 不要重置限幅值；
- 不要删除或重新创建对象；`reset()` 本身不创建新对象。

`reset()` 的效果是：清除历史，但保留控制器配置。`main.cpp` 里测试了“`reset()` 后再 `update(10, 8)` 的结果应和新建对象第一次调用一样”，就是验证这一点。

---
## 7. 验收测试与手算解释

### 7.1 编译运行

把 `pid.hpp` 改名为 `pid.h` 后，在三个文件所在目录执行：

```bash
g++ -std=c++11 -Wall -Wextra -Wpedantic -finput-charset=UTF-8 pid.cpp main.cpp -o pid_test.exe
```

Windows PowerShell：

```powershell
.\pid_test.exe
```

Git Bash：

```bash
./pid_test.exe
```

预期最后一行：

```text
Result: 24/24 checks passed.
```

### 7.2 24 个检查点分组说明

| 测试组 | 检查内容 | 关键点 |
| --- | --- | --- |
| 纯 P | 正误差、负误差、零误差 | 误差方向不能反 |
| 综合计算与复位 | P/I/D 合成、首次 D=0、reset 后恢复 | 顺序和历史状态 |
| 纯 I | 累加、到达上限、零误差保留、反向减少 | 积分状态写回限幅 |
| 纯 D | 反馈上升/下降、目标变化无突跳 | 测量微分和负号 |
| 输出限幅 | 正上限、负下限、回到正常范围 | 最后再限幅 |
| 周期 | 同样误差持续 1 秒，两种 `dt` 积分结果相同 | 显式乘 `dt` |
| 多对象 | 两个 PID 交替调用，积分状态独立 | 成员变量属于各自对象 |

### 7.3 教材要求的手算表

参数：`Kp=2, Ki=1, Kd=0.1, dt=0.1, outputLimit=100, integralLimit=10`，初始状态全为零。

| 调用 | 误差 e | P | 更新后的 I | D | 返回的 u |
| --- | ---: | ---: | ---: | ---: | ---: |
| 第一次 `update(10, 8)` | 2 | 4 | 0.2 | 0 | 4.2 |
| 第二次 `update(10, 9)` | 1 | 2 | 0.3 | -1 | 1.3 |
| `reset` 后 `update(10, 8)` | 2 | 4 | 0.2 | 0 | 4.2 |

手算过程：

**第一次**：

```text
error = 10 - 8 = 2
P = 2 * 2 = 4
integral_ = 0 + 1 * 2 * 0.1 = 0.2
integral_ 限幅：0.2 在 [-10, 10] 内，不变
firstUpdate == true，所以 D = 0
u = 4 + 0.2 + 0 = 4.2
outputLimit=100，4.2 不越界
lastFeedback_ = 8，firstUpdate_ = false
```

**第二次**：

```text
error = 10 - 9 = 1
P = 2 * 1 = 2
integral_ = 0.2 + 1 * 1 * 0.1 = 0.3
D = -0.1 * (9 - 8) / 0.1 = -0.1 * 1 / 0.1 = -1
u = 2 + 0.3 - 1 = 1.3
lastFeedback_ = 9
```

**reset 后第一次**：

```text
integral_ = 0
lastFeedback_ = 0
firstUpdate_ = true
error = 10 - 8 = 2
P = 4
integral_ = 0 + 1 * 2 * 0.1 = 0.2
D = 0（首次）
u = 4.2
```

### 7.4 基础参考代码实测输出

```text
[PASS] P positive error | actual=4 expected=4
[PASS] P negative error | actual=-4 expected=-4
[PASS] P zero error | actual=0 expected=0
[PASS] PID first call | actual=4.2 expected=4.2
[PASS] PID second call | actual=1.3 expected=1.3
[PASS] PID after reset | actual=4.2 expected=4.2
[PASS] I first increment | actual=0.5 expected=0.5
[PASS] I reaches limit | actual=1 expected=1
[PASS] I stays limited | actual=1 expected=1
[PASS] I keeps history at zero error | actual=1 expected=1
[PASS] I decreases on reverse error | actual=0.8 expected=0.8
[PASS] D first call is zero | actual=0 expected=0
[PASS] D rising feedback | actual=-1 expected=-1
[PASS] D target-only step | actual=0 expected=0
[PASS] D falling feedback | actual=1 expected=1
[PASS] Output upper limit | actual=5 expected=5
[PASS] Output lower limit | actual=-5 expected=-5
[PASS] Output returns to zero | actual=0 expected=0
[PASS] I with dt=0.1 for 1 second | actual=2 expected=2
[PASS] I with dt=0.2 for 1 second | actual=2 expected=2
[PASS] Object A first call | actual=1 expected=1
[PASS] Object B first call | actual=4 expected=4
[PASS] Object A keeps own history | actual=2 expected=2
[PASS] Object B keeps own history | actual=4 expected=4

Result: 24/24 checks passed.
```

---
## 8. 选做任务：积分分离（最多加 20 分）

### 8.1 积分分离要解决什么问题

普通积分在误差很大时也一直累加，容易把积分状态推到上限，造成超调。积分分离的思路是：

- 误差很大时：不累加积分，先让 P 和 D 快速把系统拉回来；
- 误差进入较小范围后：再开启积分，用它消除稳态误差；
- 暂停累加时：已有积分仍参与输出，不清零、不屏蔽。

### 8.2 新增接口

在 `pid.h` 的 `public:` 区域增加：

```cpp
void setIntegralSeparation(float errorThreshold);
void disableIntegralSeparation();
```

在 `private:` 区域增加配置成员：

```cpp
bool integralSeparationEnabled_ = false;
float integralSeparationThreshold_ = 0.0f;
```

这里把“是否开启”和“阈值”作为配置，默认关闭。`reset()` 不清除它们。

### 8.3 `update()` 中积分部分的改法

把原来的：

```cpp
integral_ += ki_ * error * dt_;
```

改成：

```cpp
if (!integralSeparationEnabled_
    || (error >= -integralSeparationThreshold_
        && error <= integralSeparationThreshold_)) {
    integral_ += ki_ * error * dt_;
}
integral_ = clip(integral_, -integralLimit_, integralLimit_);
```

逻辑：

- 没开启积分分离：条件为真，照常累加；
- 开启了，且 `|error| <= threshold`：累加；
- 开启了，但 `|error| > threshold`：跳过累加，旧 `integral_` 保留；
- 无论是否累加，后面都要对 `integral_` 做限幅并写回。

阈值判断使用原始误差 `error`，不要用限幅后的误差，也不要用其他中间量。

### 8.4 新增成员函数的实现

```cpp
void PID::setIntegralSeparation(float errorThreshold) {
    integralSeparationThreshold_ = errorThreshold;
    integralSeparationEnabled_ = true;
}

void PID::disableIntegralSeparation() {
    integralSeparationEnabled_ = false;
    // 阈值可以保留，方便以后再次开启；它不影响关闭状态下的计算。
}
```

`reset()` 仍然只写：

```cpp
integral_ = 0.0f;
lastFeedback_ = 0.0f;
firstUpdate_ = true;
```

不要动 `integralSeparationEnabled_` 和 `integralSeparationThreshold_`。

### 8.5 教材自检例子

参数：`Kp=Kd=0, Ki=1, dt=1, integralLimit=10, outputLimit=100, threshold=3`。  
依次让误差为 `8, 3, 2, 8, -2`，积分输出应为：

| 第几次 | 误差 | 是否累加 | 积分 I |
| --- | ---: | --- | ---: |
| 1 | 8 | 否，`8 > 3` | 0 |
| 2 | 3 | 是，边界 `3 <= 3` | 3 |
| 3 | 2 | 是 | 5 |
| 4 | 8 | 否，保留旧值 | 5 |
| 5 | -2 | 是，绝对值 `2 <= 3` | 3 |

然后关闭积分分离，再输入误差 8：

```text
I = 3 + 1 * 8 * 1 = 11
integralLimit = 10，所以 I 被限幅为 10
```

### 8.6 选做版完整 `update()` 和新增函数

```cpp
float PID::update(float target, float feedback) {
    const float error = target - feedback;
    const float p = kp_ * error;

    // 选做题：只有关闭积分分离，或 |error| <= 阈值时，才更新积分。
    if (!integralSeparationEnabled_
        || (error >= -integralSeparationThreshold_
            && error <= integralSeparationThreshold_)) {
        integral_ += ki_ * error * dt_;
    }

    integral_ = clip(integral_, -integralLimit_, integralLimit_);

    float d = 0.0f;
    if (!firstUpdate_) {
        d = -kd_ * (feedback - lastFeedback_) / dt_;
    }

    float u = p + integral_ + d;
    u = clip(u, -outputLimit_, outputLimit_);

    lastFeedback_ = feedback;
    firstUpdate_ = false;
    return u;
}

void PID::reset() {
    integral_ = 0.0f;
    lastFeedback_ = 0.0f;
    firstUpdate_ = true;
}

void PID::setIntegralSeparation(float errorThreshold) {
    integralSeparationThreshold_ = errorThreshold;
    integralSeparationEnabled_ = true;
}

void PID::disableIntegralSeparation() {
    integralSeparationEnabled_ = false;
}
```

### 8.7 选做测试运行结果

参考测试程序覆盖了：默认关闭、大误差冻结、阈值边界、重新进入积分区、关闭后恢复累加、复位保留配置。实测结果：

```text
[PASS] Default separation off: first I | actual=8 expected=8
[PASS] Separation: large error freezes I | actual=0 expected=0
[PASS] Separation: boundary error accumulates | actual=3 expected=3
[PASS] Separation: inside range accumulates | actual=5 expected=5
[PASS] Separation: large error again freezes old I | actual=5 expected=5
[PASS] Separation: reverse error inside range decreases I | actual=3 expected=3
[PASS] Disable separation: accumulate and clamp | actual=10 expected=10
[PASS] Reset keeps separation enabled: large error freezes | actual=0 expected=0
[PASS] Reset keeps threshold: boundary accumulates | actual=3 expected=3

Bonus result: 9/9 checks passed.
```

### 8.8 选做部分的边界和注意点

- `errorThreshold` 应大于等于 0。本作业假设输入有效，不要求异常处理，但你自己写测试时不要传负数来为难自己。
- 阈值过小，系统可能一直进不了积分区，单纯增大 `Ki` 也没用。
- 冻结积分会减弱消稳态误差的能力，所以积分分离不是“万能抗饱和”，只是选做的简化策略。
- 暂停累加时旧积分必须继续参与 `P + integral_ + D`，否则输出会出现突变。
- 关闭分离后，下一次 `update()` 应立即恢复累加，不需要额外清状态。
- `reset()` 只清 `integral_`、`lastFeedback_`、`firstUpdate_`，不要动配置。

---
## 9. 常见错误与排查表

| 现象 | 可能原因 | 修改方法 |
| --- | --- | --- |
| `fatal error: pid.h: No such file or directory` | 压缩包里是 `pid.hpp` | 把 `pid.hpp` 改名为 `pid.h` |
| 纯 I 测试第一次只得到 `0.05` 而不是 `0.5` | 漏乘或错乘 `dt` | 照公式 `Ki*e*dt` |
| 积分到了 1 还继续增大 | 没有把 `clip` 结果写回 `integral_` | `integral_ = clip(...)` |
| 输出限幅测试失败 | 只对积分限幅，没有对最终 `u` 限幅 | 最后再 `u = clip(...)` |
| D 一直为 0 | 先更新了 `lastFeedback_` 再算 D，或忘了复位 `firstUpdate_` | 先算 D，再更新 `lastFeedback_`，再置 `false` |
| D 符号相反 | 写成 `feedback - lastFeedback` 时忘加负号 | 作业规定前有负号 |
| 目标变化时 D 很大 | 用了目标误差微分而不是测量值微分 | 用 `feedback - lastFeedback_` |
| `reset` 后配置也变了 | 在 `reset()` 里改了 `kp_` 等 | 只重置三个历史状态 |
| 每个循环积分都归零 | 在循环内反复创建 PID 对象，或 `update` 开头调用 `reset` | 对象循环外创建一次；不要在 update 里 reset |
| 两个对象互相影响 | 把状态写成了全局变量或静态全局变量 | 状态必须是普通成员变量 |
| 选做开启后基础测试变化 | 默认状态写成了开启，或 reset 把启用状态清掉 | 默认 `false`；reset 不清配置 |
| 编译警告很多但能跑 | 命名、未使用变量、浮点后缀等 | 按 `-Wall -Wextra` 提示改，保持代码整洁 |

---

## 10. 提交说明 300～500 字的参考写法

下面是一份可直接参考、但建议用自己的话改写的说明。它回答了作业要求的四个问题。

**1. 为什么 `integral_` 和 `lastFeedback_` 要作为对象的成员变量保存？**  
因为 PID 是跨采样周期工作的，每次 `update()` 只计算一个时间步。积分需要把本次误差累加到之前的历史上，微分需要拿本次反馈和上一次反馈作差。如果把它们写成局部变量，函数返回后值就消失了，下一次调用无法继续。把它们作为成员变量，可以让状态和这个 PID 对象绑定，在对象的整个生命周期内持续保存。用 C 语言类比，相当于把一个状态结构体的指针传进每个函数；C++ 用成员变量和成员函数把这件事封装在对象内部。

**2. 为什么误差为 0 时，积分输出可能不为 0？**  
积分项累加的是历史误差，而不是当前误差。误差曾经长期为正，积分就会积累出一定输出。当误差回到 0 时，这部分积分代表控制器为维持当前目标所需要的输出，比如克服摩擦力、重力或稳态负载。如果误差一为 0 就把积分清零，输出会立刻掉下来，系统又会产生误差，形成反复。所以正常误差为 0 时不清零积分，只有 `reset()` 才清除历史。

**3. 为什么输出限幅不能代替积分限幅？**  
输出限幅只限制 `u` 的返回值，不会阻止 `integral_` 在内部继续累加。若只做输出限幅，积分状态可能远超 `integralLimit_`；当误差反向时，巨大的积分需要很长时间才能退回来，导致超调或积分饱和。积分限幅是对内部状态写回限制，保证积分本身有界；输出限幅是对最终执行器指令限制。两者作用位置不同，必须分别做。

**4. 为什么两个 PID 对象的状态必须独立？这与串级 PID 有什么关系？**  
每个 PID 对象负责一个被控量和一条控制回路，它的积分和历史反馈只对该回路有意义。如果两个对象共享状态，目标值或反馈值交替输入时，积分会被互相污染，微分差分会跨对象计算，导致输出错误。串级 PID 中外环位置环和内环速度环通常有不同的目标、不同的反馈和不同的执行周期；外环输出作为内环目标。两个环必须各自保存独立状态，否则内环和外环会互相干扰，系统无法稳定工作。同理，多电机、多轴控制也要求每个轴有独立的 PID 对象。

（上面四段合计约 450 字，可作为素材；提交前建议按自己的理解重写一遍。）

---

## 11. 提交前检查清单

- [ ] `pid.hpp` 已改名为 `pid.h`，或 include 路径已修正。
- [ ] `update()` 误差方向是 `target - feedback`。
- [ ] 积分累加显式乘了 `dt_`。
- [ ] `integral_` 保存的是已经乘过 `ki_` 的积分输出，总输出里没有重复乘 `ki_`。
- [ ] 积分限幅后写回了 `integral_`。
- [ ] 首次调用 D=0。
- [ ] 非首次 D 使用 `-kd_ * (feedback - lastFeedback_) / dt_`。
- [ ] 先算 D，再更新 `lastFeedback_`。
- [ ] 最终输出做了 `outputLimit_` 限幅。
- [ ] `reset()` 只清 `integral_`、`lastFeedback_`、`firstUpdate_`。
- [ ] `reset()` 没有改配置参数。
- [ ] 循环外只创建一次 PID 对象。
- [ ] 编译运行输出 `24/24 checks passed`。
- [ ] 能独立手算第一次、第二次、reset 后三组数据。
- [ ] 选做默认关闭，基础 24 项仍全过。
- [ ] 选做开启后只在 `|error| <= threshold` 时累加积分。
- [ ] 选做暂停积分时旧积分仍参与输出。
- [ ] 选做 `reset()` 保留启用状态和阈值。
- [ ] 已保存测试输出或截图。
- [ ] 已写 300～500 字说明。

---

## 12. 参考文件与使用方法

桌面的 `PID课后作业_详解与范例` 文件夹中提供了：

```text
PID课后作业_详解与范例/
├─ 基础参考代码/
│  ├─ pid.h              // 与作业模板接口一致
│  ├─ pid.cpp            // 基础版完整实现
│  ├─ main.cpp           // 作业自带测试，未改预期值
│  ├─ pid_test.exe       // 可执行文件（可选）
│  └─ base_test_output.txt // 24/24 的实测记录
└─ 选做_积分分离参考代码/
   ├─ pid.h              // 增加积分分离接口和成员
   ├─ pid.cpp            // 基础 + 选做完整实现
   ├─ main_bonus.cpp     // 选做自编测试
   ├─ bonus_test.exe     // 可执行文件（可选）
   └─ bonus_test_output.txt // 9/9 的实测记录
```

建议学习顺序：

1. 先读本文第 3 节，掌握必需的 C++ 知识；
2. 自己打开作业原文件，按第 5、6 节的步骤写 `update()` 和 `reset()`；
3. 编译运行，确保 `24/24`；
4. 手算第 7.3 节的三行数据；
5. 再对照基础参考代码检查自己的写法；
6. 有余力再做第 8 节积分分离，并运行选做测试。

> **最后提醒**：参考代码的作用是帮助理解，不是让你直接交一份自己讲不出来的代码。老师评分明确包含“能解释关键数据与失败原因”。建议至少亲手敲一遍 `update()` 和 `reset()`，再合上文档复述一遍八个步骤。
