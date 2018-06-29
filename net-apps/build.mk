ifdef CONFIG_UDP
ifdef CONFIG_BSD_COMPAT
SOURCES += $(NET_APPS_DIR)/udp-apps-bsd.c
else
SOURCES += $(NET_APPS_DIR)/udp-apps.c
endif
ifdef CONFIG_DNS
SOURCES += $(NET_APPS_DIR)/dns-app.c
endif
endif

ifdef CONFIG_TCP
ifdef CONFIG_BSD_COMPAT
SOURCES += $(NET_APPS_DIR)/tcp-apps-bsd.c
else
SOURCES += $(NET_APPS_DIR)/tcp-apps.c
endif
endif
