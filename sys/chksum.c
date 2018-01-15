#include <stdint.h>
#include "chksum.h"

uint32_t cksum_partial(const void *data, uint16_t len)
{
	const uint16_t *w = data;
	uint32_t sum = 0;

	while (len > 1)  {
		sum += *w++;
		len -= 2;
	}

	if (len == 1)
		sum += *(uint8_t *)w;

	return sum;
}

uint16_t cksum_finish(uint32_t csum)
{
	csum = (csum >> 16) + (csum & 0xffff);
	csum += csum >> 16;

	return ~csum;
}

uint16_t cksum(const void *data, uint16_t len)
{
	return cksum_finish(cksum_partial(data, len));
}
