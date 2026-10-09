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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/sysinfo.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

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
#define INPUT 0
#define OUTPUT 1
// (2-14 = custom functions, e.g. I2C, SPI, PWM, etc)
#define DISABLED 15

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
void pinMode(uint8_t pin, int mode) {
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
PORT_REG *port;
uint32_t u32Temp;

//    port = (PORT_REG *)&pGPIOBase[(1+(pin>>4))*0x80];
//    u32Temp = (port->data & ~u32Mask);
//    u32Temp |= u32Value;
//    port->data = u32Temp;
} /* parallel_write() */
//
// Set a GPIO output pin to the given state
//
void digitalWrite(uint8_t pin, uint8_t data) {
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
uint8_t digitalRead(uint8_t pin)
{
PORT_REG *port;
uint8_t data;

    port = (PORT_REG *)&pGPIOBase[(1+(pin>>4))*0x80];
    pin &= 15;
    data = (port->data >> pin) & 1;
    return data;
} /* digitalRead() */

int main(int argc, char *argv[])
{
const uint8_t pin = 0x10; // pin PB0

	initGPIO();
	pinMode(pin, OUTPUT);
	for(int i=0; i<10; i++) {
		digitalWrite(pin, 0);
		sleep(1);
		digitalWrite(pin, 1);
		sleep(1);
	}
	return 0;
} /* main() */
