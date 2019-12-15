Upstream Author: Krzysztof Witek

Copyright:
	 Copyright (c) 2017 Krzysztof Witek kw-net.com

License: GPL-2

Microdevt Documentation
=======================

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   Quick-Start-Guide.rst
   Demo-apps.rst
   API.rst

* :ref:`genindex`

Overview
--------

Micro-Devt is a microcontroller development toolkit for quickly developing
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
"Microdevt Port").

- Atmel AVR familly (Atmega 328P, Attiny85, Atmega2561 and many others)
- Simulavr
- X86 (for testing purposes)
