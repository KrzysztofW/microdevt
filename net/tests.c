#include "config.h"
#include "tests.h"

iface_t iface;

int eth_hdr_test(buf_t pkt)
{
	eth_input(pkt, &iface);
	return 0;
}

int net_tests(void)
{
	buf_t pkt;

	if (buf_read_file(&pkt, "net/arp.raw") < 0)
		return -1;
	buf_print(pkt);
	buf_free(&pkt);
	return 0;
}

