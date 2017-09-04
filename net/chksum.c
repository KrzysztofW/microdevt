#include <stdint.h>
#include "chksum.h"

uint32_t cksum_partial(const void *data, uint16_t len)
{
	const uint16_t *w = data;
	uint32_t sum = 0;
	uint16_t ret;

	while (len > 1)  {
		sum += *w++;
		len -= 2;
	}

	if (len == 1) {
		*(uint8_t *)(&ret) = *(uint8_t *)w;
		sum += ret;
	}

	return sum;
}

uint16_t cksum(const void *data, uint16_t len)
{
	uint32_t sum = cksum_partial(data, len);

	sum = (sum >> 16) + (sum & 0xffff);
	sum += sum >> 16;

	return ~sum;
}
