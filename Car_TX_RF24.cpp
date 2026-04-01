#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include <RF24.h>

// Pins
#define CE_PIN  7
#define CSN_PIN 8
#define MAX_RETRIES 5

// Create radio object
RF24 radio(CE_PIN, CSN_PIN);

// Direction (5 bytes + end)
uint8_t address[6] = "CAR01";

// Data structure
typedef struct {
    uint8_t steer;     // 0–100
    uint8_t throttle;  // 0–100
    uint8_t gear;      // 0–6
} ControlPacket;


// Prepare data to send

void prepare_packet(ControlPacket *pkt, uint raw_steer, uint raw_throttle, uint raw_gear) {

    pkt->steer = raw_steer;
    pkt->throttle = raw_throttle;
    pkt->gear = raw_gear;
}


// Send with retries

bool send_with_retry(ControlPacket *pkt) {
    for (int i = 0; i < MAX_RETRIES; i++) {

        if (radio.write(pkt, sizeof(ControlPacket))) {
            return true; // ACK received
        }

        sleep_ms(5);
    }

    return false;
}


int main() {

    stdio_init_all();
    sleep_ms(2000); // Time to open serial screen

    if (!radio.begin()) {
        printf("ERROR: radio not responding\n");
        while (1);
    }
    // Radio configuration
    radio.setPALevel(RF24_PA_LOW);
    radio.setPayloadSize(sizeof(ControlPacket));

    radio.stopListening();              // TX mode
    radio.openWritingPipe(address);     // Receiver address

    ControlPacket packet;

    uint steer, throttle;
    uint gear = 1;  // Initial gear is neutral
    absolute_time_t interval, now, prev_time = 0;
    unsigned char buffer[7], c;

    // Configure UART
    gpio_set_function(0, UART_FUNCSEL_NUM(uart0, 0));
    gpio_set_function(1, UART_FUNCSEL_NUM(uart0, 1));
    uart_init(uart0, 115200);

    while (1) {

        while (uart_is_readable(uart0)) {

            c = uart_getc(uart0);

            if(c == 0xAA){ // First character of input data

                for(int i=0;i<7;i++) {
                    while(!uart_is_readable(uart0));
                    buffer[i] = uart_getc(uart0);
                }

                unsigned char botones = buffer[0];                  // Buttons
                unsigned char brake_data = buffer[1];               // Brake
                unsigned char throttle_data = buffer[2];            // Throttle
                int16_t steer_data = buffer[3] | (buffer[4]<<8);    // Steer

                // Scale data to values between 0 and 100
                if (steer_data <= 0){
                    steer = 50 - steer_data*(-1.0f) * 50/(1<<15);
                }
                else {
                    steer = 50 + steer_data * 50.0f/(1<<15);
                }
                // Scale throttle to values from 0 to 100
                throttle = throttle_data*1.0f/(1<<8) * 100;

                printf("Botones:%02X Steer:%d Steer data: %d, Throttle: %d Throttle data: %d\n", botones, steer, steer_data, throttle, throttle_data);

                
                if(botones & 0x20){                       // If pressed the gear up button
                    now = get_absolute_time();
                    interval = absolute_time_diff_us(prev_time, now);
                    
                    if (gear <= 5 && interval >= 200000){ // Greater than 0.2 sec to prevent non wanted shifts
                        gear += 1;
                        prev_time = now;
                    }
                }n
                else if ((botones & 0x10)) {              // If pressed the gear down button
                    now = get_absolute_time();
                    interval = absolute_time_diff_us(prev_time, now);

                    if (gear >= 1 && interval >= 200000){ // Greater than 0.2 sec to prevent non wanted shifts
                        gear -= 1;
                        prev_time = now;
                    }
                }
                else {
                    ;
                }

                prepare_packet(&packet, steer, throttle, gear);

                bool success = send_with_retry(&packet);

                if (success) {
                    printf("OK -> S=%d T=%d G=%d\n",
                        packet.steer,
                        packet.throttle,
                        packet.gear);
                } else {
                    printf("FAIL (sin ACK)\n");
                }

                sleep_ms(5);
            }
        }
    }
}