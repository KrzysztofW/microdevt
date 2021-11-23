Quick Start Guide
=================

Asynchronous Timers
-------------------

The timer implementation allows to define a minimum system tick duration which
triggers calls to scheduled timers.
The tick duration is defined by the CONFIG_TIMER_RESOLUTION_US option.
It's unit is a microsecond and should be set according to the microcontroller
specification and the user needs. The timer timeouts will be rounded to
the nearest multiple of this value.

Example of a timer usage:

.. code-block:: C

    #include <sys/timer.h>

    tim_t my_timer = TIMER_INIT(my_timer);
    #define MY_TIMER_TIMEOUT 1000000 /* one second */

    void callback(void *arg)
    {
        LOG("timeout!\n");
        timer_reschedule(&my_timer, MY_TIMER_TIMEOUT);
    }

    int main()
    {
        timer_subsystem_init();
        timer_add(&my_timer, 1000000, MY_TIMER_TIMEOUT, NULL)
        while (1) {
            /* infinite loop */
        }
    }

Asynchronous Task Scheduler
---------------------------

The task scheduler allows to schedule interruptible tasks that are too big
to be executed in an interrupt. They should be used as bottom halves of
interrupt handler functions like in an OS.
Tasks can be scheduled to execute in response to interrupts, events, or
anything that needs to postpone some work.

There are many implementations of real task schedulers with priorities and
context switching. This asynchronous task scheduler uses a different approach
for the sake of clarity and simplicity. It only provides a simple way to better
handle interrupts. The scheduling is not periodic, a scheduled task will run
only once and exit. If there is a need to execute a task periodically,
the correct way of doing that is to create a timer (see :ref:`timers`) that
schedules a task upon its expiration. The scheduled task would have to re-arm
that timer again so the task gets executed continuously.
There are only two levels of priorities: the tasks that have been scheduled from
an interrupt handler have higher priority than tasks scheduled form other tasks.

Also, note that the proper way of sharing data between an interruptible task
and an interrupt handler function is to use :ref:`ring_bufs`.
In order to handle interrupts efficiently, an interrupt handler should only fill
a ring buffer with data and schedule a task to process that data.

The scheduler uses the power management features of the microcontroller.
That is, if there are no tasks to execute, the microcontroller will go to
the idle state and reduce the power consumption.

System utilities
----------------

There are a lot of useful functions and macros in the "sys" directory.
Among others pay attention to the already implemented lists, buffers and
interrupt safe ring buffers.
These data structures are supposed to be used in drivers and user applications.

.. _uc-port:

Microdevt Port
--------------

Adding support of new microcontrollers to Micodevt is relatively easy.
All needed functions should be defined in the "arch" directory along with a
common makefile for the new targets.

Basically, the needed functions that should be ported are:

- UART
- IO for serial communications
- interrupt enable/disable functions
- timer interrupts
- power management
- watchdog
- ADC
- eeprom
- optimized print function (LOG() and DEBUG_LOG() macros)
- delay utils
- atomic integer operations

Cryptography
------------

Currently, the following cryptography algorithms are supported:

- lightweight xtea

Power Management
----------------

Microdevt allows to easily use the power management features of supported
microcontrollers.
(see :ref:`pwr-mgr` API section)

Debugging
---------

There are several tools that allow to simplify application debugging.
The DEBUG_LOG() macro, allows to print text to a terminal console.
This macro is only compiled if the global DEBUG environment variable is set to
1 at compile time: DEBUG=1 make.

STATIC_ASSERT()/STATIC_IF() - macros that allow to make assertions at compile
time. They have no over head in the produced final binary program.

For Atmel AVR microcontroller family, it is possible to run applications
on a simulator. The supported simulator is "simulavr". Its code source is not
provided with Microdevt.

Example of configuration:

.. code:: shell

    CONFIG_AVR_SIMU=y
    CONFIG_AVR_SIMU_PATH=/<some path>/simulavr
    CONFIG_AVR_SIMU_MCU=atmega328

The network applications can be easily debugged on a Linux x86 host using the
tun-driver application in the "apps" folder. This powerful application allows
to set up the whole TCP/IP stack and use virtual TUN/TAP interfaces to send
and receive network packets. Tcpdump can be used to capture all inbound and
outbound packets. In addition, it can be run with GDB making the debugging of
network applications very easy.

Unitary tests
-------------

All the unitary tests are held in the "apps/tests" folder.
The network unitary tests are in "net/tests.c" file.
These tests are meant to check basic functionality, parsing and serializing
data structures in protocols.

Interrupt-based Drivers
-----------------------

The interrupt-based drivers should be implemented in quite similar way they are
in modern kernels:

An interrupt function handler (a top half) should handle the interrupt as fast
as possible then create a task (a bottom half) and schedule it for later
processing.
To pass data from the top half to the bottom half and vice versa, interrupt
safe data structures must be used such as circular ring buffers (see sys/ring.h).
These buffers are single reader / single writer circular ring buffers.
This means that only one ring is needed for a driver in which the interrupt
function handler produces data (inserts bytes to the ring) and the task (that
can be seen as a work queue with heavy stuff to do) consumes them.
In case there is a need for a bidirectional communication between a task and
an interrupt handler, two ring buffers are needed.

See the example of a very simple interrupt-based UART driver implementation:

.. code:: C

    #define UART_RING_SIZE 32
    STATIC_RING_DECL(uart_ring, UART_RING_SIZE);

    static void uart_parse_buffer(buf_t *buf)
    {
        /* do some heavy parsing */
        buf_print(buf);
    }

    static void uart_task(void *arg)
    {
        buf_t buf = BUF(UART_RING_SIZE);
        uint8_t c;

        while (ring_getc(uart_ring, &c) >= 0) {
            if (c == '\0' || buf_addc(&buf, c) < 0)
                break;
        }
        if (buf.len)
            uart_parse(&buf);
    }

    static void uart_interrupt_handler(int c)
    {
        if (c == '\r')
            return;
        if (c == '\n') {
            c = '\0';
            schedule_task(uart_task, NULL);
            return;
        }

        if (ring_addc(uart_ring, c) < 0) {
            /* handle excess of data */
            schedule_task(uart_task, NULL);
            ring_reset(uart_ring);
        }
    }

This simple driver reads bytes from the UART device and stores them in a
circular ring buffer. When there is enough data in the ring it schedules a
task that copies these data in a linear buffer and parses it (here it
only displays its content).

Event-based Networking
----------------------

Basically, on reception, a network driver handles the reception of network
packets, schedules a task that passes the packets to the network TCP/IP stack
which (after decapsulating all network layers) calls the user application
callback asynchronously upon a read event. The application receives a buffer
that points directly to the packet payload which avoids copying of the payload.
Similarly, on sending, the application passes a buffer with its data to
the network layer which allocates a packet and copies the data. Then the
packet is put in queue in the driver's tx bucket and the drivers sends it.

Doing things in this ways avoids any time consuming copies and busy waiting
for packets or user data to be available. When there is no network data,
the microcontroller has no overhead in processing internal functions.

How it works:
Given one interface and an interrupt based driver (which receives and sends
packets on interrupts), a minimum of 3 buckets (network packet queues) are
needed.

- Bucket (1) of free packets (packet pool) that only the driver can allocate packets from,
- Receive bucket (2) filled by the driver and read by a task,
- Transmit bucket (3) filled by a task and read by the driver

Only a task or a user application is allowed to allocate and free packets from
the main packet pool. The driver has to schedule the receive task to free
packets it does not use anymore and the task has to refill driver's free packet
pool each time it takes a packet from the receive bucket (2).

This is because there can only be one reader & one writer of a bucket:
one end is the task and the other end is the interrupt handler (the driver).

The application buckets and the driver buckets are stored in the iface_t
structure. These are defined at boot time in the interface initialization.

Events
------

There are 4 types of events:

- read    (occurs when the network socket has data to be read)
- write   (occurs when the network socket can send data)
- error   (occurs on errors)
- hungup  (occurs on normal connection closures)

When developing new protocols the event_t C structure that handles the events
has to be part of the underlying socket structure used for handling the network
stack.

In the case of a TCP or a UDP socket, the events are registered using the
socket_event_register() function which indicates which event should wake
the application up.
For more details see the examples of network applications in "net-apps" folder.
