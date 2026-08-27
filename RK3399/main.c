//
// linux_fast_gpio
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

volatile uint8_t *pGPIOBase;
static int mem_fd;
#define NUM_CHIPS 5
// Rockchip RK3399 GPIO PORT 0 to 4 registers base addresses
const uint32_t u32Bases[NUM_CHIPS] = {0xFF720000,0xFF730000,0xFF780000,0xFF788000,0xFF790000};
#define GPIO_BLOCK_SIZE 0x10000

typedef struct tag_gpio_reg
{
uint32_t SWPORTA_DR;        //0x0000 Port A data register
uint32_t SWPORTA_DDR;       //0x0004 Port A data direction register
uint32_t RESERVED0[10];     //0x0008
uint32_t INTEN;	            //0x0030 Interrupt enable register
uint32_t INTMASK;           //0x0034 Interrupt mask register
uint32_t INTTYPE_LEVEL;     //0x0038 Interrupt level register
uint32_t INT_POLARITY;      //0x003C Interrupt polarity register
uint32_t INT_STATUS;        //0x0040 Interrupt status of port A
uint32_t INT_RAWSTATUS;     //0x0044 Raw Interrupt status of port A
uint32_t DEBOUNCE;          //0x0048 Debounce enable register
uint32_t PORTA_EOI;         //0x004c Port A clear interrupt register
uint32_t EXT_PORTA;         //0x0050 Debounce enable register
uint32_t RESERVED1[3];      //0x0054
uint32_t LS_SYNC;           //0x0060 Level_sensitive synchronization enable register
} PORT_REG;
void *pPorts[8];
#define INPUT 0
#define OUTPUT 1

//
// Prepare direct GPIO port access by mapping
// the hardware address into user space
//
void initGPIO(void)
{
void *gpio_map;

    for (int iChip=0; iChip<NUM_CHIPS; iChip++) {
        if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
	    perror("Failed to open /dev/mem, try running as root");
            exit(EXIT_FAILURE);
        }
        gpio_map = mmap(NULL, GPIO_BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, u32Bases[iChip]);
        close(mem_fd);
        if (gpio_map == MAP_FAILED) {
            perror("mmap error");
	    exit(EXIT_FAILURE);
        }
	pPorts[iChip] = gpio_map;
    } // for iChip
} /* initGPIO() */
//
// On the Rock Pi 4B, ports 3 and 4 have all of the GPIO header pins
// 00-1F = PORT0
// 20-3F = PORT1
// 40-5F = PORT2
// 60-7F = PORT3
// 80-9F = PORT4
// Sets the pin mode to input or output
//
void pinMode(uint8_t pin, int mode) {
volatile PORT_REG *port;

    port = (PORT_REG *)pPorts[pin>>5];
    pin &= 31; // limit to bit number within the chosen port
    port->SWPORTA_DDR &= ~(1 << pin);
    port->SWPORTA_DDR |= (mode << pin);
} /* pinMode() */
//
// Set a GPIO output pin to the given state
//
void digitalWrite(uint8_t pin, uint8_t data) {
volatile PORT_REG *port;

    port = (PORT_REG *)pPorts[pin>>5];
    pin &= 31;
    if(data) {
        port->SWPORTA_DR |= (1 << pin);
    } else {
        port->SWPORTA_DR &= ~(1 << pin);
    }
} /* digitalWrite() */
//
// Read the current value from a GPIO pin
//
uint8_t digitalRead(uint8_t pin)
{
volatile PORT_REG *port;
uint8_t data;

    port = (PORT_REG *)pPorts[pin>>5];
    pin &= 31;
    data = (port->SWPORTA_DR >> pin) & 1;
    return data;
} /* digitalRead() */

int main(int argc, char *argv[])
{
const uint8_t pin = 131; // pin 12 on the 40-pin header of the Rock Pi 4B+

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
