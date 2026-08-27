# linux_fast_gpio<br>

Copyright (c) 2026 BitBank Software, Inc.<br>
Written by Larry Bank<br>
bitbank@pobox.com<br>
<br>
## What is it?<br>
This is a small demo program to show how you can directly manipulate the GPIO registers from user space on the H618 and RK3399 SoCs (so far). This opens up the possibility of higher speed access as well as features missing from the Linux GPIOD driver such as parallel data read and write.
<br>

## Why did you write it?<br>
I've always been interested in pushing computers to their limits and have found that the Linux GPIO driver is a bottleneck for a lot of interesting use cases of these inexpensive SBCs. The Raspberry Pi has much wider adoption and documentation than the "no name" Arm Linux boards, and there are a few example "bare metal" projects which show how to use the GPIO. I couldn't find anything for the AllWinner H618, so I wanted to share something new for people with those boards.<br>

Below is an image showing the GPIO pin mapping of the Orange Pi Zero 2W. The example code in this project makes it easy to access all of the available GPIOs.<br>

![h618](/opi02w_gpio.png?raw=true "Orange Pi Zero2W GPIO map")

<br>

If you find this code useful, please consider becoming a sponsor or sending a donation.

[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=SR4F44J2UR8S4)
