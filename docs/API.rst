API Reference
=============

Task Scheduler
--------------

.. doxygenfunction:: schedule_task
   :project: doxygen

.. doxygenfunction:: scheduler_run_task
   :project: doxygen

.. doxygenfunction:: scheduler_run_tasks
   :project: doxygen

Timers
------

.. doxygendefine:: TIMER_INIT
   :project: doxygen

.. doxygenfunction:: timer_subsystem_init
   :project: doxygen

.. doxygenfunction:: timer_subsystem_shutdown
   :project: doxygen

.. doxygenfunction:: timer_init
   :project: doxygen

.. doxygenfunction:: timer_add
   :project: doxygen

.. doxygenfunction:: timer_reschedule
   :project: doxygen

.. doxygenfunction:: timer_del
   :project: doxygen

.. doxygenfunction:: timer_process
   :project: doxygen

.. doxygenfunction:: timer_is_pending
   :project: doxygen

.. _pwr-mgr:

Power Management
----------------

.. doxygendefine:: power_management_pwr_down_reset
   :project: doxygen

.. doxygendefine:: power_management_set_inactivity
   :project: doxygen

.. doxygenfunction:: power_management_power_down_init
   :project: doxygen

.. doxygenfunction:: power_management_power_down_enable
   :project: doxygen

.. doxygenfunction:: power_management_power_down_disable
   :project: doxygen

Buffers
-------

.. doxygenfile:: buf.h
   :project: doxygen

Circular ring buffers
---------------------

.. doxygenfile:: ring.h
   :project: doxygen

Lists
-----

.. doxygenfile:: list.h
   :project: doxygen

Byte
----

.. doxygenfile:: byte.h
   :project: doxygen

Misc Utils
----------

.. doxygenfile:: sys/utils.h
   :project: doxygen

Networking
----------

Packet memory pool
~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: pkt_mempool_init
   :project: doxygen

.. doxygenfunction:: pkt_mempool_shutdown
   :project: doxygen

.. doxygenfunction:: pkt_get
   :project: doxygen

.. doxygenfunction:: pkt_put
   :project: doxygen

.. doxygenfunction:: pkt_alloc
   :project: doxygen

.. doxygenfunction:: pkt_free
   :project: doxygen

.. doxygenfunction:: pkt_alloc_emergency
   :project: doxygen

.. doxygenfunction:: pkt_is_emergency
   :project: doxygen

.. doxygenfunction:: pkt_len
   :project: doxygen

.. doxygenfunction:: pkt_retain
   :project: doxygen

.. doxygenfunction:: pkt_pool_get_nb_free
   :project: doxygen

.. doxygenfunction:: pkt_get_traced_pkts
   :project: doxygen


Socket API
~~~~~~~~~~

.. doxygenenum:: sock_status
   :project: doxygen

.. doxygenstruct:: sock_info
   :project: doxygen
   :members:
   :undoc-members:

.. doxygenfunction:: socket_event_register
   :project: doxygen

.. doxygenfunction:: socket_event_register_fd
   :project: doxygen

.. doxygenfunction:: socket_event_unregister
   :project: doxygen

.. doxygenfunction:: socket_event_set_mask
   :project: doxygen

.. doxygenfunction:: socket_event_clear_mask
   :project: doxygen

.. doxygenfunction:: socket_event_get_sock_info
   :project: doxygen

.. doxygenfunction:: socket
   :project: doxygen

.. doxygenfunction:: close
   :project: doxygen

.. doxygenfunction:: bind
   :project: doxygen

.. doxygenfunction:: listen
   :project: doxygen

.. doxygenfunction:: accept
   :project: doxygen

.. doxygenfunction:: connect
   :project: doxygen

.. doxygenfunction:: sendto
   :project: doxygen

.. doxygenfunction:: recvfrom
   :project: doxygen

.. doxygenfunction:: socket_get_pkt
   :project: doxygen

.. doxygenfunction:: socket_put_sbuf
   :project: doxygen

.. doxygenfunction:: __socket_get_pkt
   :project: doxygen

.. doxygenfunction:: __socket_put_sbuf
   :project: doxygen

.. doxygenfunction:: sock_info_init
   :project: doxygen

.. doxygenfunction:: sock_info_bind
   :project: doxygen

.. doxygenfunction:: sock_info_close
   :project: doxygen

.. doxygenfunction:: sock_info_listen
   :project: doxygen

.. doxygenfunction:: sock_info_accept
   :project: doxygen

.. doxygenfunction:: sock_info_connect
   :project: doxygen

.. doxygenfunction:: sock_info_state
   :project: doxygen

Swen (level 2) API
~~~~~~~~~~~~~~~~~~

.. doxygenenum:: generic_cmd_status
   :project: doxygen

.. doxygenfunction:: swen_sendto
   :project: doxygen

.. doxygenfunction:: swen_generic_cmds_init
   :project: doxygen

.. doxygenfunction:: swen_generic_cmds_start_recording
   :project: doxygen

.. doxygenfunction:: swen_ev_set
   :project: doxygen

.. doxygenfunction:: swen_generic_cmds_get_list
   :project: doxygen

.. doxygenfunction:: swen_generic_cmds_delete_recorded_cmd
   :project: doxygen

Swen (level 3) API
~~~~~~~~~~~~~~~~~~

.. doxygenenum:: swen_l3_state
   :project: doxygen

.. doxygenfunction:: swen_l3_get_state
   :project: doxygen

.. doxygenfunction:: swen_l3_assoc_init
   :project: doxygen

.. doxygenfunction:: swen_l3_assoc_shutdown
   :project: doxygen

.. doxygenfunction:: swen_l3_associate
   :project: doxygen

.. doxygenfunction:: swen_l3_disassociate
   :project: doxygen

.. doxygenfunction:: swen_l3_is_assoc_bound
   :project: doxygen

.. doxygenfunction:: swen_l3_assoc_bind
   :project: doxygen

.. doxygenfunction:: swen_l3_send
   :project: doxygen

.. doxygenfunction:: swen_l3_send_buf
   :project: doxygen

.. doxygenfunction:: swen_l3_get_pkt
   :project: doxygen

.. doxygenfunction:: swen_l3_event_register
   :project: doxygen

.. doxygenfunction:: swen_l3_event_unregister
   :project: doxygen

.. doxygenfunction:: swen_l3_event_set_mask
   :project: doxygen

.. doxygenfunction:: swen_l3_event_clear_mask
   :project: doxygen

.. doxygenfunction:: swen_l3_event_get_assoc
   :project: doxygen
