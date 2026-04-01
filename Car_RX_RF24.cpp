#include <stdio.h>
#include "pico/stdlib.h"
#include <RF24.h>
#include "hardware/pwm.h"

// Pins
#define CE_PIN  7
#define CSN_PIN 8
#define gear_gpio 0
#define steer_gpio 1
#define motor_gpio 2

// Create radio object
RF24 radio(CE_PIN, CSN_PIN);

uint8_t address[6] = "CAR01";

// Data structure
typedef struct {
    uint8_t steer;
    uint8_t throttle;
    uint8_t gear;
} ControlPacket;

int main() {
    stdio_init_all();
    sleep_ms(2000); // Time to open serial screen

    uint steer_level, throttle_level;

    // Levels on gear servomotor, corresponding to duty cycle between 5% and 10% at 50Hz
    const uint gear_level[7] = {1250, 1458, 1666,1874,2082,2290, 2500};

    // Initialize PWM
    gpio_set_function(gear_gpio, GPIO_FUNC_PWM);
    gpio_set_function(steer_gpio, GPIO_FUNC_PWM);
    gpio_set_function(motor_gpio, GPIO_FUNC_PWM);

    const uint gear_slice = pwm_gpio_to_slice_num(gear_gpio);
    const uint steer_slice = pwm_gpio_to_slice_num(steer_gpio);
    const uint motor_slice = pwm_gpio_to_slice_num(motor_gpio);
    
    // Clock division for both servomotors
    pwm_config gear_config = pwm_get_default_config();
    pwm_config_set_clkdiv(&gear_config, 100.0f);
    pwm_init(gear_slice, &gear_config, true);

    pwm_config steer_config = pwm_get_default_config();
    pwm_config_set_clkdiv(&steer_config, 100.0f);
    pwm_init(steer_slice, &steer_config, true);

    pwm_set_wrap(gear_slice, 25000);  // Clk frequency/clk division/wrap = 50Hz
    pwm_set_wrap(steer_slice, 25000); // Clk frequency/clk division/wrap = 50Hz
    pwm_set_wrap(motor_slice, 6250);  // clk frequency/wrap = 20000Hz to reduce motor frequency noise
    
    const uint gear_channel = pwm_gpio_to_channel(gear_gpio);
    const uint steer_channel = pwm_gpio_to_channel(steer_gpio);
    const uint motor_channel = pwm_gpio_to_channel(motor_gpio);

    pwm_set_chan_level(gear_slice, gear_channel, gear_level[1]);    // Neutral gear
    pwm_set_chan_level(steer_slice, steer_channel, 1875);           // Centered
    pwm_set_chan_level(motor_slice, motor_channel, 0);              // Motor off

    pwm_set_enabled(gear_slice, true);
    pwm_set_enabled(steer_slice, true);
    pwm_set_enabled(motor_slice, true);

    if (!radio.begin()) {
        printf("ERROR: radio not working\n");
        while (1);
    }

    radio.setPALevel(RF24_PA_LOW);
    radio.setPayloadSize(sizeof(ControlPacket));

    radio.openReadingPipe(1, address);
    radio.startListening();             // RX mode

    ControlPacket packet;

    while (1) {

        if (radio.available()) {
            radio.read(&packet, sizeof(packet));

            printf("Received -> S=%d T=%d G=%d\n",
                   packet.steer,
                   packet.throttle,
                   packet.gear);

            // Unities conversion
            steer_level = (packet.steer*1.0f/100) * 1250 + 1250;     // 5 to 10% steer duty cycle
            throttle_level = (packet.throttle*1.0f/100) * (1<<16);   // 0 to 100% throttle duty cycle
            printf("Steer level: %d Throttle level: %d\n", steer_level, throttle_level);

            // Set PWM values
            pwm_set_chan_level(steer_slice, steer_channel, steer_level);
            pwm_set_chan_level(motor_slice, motor_channel, throttle_level);
            pwm_set_chan_level(gear_slice, gear_channel, gear_level[packet.gear]);
        }
    }
}