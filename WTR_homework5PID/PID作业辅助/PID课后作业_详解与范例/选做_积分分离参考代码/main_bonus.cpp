#include "pid.h"
#include <cmath>
#include <iostream>

static int failed = 0;
static int checked = 0;

bool expectClose(const char* name, float actual, float expected) {
    const float tolerance = 0.00001f;
    const bool ok = std::isfinite(actual)
                 && std::fabs(actual - expected) <= tolerance;
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << name
              << " | actual=" << actual
              << " expected=" << expected << '\n';
    return ok;
}

void check(const char* name, float actual, float expected) {
    ++checked;
    failed += !expectClose(name, actual, expected);
}

int main() {
    // 默认关闭积分分离：行为应与基础版一致。
    PID base(0.0f, 1.0f, 0.0f, 1.0f, 100.0f, 10.0f);
    check("Default separation off: first I", base.update(8, 0), 8);

    // 教材自检例子：误差 8, 3, 2, 8, -2 -> I = 0, 3, 5, 5, 3。
    PID pid(0.0f, 1.0f, 0.0f, 1.0f, 100.0f, 10.0f);
    pid.setIntegralSeparation(3.0f);
    check("Separation: large error freezes I", pid.update(8, 0), 0);
    check("Separation: boundary error accumulates", pid.update(3, 0), 3);
    check("Separation: inside range accumulates", pid.update(2, 0), 5);
    check("Separation: large error again freezes old I", pid.update(8, 0), 5);
    check("Separation: reverse error inside range decreases I", pid.update(-2, 0), 3);

    // 关闭积分分离后恢复累加：I 从 3 开始，误差 8 -> 11，被积分限幅为 10。
    pid.disableIntegralSeparation();
    check("Disable separation: accumulate and clamp", pid.update(8, 0), 10);

    // reset 只清除历史状态，但保留“启用状态 + 阈值”。
    pid.setIntegralSeparation(3.0f);
    pid.reset();
    check("Reset keeps separation enabled: large error freezes", pid.update(8, 0), 0);
    check("Reset keeps threshold: boundary accumulates", pid.update(3, 0), 3);

    std::cout << "\nBonus result: " << checked - failed << "/" << checked
              << " checks passed.\n";
    return failed == 0 ? 0 : 1;
}
