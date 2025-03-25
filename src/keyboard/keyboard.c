#include "keyboard.h"
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(keyboard);

typedef struct
{
    struct k_work_delayable work;
    uint32_t pending_pins_mask;
} keyboard_delayed_work_t;

typedef struct
{
    struct gpio_dt_spec gpio;
    keyboard_button_t button;
} keyboard_gpio_map_t;

typedef struct 
{
    struct gpio_callback callback_data;
    keyboard_delayed_work_t button_work;
    keyboard_callback_t button_callbacks[KEYBOARD_BUTTONS_COUNT];
} keyboard_ctx_t;

static keyboard_ctx_t ctx;

static const keyboard_gpio_map_t gpio_map[KEYBOARD_BUTTONS_COUNT] = {
    {GPIO_DT_SPEC_GET(DT_NODELABEL(button_up), gpios), KEYBOARD_UP},
    {GPIO_DT_SPEC_GET(DT_NODELABEL(button_down), gpios), KEYBOARD_DOWN},
    {GPIO_DT_SPEC_GET(DT_NODELABEL(button_left), gpios), KEYBOARD_LEFT},
    {GPIO_DT_SPEC_GET(DT_NODELABEL(button_right), gpios), KEYBOARD_RIGHT},
    {GPIO_DT_SPEC_GET(DT_NODELABEL(button_enter), gpios), KEYBOARD_ENTER}
};

inline static bool is_pin_active(const struct gpio_dt_spec *gpio)
{
    return (gpio_pin_get_dt(gpio) == true);
}

inline static bool is_pin_pending(const struct gpio_dt_spec *gpio, uint32_t mask)
{
    return ((mask & BIT(gpio->pin)) != 0);
}

static void keyboard_gpio_handler(struct k_work *work)
{
    struct k_work_delayable *dwork = k_work_delayable_from_work(work);
    const keyboard_delayed_work_t *item = CONTAINER_OF(dwork, keyboard_delayed_work_t, work);

    for (size_t i = 0; i < ARRAY_SIZE(gpio_map); ++i) {
        const keyboard_gpio_map_t *entry = &gpio_map[i];
        if (is_pin_pending(&entry->gpio, item->pending_pins_mask) && is_pin_active(&entry->gpio)) {
            if (ctx.button_callbacks[entry->button] != NULL) {
                ctx.button_callbacks[entry->button]();
            }
        }
    }
}

static void keyboard_button_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    ARG_UNUSED(cb);

    ctx.button_work.pending_pins_mask = pins;
    k_work_reschedule(&ctx.button_work.work, K_MSEC(KEYBOARD_DEBOUNCE_TIME_MS));
}

int keyboard_init(void)
{
    int status;
    uint32_t gpio_pin_mask = 0;

    for (size_t i = 0; i < ARRAY_SIZE(gpio_map); ++i) {
        status = device_is_ready(gpio_map[i].gpio.port);
        if (status == 0) {
            LOG_ERR("Button GPIO not ready!");
            return -ENODEV;
        }

        status = gpio_pin_configure_dt(&gpio_map[i].gpio, GPIO_INPUT);
        if (status < 0) {
            LOG_ERR("Failed to configure GPIO pin as input, error %d", status);
            return status;
        }

        status = gpio_pin_interrupt_configure_dt(&gpio_map[i].gpio, GPIO_INT_EDGE_FALLING);
        if (status < 0) {
            LOG_ERR("Failed to configure GPIO pin interrupt, error: %d", status);
            return status;
        }

        gpio_pin_mask |= BIT(gpio_map[i].gpio.pin);
    }

    gpio_init_callback(&ctx.callback_data, keyboard_button_callback, gpio_pin_mask);
    status = gpio_add_callback(gpio_map[0].gpio.port, &ctx.callback_data);
    if (status < 0) {
        LOG_ERR("Failed to add GPIO callback, error %d", status);
        return status;
    }

    k_work_init_delayable(&ctx.button_work.work, keyboard_gpio_handler);

    return 0;
}

void keyboard_attach_callback(keyboard_button_t button, keyboard_callback_t callback)
{
    if ((button < 0) && (button >= KEYBOARD_BUTTONS_COUNT)) {
        return;
    }

    ctx.button_callbacks[button] = callback;
}
