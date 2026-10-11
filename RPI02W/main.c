//
// RPI Zero 2W fast_gpio
// Written by Larry Bank (bitbank@pobox.com)
// Project started 8/25/2026
//
// SPDX-FileCopyrightText: 2026 BitBank Software, Inc.
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#define _DEFAULT_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <bb_spi_lcd.h>
SPILCD lcd;
//
// Pimoroni Display HAT mini pin assignments on the OrangePi Zero 2W
//
#define RESET_PIN -1
#define CS_PIN 7
#define CLK_PIN 11
#define BL_PIN 13
#define DC_PIN 9
#define MOSI_PIN 10

// Port access struct
typedef struct tag_port_register
{
  uint32_t cfg[3]; // 3 sets of 10 x 3 mode bits
  uint32_t gap0[4];
  uint32_t data_set;
  uint32_t gap1[2];
  uint32_t data_clr;
  uint32_t gap2[2];
  uint32_t data;
} PORT_REG;

volatile uint8_t *pGPIOBase;
static int mem_fd;

#define GPIO_BASE         0x3f200000
#define GPIO_BLOCK_SIZE   0x10000
#ifdef INPUT
#undef INPUT
#endif
#ifdef OUTPUT
#undef OUTPUT
#endif
#define INPUT 0
#define OUTPUT 1

// Return the current time in milliseconds
int MilliTime(void)
{
int iTime;
struct timespec res;

    clock_gettime(CLOCK_MONOTONIC, &res);
    iTime = 1000*res.tv_sec + res.tv_nsec/1000000;

    return iTime;
} /* MilliTime() */

//
// Prepare direct GPIO port access by mapping
// the hardware address into user space
//
void initGPIO(void)
{
void *gpio_map;

    if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
	perror("Failed to open /dev/mem, try running as root");
        exit(EXIT_FAILURE);
    }
    gpio_map = mmap(NULL, GPIO_BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, GPIO_BASE);
    close(mem_fd);
    if (gpio_map == MAP_FAILED) {
        perror("mmap error");
	exit(EXIT_FAILURE);
    }
    pGPIOBase = (volatile uint8_t *)gpio_map;
    //printf("GPIOBase address: 0x%016lx\n", (intptr_t)pGPIOBase);
} /* initGPIO() */
//
//
// Sets the pin mode to input or output
//
void pinMode(int pin, int mode) {
volatile PORT_REG *port;
int reg, shift;
uint32_t u32;

   port  = (PORT_REG *)pGPIOBase;
   reg   = pin/10;
   shift = (pin % 10) * 3;
   u32 = port->cfg[reg] & ~(7 << shift);
   u32 |= (mode << shift);
   port->cfg[reg] = u32;
} /* pinMode() */
//
// Set a collection of sequential GPIOs to a specific value
// The data bits and mask must already by shifted into their correct positions
// e.g. write 8-bits starting at bit 16: 0x00ff0000 0x00550000
//
#ifdef FUTURE
void parallel_write(uint32_t u32Mask, uint32_t u32Value)
{
PORT_REG *port;
uint32_t u32Temp;

    port = (PORT_REG *)&pGPIOBase[(2+(pin>>5))*0x24];
    u32Temp = (port->data & ~u32Mask);
    u32Temp |= u32Value;
    port->data = u32Temp;
} /* parallel_write() */
#endif // FUTURE
//
// Set a GPIO output pin to the given state
//
void digitalWrite(int pin, int data) {
volatile PORT_REG *port;

    port = (PORT_REG *)pGPIOBase;
    if(data) {
        port->data_set = (1 << pin);
    } else {
        port->data_clr = (1 << pin);
    }
} /* digitalWrite() */
//
// Read the current value from a GPIO pin
//
int digitalRead(int pin)
{
PORT_REG *port;
uint8_t data;

    port = (PORT_REG *)pGPIOBase;
    data = (port->data >> pin) & 1;
    return data;
} /* digitalRead() */

void LCDReset(void)
{
    if (RESET_PIN != -1) {
        digitalWrite(RESET_PIN, 0);
        usleep(100000);
        digitalWrite(RESET_PIN, 1);
        usleep(100000);
    }
    digitalWrite(BL_PIN, 1);
} /* LCDReset() */

static inline void delay_cycles(int cycles)
{
    while (cycles) {
        __asm__ __volatile__ ("nop");
        cycles--;
    }
} /* delay_cycles() */

void FastLCDWrite(uint8_t *pData, int iLen, int iMode)
{
int i, iBit;
uint8_t uc;
volatile uint32_t *pPort;
uint32_t u32CLKLow, u32Data;
PORT_REG *port;
volatile uint32_t *port_set, *port_clr;
const uint32_t u32CLK = (1 << CLK_PIN);
const uint32_t u32MOSI = (1 << MOSI_PIN);

    port = (PORT_REG *)pGPIOBase;
    port_set = &port->data_set;
    port_clr = &port->data_clr;
    digitalWrite(DC_PIN, (iMode == MODE_DATA)); // 0=CMD/1=DATA
    digitalWrite(CS_PIN, 0); // start SPI transaction
    delay_cycles(10); // allow setup time for CS+DC
    for (i=0; i<iLen; i++) {
        uc = *pData++;
        if (uc == 0 || uc == 0xff) { // simpler case when all data bits are the same
            if (uc) { // set the data once
                *port_set = u32MOSI;
            } else {
                *port_clr = u32MOSI;
            }
            for (iBit=0; iBit<8; iBit++) { // toggle the clock 8 times
                *port_clr = u32CLK; // clock low
                delay_cycles(1);
                *port_set = u32CLK;
            }
        } else {
            for (iBit=0; iBit<8; iBit++) {
                *port_clr = u32CLK; // clock low
                if (uc & 0x80) {
                    *port_set = u32MOSI; 
                } else {
                    *port_clr = u32MOSI;
                }
                delay_cycles(1);
                *port_set = u32CLK;
                uc <<= 1;
            }
        }
    }
    digitalWrite(CS_PIN, 1); // end SPI transaction
} /* FastLCDWrite() */

void LCDWriteData(uint8_t *pData, int iLen, int iMode)
{
int i, iBit;
uint8_t uc;

//printf("write data: len=%d, mode=%d\n", iLen, iMode);

    digitalWrite(DC_PIN, (iMode == MODE_DATA)); // 0=CMD/1=DATA
    digitalWrite(CS_PIN, 0); // start SPI transaction
    delay_cycles(50); // allow setup time for CS+DC
    for (i=0; i<iLen; i++) {
        uc = *pData++;
        for (iBit=0; iBit<8; iBit++) {
            digitalWrite(CLK_PIN, 0);
            digitalWrite(MOSI_PIN, uc & 0x80);
            delay_cycles(4);
            digitalWrite(CLK_PIN, 1);
            //delay_cycles(4);
            uc <<= 1;
        }
    }
    digitalWrite(CS_PIN, 1); // end SPI transaction
} /* LCDWriteData() */

int main(int argc, char *argv[])
{
int iTime, rc;

    initGPIO();

    //pinMode(RESET_PIN, OUTPUT);
    pinMode(CS_PIN, OUTPUT);
    pinMode(MOSI_PIN, OUTPUT);
    pinMode(CLK_PIN, OUTPUT);
    pinMode(DC_PIN, OUTPUT);
    pinMode(BL_PIN, OUTPUT);
    digitalWrite(BL_PIN, 1);
    spilcdSetCallbacks(&lcd, LCDReset, FastLCDWrite); //LCDWriteData);
    // Since we're managing the reset and data, we don't have to pass any GPIO pin info to the spilcdInit function
    rc = spilcdInit(&lcd, LCD_ST7789, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (rc != 0) {
        printf("Error initializing LCD\n");
    } else {
        spilcdSetOrientation(&lcd, LCD_ORIENTATION_270);
        iTime = MilliTime();
        for (rc = 0; rc < 10; rc++) {
            spilcdFill(&lcd, TFT_BLACK, DRAW_TO_LCD);
        }
        iTime = MilliTime() - iTime;
        printf("fillscreen time (10 iterations) = %d ms\n", iTime);
        spilcdWriteString(&lcd, 0, 0, "Hello World!", TFT_GREEN, TFT_BLACK, FONT_12x16, DRAW_TO_LCD);
    }
    return 0;
} /* main() */
