ifdef CONFIG_UDP
ifdef CONFIG_BSD_COMPAT
SOURCES += $(NET_APPS_DIR)/udp_apps_bsd.c
else
SOURCES += $(NET_APPS_DIR)/udp_apps.c
endif
ifdef CONFIG_DNS
SOURCES += $(NET_APPS_DIR)/dns_app.c
endif
endif

ifdef CONFIG_TCP
ifdef CONFIG_BSD_COMPAT
SOURCES += $(NET_APPS_DIR)/tcp_apps_bsd.c
else
SOURCES += $(NET_APPS_DIR)/tcp_apps.c
endif
endif
