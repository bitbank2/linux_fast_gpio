//
// h618_fast_gpio
// Written by Larry Bank (bitbank@pobox.com)
// Project started 8/25/2026
//
// SPDX-FileCopyrightText: 2026 BitBank Software, Inc.
// SPDX-License-Identifier: Apache-2.0
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// Adapt these functions to whatever target platform you're using
// and the rest of the code can remain unchanged
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
  uint32_t cfg[4]; // 8 sets of 4 mode bits controlled per 32-bit register
  uint32_t data;
} PORT_REG;

volatile uint8_t *pGPIOBase;
static int mem_fd;

// AllWinner H618 GPIO PORT registers base address.
#define GPIO_BASE         0x0300B000
// Each set of 0x24 bytes maps to a port. 0=A, 1=B, 2=C, etc
#define GPIO_BLOCK_SIZE (9 * 0x24)
#define INPUT 0
#define OUTPUT 1

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
// On the OrangePi Zero 2W, ports C through I are available, so we'll
// map those to pin numbers 00-FF with 32 pins per letter (port)
// 00-1F = PORTC
// 20-3F = PORTD
// 40-5F = PORTE
// 60-7F = PORTF
// 80-9F = PORTG
// A0-BF = PORTH
// C0-DF = PORTI
// Sets the pin mode to input or output
//
void pinMode(uint8_t pin, int mode) {
PORT_REG *port;

    port = (PORT_REG *)&pGPIOBase[(2+(pin>>5)) * 0x24];
    pin &= 31; // limit to bit number within the chosen port
    port->cfg[pin>>3] &= ~(7 << ((pin & 7) * 4));
    port->cfg[pin>>3] |= (mode << ((pin & 7) * 4));
} /* pinMode() */
//
// Set a GPIO output pin to the given state
//
void digitalWrite(uint8_t pin, uint8_t data) {
PORT_REG *port;

    port = (PORT_REG *)&pGPIOBase[(2+(pin>>5))*0x24];
    pin &= 31;
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

    port = (PORT_REG *)&pGPIOBase[(2+(pin>>5))*0x24];
    pin &= 31;
    data = (port->data >> pin) & 1;
    return data;
} /* digitalRead() */

int main(int argc, char *argv[])
{
const uint8_t pin = 0xc1; // pin PI1

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
