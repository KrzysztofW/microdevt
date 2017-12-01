ifdef CONFIG_UDP
ifdef CONFIG_BSD_COMPAT
SOURCES += net_apps/udp_apps_bsd.c
else
SOURCES += net_apps/udp_apps.c
endif
ifdef CONFIG_DNS
SOURCES += net_apps/dns_app.c
endif
endif

ifdef CONFIG_TCP
ifdef CONFIG_BSD_COMPAT
SOURCES += net_apps/tcp_apps_bsd.c
else
SOURCES += net_apps/tcp_apps.c
endif
endif
