#include "pid.h"

#include <cmath>
#include <iostream>

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

    // 1. Pure proportional control.
    PID proportional(2.0f, 0.0f, 0.0f, 0.1f, 100.0f, 10.0f);
    checked++; failed += !expectClose("P positive error", proportional.update(10, 8), 4);
    checked++; failed += !expectClose("P negative error", proportional.update(3, 5), -4);
    checked++; failed += !expectClose("P zero error", proportional.update(5, 5), 0);

    // 2. Combined PID and reset.
    PID normal(2.0f, 1.0f, 0.1f, 0.1f, 100.0f, 10.0f);
    checked++; failed += !expectClose("PID first call", normal.update(10, 8), 4.2f);
    checked++; failed += !expectClose("PID second call", normal.update(10, 9), 1.3f);
    normal.reset();
    checked++; failed += !expectClose("PID after reset", normal.update(10, 8), 4.2f);

    // 3. Integral accumulation and integral limit.
    PID integral(0.0f, 1.0f, 0.0f, 0.1f, 100.0f, 1.0f);
    checked++; failed += !expectClose("I first increment", integral.update(5, 0), 0.5f);
    checked++; failed += !expectClose("I reaches limit", integral.update(5, 0), 1.0f);
    checked++; failed += !expectClose("I stays limited", integral.update(5, 0), 1.0f);
    checked++; failed += !expectClose("I keeps history at zero error", integral.update(0, 0), 1.0f);
    checked++; failed += !expectClose("I decreases on reverse error", integral.update(0, 2), 0.8f);

    // 4. Measurement derivative.
    PID derivative(0.0f, 0.0f, 0.1f, 0.1f, 100.0f, 10.0f);
    checked++; failed += !expectClose("D first call is zero", derivative.update(10, 8), 0);
    checked++; failed += !expectClose("D rising feedback", derivative.update(10, 9), -1);
    checked++; failed += !expectClose("D target-only step", derivative.update(20, 9), 0);
    checked++; failed += !expectClose("D falling feedback", derivative.update(20, 8), 1);

    // 5. Final output limits.
    PID outputBound(10.0f, 0.0f, 0.0f, 0.1f, 5.0f, 10.0f);
    checked++; failed += !expectClose("Output upper limit", outputBound.update(1, 0), 5);
    checked++; failed += !expectClose("Output lower limit", outputBound.update(-1, 0), -5);
    checked++; failed += !expectClose("Output returns to zero", outputBound.update(0, 0), 0);

    // 6. Same one-second integral with two fixed periods.
    PID fast(0.0f, 2.0f, 0.0f, 0.1f, 100.0f, 10.0f);
    PID slow(0.0f, 2.0f, 0.0f, 0.2f, 100.0f, 10.0f);
    float fastResult = 0.0f;
    float slowResult = 0.0f;
    for (int i = 0; i < 10; ++i) fastResult = fast.update(1, 0);
    for (int i = 0; i < 5; ++i) slowResult = slow.update(1, 0);
    checked++; failed += !expectClose("I with dt=0.1 for 1 second", fastResult, 2);
    checked++; failed += !expectClose("I with dt=0.2 for 1 second", slowResult, 2);

    // 7. Independent state for two objects.
    PID first(0.0f, 1.0f, 0.0f, 1.0f, 100.0f, 20.0f);
    PID second(0.0f, 1.0f, 0.0f, 1.0f, 100.0f, 20.0f);
    checked++; failed += !expectClose("Object A first call", first.update(1, 0), 1);
    checked++; failed += !expectClose("Object B first call", second.update(4, 0), 4);
    checked++; failed += !expectClose("Object A keeps own history", first.update(1, 0), 2);
    checked++; failed += !expectClose("Object B keeps own history", second.update(0, 0), 4);

    std::cout << "\nBasic result: " << checked - failed
              << "/" << checked << " checks passed.\n";

    // Optional task: integral separation.
    int optionalFailed = 0;
    int optionalChecked = 0;
    PID separated(0.0f, 1.0f, 0.0f, 1.0f, 100.0f, 10.0f);
    separated.setIntegralSeparation(3.0f);

    optionalChecked++; optionalFailed += !expectClose("IS large error is frozen", separated.update(8, 0), 0);
    optionalChecked++; optionalFailed += !expectClose("IS threshold boundary integrates", separated.update(3, 0), 3);
    optionalChecked++; optionalFailed += !expectClose("IS small error integrates", separated.update(2, 0), 5);
    optionalChecked++; optionalFailed += !expectClose("IS large error keeps old I", separated.update(8, 0), 5);
    optionalChecked++; optionalFailed += !expectClose("IS reverse error integrates", separated.update(-2, 0), 3);

    separated.disableIntegralSeparation();
    optionalChecked++; optionalFailed += !expectClose("IS disabled resumes integration", separated.update(8, 0), 10);

    separated.setIntegralSeparation(3.0f);
    separated.reset();
    optionalChecked++; optionalFailed += !expectClose("IS reset keeps separation enabled", separated.update(8, 0), 0);
    optionalChecked++; optionalFailed += !expectClose("IS reset still accepts threshold", separated.update(3, 0), 3);

    std::cout << "Optional result: " << optionalChecked - optionalFailed
              << "/" << optionalChecked << " checks passed.\n";
    std::cout << "Total result: " << (checked + optionalChecked) - (failed + optionalFailed)
              << "/" << (checked + optionalChecked) << " checks passed.\n";

    return (failed == 0 && optionalFailed == 0) ? 0 : 1;
}
