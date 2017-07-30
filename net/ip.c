#include "ip.h"
#include "icmp.h"

void ip_input(buf_t buf, const iface_t *iface)
{
	ip_hdr_t *ip;
	uint32_t *ip_addr = (uint32_t *)iface->ip4_addr;

	if (buf.len == 0) {
		goto error;
	}

	ip = btod(&buf, ip_hdr_t *);

	if (ip->v != 4 || ip->dst != *ip_addr || ip->ttl == 0)
		goto error;
	if (ip->off & IP_MF) {
		/* ip fragmentation is unsupported */
		goto error;
	}

#ifdef CONFIG_IP_CHKSUM
#endif

	buf_adj(&buf, sizeof(ip_hdr_t));

	switch (ip->p) {
	case IPPROTO_ICMP:
		icmp_input(buf, iface);
		return;

#ifdef CONFIG_IPV6
	case IPPROTO_ICMPV6:
		icmp6_input(buf, iface);
		return;
#endif
	case IPPROTO_TCP:
		//tcp_input(buf, iface);
		return;

	case IPPROTO_UDP:
		//udp_input(buf, iface);
		return;
	default:
		/* unsupported protocols */
		break;
	}

 error:
	/* inc stats */
	return;
}
