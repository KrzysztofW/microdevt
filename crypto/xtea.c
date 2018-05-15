#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "xtea.h"

#define DELTA 0x9e3779b9
#define MX (((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (key[(p&3)^e] ^ z)))

int xtea_encode(buf_t *buf, uint32_t const key[4])
{
	uint32_t y, z, sum, *v;
	unsigned p, rounds, e, n;

	/* the buffer must be a multiple of 4 */
	if (buf_pad(buf, 2) < 0)
		return -1;

	n = buf_len(buf) / 4;
	v = (uint32_t *)buf_data(buf);

	rounds = 6 + 52 / n;
	sum = 0;
	z = v[n - 1];
	do {
		sum += DELTA;
		e = (sum >> 2) & 3;
		for (p = 0; p < n - 1; p++) {
			y = v[p+1];
			z = v[p] += MX;
		}
		y = v[0];
		z = v[n - 1] += MX;
	} while (--rounds);
	return 0;
}

int xtea_decode(buf_t *buf, uint32_t const key[4])
{
	uint32_t y, z, sum, *v;
	unsigned p, rounds, e, n;

	/* the buffer must be a multiple of 4 */
	if (buf_len(buf) & 0x3)
		return -1;

	n = buf_len(buf) / 4;
	v = (uint32_t *)buf_data(buf);

	rounds = 6 + 52 / n;
	sum = rounds * DELTA;
	y = v[0];
	do {
		e = (sum >> 2) & 3;
		for (p = n - 1; p > 0; p--) {
			z = v[p - 1];
			y = v[p] -= MX;
		}
		z = v[n - 1];
		y = v[0] -= MX;
		sum -= DELTA;
	} while (--rounds);
	return 0;
}
#if _TEST /* for testing purposes */
int main(int argc, char **argv)
{
	buf_t buf;
	uint32_t key[4] = { 0x11111111, 0x22222222, 0x33333333, 0x44444444 };
	int len;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <string to encode>\n", argv[0]);
		return -1;
	}
	len = strlen(argv[1]) + 1;
	if (len <= 4) {
		fprintf(stderr, "string length must be longer than "
			"3 characters\n");
		return -1;
	}

	/* align the buffer to a multiple of 4 */
	while (len & 3)
		len++;

	buf = BUF(len);
	if (buf_adds(&buf, argv[1]) < 0) {
		fprintf(stderr, "string too big\n");
		return -1;
	}
	printf("\nplaintext buf (len:%d): %s\n", buf_len(&buf), buf.data);
	printf("plaintext buf hex (len:%d):\n", buf_len(&buf));
	buf_print_hex(&buf);

	if (xtea_encode(&buf, key) < 0) {
		fprintf(stderr, "failed to encode\n");
		return -1;
	}
	printf("\nencoded buf (len:%d):\n", buf_len(&buf));
	buf_print_hex(&buf);

	if (xtea_decode(&buf, key) < 0) {
		fprintf(stderr, "failed to decode\n");
		return -1;
	}
	printf("decoded buf hex (len:%d):\n", buf_len(&buf));
	buf_print_hex(&buf);
	printf("plaintext buf (len:%d): %s\n", buf_len(&buf), buf.data);
	return 0;
}
#endif
