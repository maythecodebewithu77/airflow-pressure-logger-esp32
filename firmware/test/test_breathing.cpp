#include <unity.h>
#include "breathing.h"
#include <math.h>

void setUp(void) {}
void tearDown(void) {}

void test_breathing_normal_bpm(void) {
    BreathingAnalyzer analyzer;
    float fs = 50.0f;
    float dt = 1.0f / fs;
    float period_s = 4.0f; // 15 BPM

    for (int step = 0; step < 3000; step++) { // 60 seconds
        float t = step * dt;
        uint32_t now_ms = (uint32_t)(t * 1000.0f);
        float phase = fmodf(t, period_s) / period_s;

        AirflowSample sample;
        sample.timestamp_ms = now_ms;
        sample.sensor_fault = false;

        if (phase < 0.40f) {
            sample.relative_flow_lmin = -22.0f * sinf(phase / 0.40f * 3.14159f);
        } else {
            sample.relative_flow_lmin = 18.0f * sinf((phase - 0.40f) / 0.60f * 3.14159f);
        }

        BreathingMetrics m = analyzer.update(sample);
    }

    BreathingMetrics final_m = analyzer.getMetrics();
    TEST_ASSERT_FLOAT_WITHIN(0.3f, 15.0f, final_m.bpm);
}

void test_pause_alert_trigger(void) {
    BreathingAnalyzer analyzer;
    float fs = 50.0f;
    float dt = 1.0f / fs;
    bool pause_raised = false;

    for (int step = 0; step < 1500; step++) { // 30 seconds
        float t = step * dt;
        uint32_t now_ms = (uint32_t)(t * 1000.0f);

        AirflowSample sample;
        sample.timestamp_ms = now_ms;
        sample.sensor_fault = false;

        if (t >= 5.0f && t < 18.0f) {
            sample.relative_flow_lmin = 0.0f; // 13s pause
        } else {
            sample.relative_flow_lmin = -20.0f * sinf(t * 1.57f);
        }

        BreathingMetrics m = analyzer.update(sample);
        if (m.pause_event_raised) {
            pause_raised = true;
        }
    }

    TEST_ASSERT_TRUE(pause_raised);
}

void test_sensor_fault_discrimination(void) {
    BreathingAnalyzer analyzer;
    AirflowSample sample;
    sample.timestamp_ms = 1000;
    sample.relative_flow_lmin = 0.0f;
    sample.sensor_fault = true;

    BreathingMetrics m = analyzer.update(sample);
    TEST_ASSERT_TRUE(m.status_flags & STATUS_BIT_SENSOR_FAULT);
    TEST_ASSERT_FALSE(m.status_flags & STATUS_BIT_PAUSE_ALERT);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_breathing_normal_bpm);
    RUN_TEST(test_pause_alert_trigger);
    RUN_TEST(test_sensor_fault_discrimination);
    return UNITY_END();
}
