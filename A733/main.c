//
// A733_fast_gpio
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
#include <linux/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <bb_spi_lcd.h>
SPILCD lcd;

// Pimoroni Display HAT mini pin assignments on the OrangePi Zero 3W
// 
#define RESET_PIN -1
// PE4
#define CS_PIN 0x44
// PE1
#define CLK_PIN 0x41
// PD3
#define BL_PIN 0x33
// PE3
#define DC_PIN 0x43
// PE2
#define MOSI_PIN 0x42

// Port access struct
typedef struct tag_port_register
{
  uint32_t cfg[2]; // 8 sets of 4 mode bits controlled per 32-bit register
  uint32_t gap0[2]; // unused = gap
  uint32_t data;
  uint32_t data_set;
  uint32_t data_clr;
  uint32_t gap1;
  uint32_t drv0; // pin drive strength (2-bits per GPIO)
  uint32_t drv1;
  uint32_t gap2[2];
  uint32_t pul0; // 2-bits per pull up/dn (00=disable,01=up,10=dn,11=reserved)
  uint32_t gap3[3];
  uint32_t int_cfg0;
  uint32_t int_cfg1;
  uint32_t gap4[2];
  uint32_t int_ctrl;
  uint32_t int_status;
  uint32_t int_debounce;
  uint32_t gap5[5];
  uint32_t secure;
  uint32_t gap6[3];
} PORT_REG;

volatile uint8_t *pGPIOBase;
static int mem_fd;

// AllWinner A733 GPIO PORT registers base address.
// (it's really 0x2000000, but the there is no port A and special stuff is in 0-0x7f
//
#define GPIO_BASE         0x02000000
#define GPIO_BLOCK_SIZE (12 * 0x80)
// 4-bits per pin configuration for 16 possible states
#ifdef INPUT
#undef INPUT
#endif
#ifdef OUTPUT
#undef OUTPUT
#endif
#define INPUT 0
#define OUTPUT 1
// (2-14 = custom functions, e.g. I2C, SPI, PWM, etc)
#define DISABLED 15

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
// On the OrangePi Zero 3W, ports B through L are available
// with 16 GPIO pins per grouping
// 00-0F = PORTA (N/A)
// 10-1F = PORTB
// 20-2F = PORTC (N/A)
// 30-3F = PORTD
// 40-4F = PORTE
// 50-5F = PORTF (N/A)
// 60-6F = PORTG (N/A)
// 70-7F = PORTH (N/A)
// 80-8F = PORTI (N/A)
// 90-9F = PORTJ (N/A)
// A0-AF = PORTK (N/A)
// B0-BF = PORTL
// C0-FF = not used
//
// Sets the pin mode to input or output
//
void pinMode(int pin, int mode) {
PORT_REG *port;

    port = (PORT_REG *)&pGPIOBase[(1+(pin>>4)) * 0x80];
    pin &= 15; // limit to bit number within the chosen port
    port->cfg[pin>>3] &= ~(0xf << ((pin & 7) * 4));
    port->cfg[pin>>3] |= (mode << ((pin & 7) * 4));
} /* pinMode() */
//
// Set a collection of sequential GPIOs to a specific value
// The data bits and mask must already by shifted into their correct positions
// e.g. write 8-bits starting at bit 16: 0x00ff0000 0x00550000
//
void parallel_write(uint32_t u32Mask, uint32_t u32Value)
{
	(void)u32Mask; (void)u32Value;
//PORT_REG *port;
//uint32_t u32Temp;

//    port = (PORT_REG *)&pGPIOBase[(1+(pin>>4))*0x80];
//    u32Temp = (port->data & ~u32Mask);
//    u32Temp |= u32Value;
//    port->data = u32Temp;
} /* parallel_write() */
//
// Set a GPIO output pin to the given state
//
void digitalWrite(int pin, int data) {
PORT_REG *port;

    port = (PORT_REG *)&pGPIOBase[(1+(pin>>4))*0x80];
    pin &= 15;
    if(data) {
        port->data |= (1 << pin);
    } else {
        port->data &= ~(1 << pin);
    }
} /* digitalWrite() */
//
// Read the current value from a GPIO pin
//
int digitalRead(int pin)
{
PORT_REG *port;
uint8_t data;

    port = (PORT_REG *)&pGPIOBase[(1+(pin>>4))*0x80];
    pin &= 15;
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
        digitalWrite(BL_PIN, 1);
    }
} /* LCDReset() */

static inline void delay_cycles(int cycles)
{
    while (cycles) {
        __asm__ __volatile__ ("nop");
        cycles--;
    }
} /* delay_cycles() */

void LCDWriteData(uint8_t *pData, int iLen, int iMode)
{
int i, iBit;
uint8_t uc;

//printf("write data: len=%d, mode=%d\n", iLen, iMode);

    digitalWrite(DC_PIN, (iMode == MODE_DATA)); // 0=CMD/1=DATA
    digitalWrite(CS_PIN, 0); // start SPI transaction
    delay_cycles(500); // allow setup time for CS+DC
    for (i=0; i<iLen; i++) {
        uc = *pData++;
	for (iBit=0; iBit<8; iBit++) {
            digitalWrite(CLK_PIN, 0);
            digitalWrite(MOSI_PIN, uc & 0x80);
	    delay_cycles(1);
            digitalWrite(CLK_PIN, 1);
	    uc <<= 1;
	}
    }
    digitalWrite(CS_PIN, 1); // end SPI transaction
} /* LCDWriteData() */

void FastLCDWrite(uint8_t *pData, int iLen, int iMode)
{
int i, iBit;
uint8_t uc;
volatile uint32_t *pPort;
uint32_t u32CLKLow, u32Data;
PORT_REG *port;
const uint32_t u32CLK = (CLK_PIN & 0xf);
const uint32_t u32MOSI = (MOSI_PIN & 0xf);

// The SPI CLK and MOSI pins must be on the same GPIO port
    port = (PORT_REG *)&pGPIOBase[(1+(MOSI_PIN>>4))*0x80];
    pPort = &port->data; // point to the data register of this port
    port->data &= ~((1<<u32CLK) | (1<<u32MOSI));
    digitalWrite(DC_PIN, (iMode == MODE_DATA)); // 0=CMD/1=DATA
    digitalWrite(CS_PIN, 0); // start SPI transaction
    u32CLKLow = port->data;
    delay_cycles(10); // allow setup time for CS+DC
    for (i=0; i<iLen; i++) {
        uc = *pData++;
        for (iBit=0; iBit<8; iBit++) {
            u32Data = uc >> 7;
            *pPort = u32CLKLow | (u32Data << u32MOSI); // write data + clock low
            delay_cycles(4);
            *pPort = u32CLKLow | ((1 << u32CLK) | (u32Data << u32MOSI));
	    delay_cycles(4);
            uc <<= 1;
        }
    }
    digitalWrite(CS_PIN, 1); // end SPI transaction
} /* FastLCDWrite() */

int main(int argc, char *argv[])
{
int rc;
int iTime;

	initGPIO();
	pinMode(RESET_PIN, OUTPUT);
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
		spilcdFill(&lcd, TFT_BLACK, DRAW_TO_LCD);
		iTime = MilliTime() - iTime;
		printf("fillscreen time = %d ms\n", iTime);
		spilcdWriteString(&lcd, 0, 0, "Hello World!", TFT_GREEN, TFT_BLACK, FONT_12x16, DRAW_TO_LCD);
	}
	return 0;
} /* main() */
