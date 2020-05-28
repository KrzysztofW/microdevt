Upstream Author: Krzysztof Witek

Copyright:
	 Copyright (c) 2017 Krzysztof Witek www.kw-net.com

License: GPL-2

Microdevt Documentation
=======================

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   KW-NET Home page <https://www.kw-net.com>
   Quick-Start-Guide.rst
   Demo-apps.rst
   API.rst

* :ref:`genindex`

Overview
--------

Microdevt is a microcontroller development toolkit for quickly developing
embedded applications.
The toolkit provides asynchronous timers, a task scheduler and a zero-copy,
event-based TCP/IP stack. The asynchronous approach of developing Microdevt
applications allows to benefit as much as possible of the precious and very
limited resources of microcontrollers.
It is distributed freely under the GPL-2 open source license and is designed
for reliability and ease of use.

Supported Devices
-----------------

If you don't see a match of your microcontroller, keep in mind that it's
easy to port Microdevt code to other microcontrollers. (See section
:ref:`uc-port`).

- Atmel AVR familly (Atmega 328P, Attiny85, Atmega2561 and many others)
- Simulavr
- X86 (for testing purposes)

TCP/IP protocols
----------------

- Ethernet
- ARP
- IP
- ICMP
- TCP (basic support with retransmissions, no window support yet)
- UDP
- DNS

Wireless Radio Frequency
------------------------

- Swen level 2 (Data Link Layer)
- Swen level 3 (Network Layer)

The Swen protocols rely on the robust RF driver (*/drivers/rf.c*) that
uses the pulse width modulated encoding to send & receive data.

The Swen L3 protocol is a unicast connection-oriented protocol and it has
been designed to be reliable and secure (uses xtea encription).
It is reliable as it notifies the user application whether or not the delivery
of data to the recipient was successful. It retransmits lost data, detects
data corruptions and uses acknowledgments.

The Swen L2 protocol can be used to send & receive unacknowledged data but also
it can record and replay generic commands from widely used 433Mhz remote
controls and gadgets.
