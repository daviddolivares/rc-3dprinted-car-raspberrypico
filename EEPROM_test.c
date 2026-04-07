/*
This code test the AT24C128 EEPROM memory, writing some bytes on a memory direction,
or reading these bytes from this direction.

Some of the issues presented here are been solved adding the while true loop.
Without this loop the reading test only worked sometimes unexpectly, where it returned
correctly the variable 'ret' that returns de i2c reading function, but did not shown
the raw values of the data on the memory.
*/


#include <hardware/gpio.h>
#include <hardware/structs/io_bank0.h>
#include <pico/error.h>
#include <pico/time.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/reent.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define SDA_PIN 0
#define SCL_PIN 1
#define DEVICE_ADDR 0x50    // Addr defined on datasheet


int byte_write(uint8_t addr, uint8_t *data_write){
    int ret;
    ret = i2c_write_blocking(i2c0, addr, data_write, 3, false);
    return ret;
}

int page_write(uint8_t addr, uint8_t data_array[], size_t len){
    int ret;
    ret = i2c_write_blocking(i2c0, addr, data_array, len, false);
    return ret;
}

int random_read(uint8_t addr, uint8_t *word_addr, uint8_t *data_read){
    int ret;
    i2c_write_blocking(i2c0, addr, word_addr, 2, true);
    ret = i2c_read_blocking(i2c0, addr, data_read, 2, false);        
    return ret;
}


int main()
{
    stdio_init_all();
    sleep_ms(4000);

    //          I2C Setup
    //________________________________
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);

    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    
    i2c_init(i2c0, 100000);

    //          Declare variables
    //________________________________
    int ret;
    uint8_t data = 230, word_addr_1, word_addr_2;
    uint16_t page_num = 0;
    uint8_t data_write[4], data_read[2];

    // word_addr_1 is the first 8-bit sequence of word address, so it has been shifted 
    // 8 bits to right minus the 6 bits required to left, presented on datasheet 
    // word_addr_2 has been shifted the 6 bits, presented on datasheet
    word_addr_1 = (page_num >> 2) & 0xFF;
    word_addr_2 = (page_num << 6) & 0xFF;

    data_write[0] = word_addr_1;
    data_write[1] = word_addr_2;
    data_write[2] = data;
    data_write[3] = data - 25;


    while (true) {
    
        // __________Writing Test________
        // It writes the 2 last data words on page 0 of the EEPROM
        printf("Data to write, Address: %d %d, Data 1: %d, Data 2: %d\n", data_write[0], data_write[1], data_write[2],data_write[3]);
        ret = byte_write(DEVICE_ADDR, data_write);
        ret = page_write(DEVICE_ADDR, data_write, 3);

        // __________Reading Test_________
        // It reads these 2 bytes writed on page 0
        ret = random_read(DEVICE_ADDR, data_write, data_read);
        printf("Data read: %u %u\n", data_read[0], data_read[1]);
        printf("Raw: 0x%02X 0x%02X\n", data_read[0], data_read[1]);
        
        printf("Ret: %d\n", ret);

        sleep_ms(1000);

    }
}
