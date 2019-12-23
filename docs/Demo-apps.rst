Demo Applications
=================

Hello World
-----------

*/apps/hello-world*

A basic AVR application with frequently used features:

- Analog to digital converter
- Watchdog
- Watchdog interrupt to wake up the microcontroller
- USAR input/output
- Power management
- Timers
- Tasks
- GPIOs

This simple application reads input from the serial console and prints
the relative humidity and temperature.
The humidity and temperature values are read every 10 seconds using a timer
and stored locally.
In case of inactivity, the app goes to sleep (power down mode).
In addition, the scheduler will make sure that if there are no pending tasks
the microcontroller will go the IDLE power save mode.
A timer makes a led blink every second and a PIR sensor wakes
the app up (see gpio.h file for PIN assignments).

Network Applications
--------------------

*/net-apps*

The network example applications show the usage of TCP, UDP and DNS protocols.
The esiest way to explore them is to use the X86 tun-driver app in
*/apps/tun-driver* folder.

By default, the tun-driver runs a TCP echo server. For different usages modify
the ``config`` file in */apps/tun-driver* folder.

To run the tun-driver apps:

.. code:: shell

    cd apps/tun-driver
    make run

Then connect from a different terminal to the server and type some text:

.. code:: shell

   nc 1.1.2.2 777
   the typed text will be echoed back
