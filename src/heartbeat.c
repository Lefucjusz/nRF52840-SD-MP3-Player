#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>

#define HEARTBEAT_PWM_PERIOD_NS (1 * 1000U * 1000U)
#define HEARTBEAT_STACK_SIZE (1024U * 1)
#define HEARTBEAT_PRIORITY 10

static const struct pwm_dt_spec pwm_led = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));

static uint32_t percent_to_pulse_value(float percent)
{
	return (percent * HEARTBEAT_PWM_PERIOD_NS) / 100.0f;
}

static void heartbeat_thread(void)
{
    if (!pwm_is_ready_dt(&pwm_led)) {
		return;
	}

	while (1) {
		for (size_t i = 0; i < 20; ++i) {
			pwm_set_dt(&pwm_led, HEARTBEAT_PWM_PERIOD_NS, percent_to_pulse_value(i));
			k_msleep(50);
		}

		for (size_t i = 20; i > 0; --i) {
			pwm_set_dt(&pwm_led, HEARTBEAT_PWM_PERIOD_NS, percent_to_pulse_value(i));
			k_msleep(50);
		}
	}
}

K_THREAD_DEFINE(heartbeat, HEARTBEAT_STACK_SIZE, heartbeat_thread, NULL, NULL, NULL, HEARTBEAT_PRIORITY, 0, 0);
