#include "pid.hpp"
#include <cmath>
#include <iostream>

// 测试辅助函数已提供。无需修改，只需阅读各测试的输入和预期输出。
// 浮点数存在舍入误差，因此用容差比较，不能直接用 == 判断所有结果。
bool expectClose(const char* name, float actual, float expected) {
    const float tolerance = 0.00001f;
    const bool ok = std::isfinite(actual)
                 && std::fabs(actual - expected) <= tolerance;
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << name
              << " | actual=" << actual
              << " expected=" << expected << '\n';
    return ok;
}

int main() {
    int failed = 0;
    int checked = 0;

    // 1. 纯 P：检查误差方向与比例计算。
    PID proportional(2.0f, 0.0f, 0.0f, 0.1f, 100.0f, 10.0f);
    checked++; failed += !expectClose("P positive error", proportional.update(10, 8), 4);
    checked++; failed += !expectClose("P negative error", proportional.update(3, 5), -4);
    checked++; failed += !expectClose("P zero error", proportional.update(5, 5), 0);

    // 2. 综合计算与复位：首次 D=0，随后使用反馈变化率。
    PID normal(2.0f, 1.0f, 0.1f, 0.1f, 100.0f, 10.0f);
    checked++; failed += !expectClose("PID first call", normal.update(10, 8), 4.2f);
    checked++; failed += !expectClose("PID second call", normal.update(10, 9), 1.3f);
    normal.reset();
    checked++; failed += !expectClose("PID after reset", normal.update(10, 8), 4.2f);

    // 3. 纯 I：达到上限后不再增加，误差归零时保留积分，反向时可减少。
    PID integral(0.0f, 1.0f, 0.0f, 0.1f, 100.0f, 1.0f);
    checked++; failed += !expectClose("I first increment", integral.update(5, 0), 0.5f);
    checked++; failed += !expectClose("I reaches limit", integral.update(5, 0), 1.0f);
    checked++; failed += !expectClose("I stays limited", integral.update(5, 0), 1.0f);
    checked++; failed += !expectClose("I keeps history at zero error", integral.update(0, 0), 1.0f);
    checked++; failed += !expectClose("I decreases on reverse error", integral.update(0, 2), 0.8f);

    // 4. 纯 D：目标变化、反馈未变时，测量微分不产生额外突跳。
    PID derivative(0.0f, 0.0f, 0.1f, 0.1f, 100.0f, 10.0f);
    checked++; failed += !expectClose("D first call is zero", derivative.update(10, 8), 0);
    checked++; failed += !expectClose("D rising feedback", derivative.update(10, 9), -1);
    checked++; failed += !expectClose("D target-only step", derivative.update(20, 9), 0);
    checked++; failed += !expectClose("D falling feedback", derivative.update(20, 8), 1);

    // 5. 总输出限幅：检查上下界以及离开限幅后的正常输出。
    PID outputBound(10.0f, 0.0f, 0.0f, 0.1f, 5.0f, 10.0f);
    checked++; failed += !expectClose("Output upper limit", outputBound.update(1, 0), 5);
    checked++; failed += !expectClose("Output lower limit", outputBound.update(-1, 0), -5);
    checked++; failed += !expectClose("Output returns to zero", outputBound.update(0, 0), 0);

    // 6. 同样持续 1 秒的相同误差，积分结果应与使用哪个固定周期一致。
    // 这里只检验积分计算，不是两种采样周期下完整闭环性能的比较。
    PID fast(0.0f, 2.0f, 0.0f, 0.1f, 100.0f, 10.0f);
    PID slow(0.0f, 2.0f, 0.0f, 0.2f, 100.0f, 10.0f);
    float fastResult = 0;
    float slowResult = 0;
    for (int i = 0; i < 10; ++i) fastResult = fast.update(1, 0);
    for (int i = 0; i < 5; ++i) slowResult = slow.update(1, 0);
    checked++; failed += !expectClose("I with dt=0.1 for 1 second", fastResult, 2);
    checked++; failed += !expectClose("I with dt=0.2 for 1 second", slowResult, 2);

    // 7. 两个对象交替调用：积分状态必须相互独立。
    PID first(0.0f, 1.0f, 0.0f, 1.0f, 100.0f, 20.0f);
    PID second(0.0f, 1.0f, 0.0f, 1.0f, 100.0f, 20.0f);
    checked++; failed += !expectClose("Object A first call", first.update(1, 0), 1);
    checked++; failed += !expectClose("Object B first call", second.update(4, 0), 4);
    checked++; failed += !expectClose("Object A keeps own history", first.update(1, 0), 2);
    checked++; failed += !expectClose("Object B keeps own history", second.update(0, 0), 4);

    std::cout << "\nResult: " << checked - failed << "/" << checked << " checks passed.\n";
    
    // 8.选做作业测试代码
    //统计
    int optionalFailed = 0;
    int optionalChecked = 0;
    /*测试数据*/
    PID separated(0.0f, 1.0f, 0.0f, 1.0f, 100.0f, 10.0f);
    //开启积分分离，阈值为3
    separated.setIntegralSeparation(3.0f);

    /*---至少覆盖大误差、阈值边界、重新进入积分区、关闭分离后的恢复，以及复位---*/
    //大误差
    optionalChecked++; optionalFailed += !expectClose("IS large error is frozen", separated.update(8, 0), 0);
    //阈值边界
    optionalChecked++; optionalFailed += !expectClose("IS threshold boundary integrates", separated.update(3, 0), 3);
    //误差小的时候允许积分
    optionalChecked++; optionalFailed += !expectClose("IS small error integrates", separated.update(2, 0), 5);
    //再次大误差，原有积分保留，不清0
    optionalChecked++; optionalFailed += !expectClose("IS large error keeps old I", separated.update(8, 0), 5);
    //重新进入积分区
    optionalChecked++; optionalFailed += !expectClose("IS reverse error integrates", separated.update(-2, 0), 3);

    //关闭积分分离
    separated.disableIntegralSeparation();
    //关闭积分分离后进行大误差积分（包含积分限幅）
    optionalChecked++; optionalFailed += !expectClose("IS disabled resumes integration", separated.update(8, 0), 10);

    //再次开启积分分离
    separated.setIntegralSeparation(3.0f);
    //复位
    separated.reset();
    //验证复位后积分分离还是开启状态
    //大误差
    optionalChecked++; optionalFailed += !expectClose("IS reset keeps separation enabled", separated.update(8, 0), 0);
    //阈值边界
    optionalChecked++; optionalFailed += !expectClose("IS reset still accepts threshold", separated.update(3, 0), 3);

    //输出结果，这里把基础作业的测试数量和选作作业的测试项相加
    std::cout << "Optional result: " << optionalChecked - optionalFailed
              << "/" << optionalChecked << " checks passed.\n";
    std::cout << "Total result: " << (checked + optionalChecked) - (failed + optionalFailed)
              << "/" << (checked + optionalChecked) << " checks passed.\n";

    return (failed == 0 && optionalFailed == 0) ? 0 : 1;
}
