#include <stdint.h>
#include "chksum.h"

uint16_t cksum(void *data, int len)
{
	uint16_t *w = data;
	uint32_t sum = 0;
	uint16_t answer = 0;

	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (len > 1)  {
		sum += *w++;
		len -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (len == 1) {
		*(uint8_t *)(&answer) = *(uint8_t *)w ;
		sum += answer;
	}

	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
	sum += (sum >> 16);         /* add carry */
	answer = ~sum;              /* truncate to 16 bits */

	return answer;
}
