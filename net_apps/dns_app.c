#include <stdio.h>
#include <log.h>
#include "dns_app.h"
#include "../net/dns.h"
#include "../sys/buf.h"

static void dns_cb(uint32_t ip)
{
	DEBUG_LOG("%s: ip=0x%lX\n", __func__, ip);
}

int dns_resolver_init(void)
{
	sbuf_t sb = SBUF_INITS("free.fr");

	/* set google dns nameserver */
	dns_init(0x08080808);

	return dns_resolve(&sb, dns_cb);
}
