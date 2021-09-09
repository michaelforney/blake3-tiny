#ifndef BLAKE3_H
#define BLAKE3_H 1

#include <stdint.h>

struct blake3 {
	unsigned char input[64];      /* current input bytes */
	unsigned bytes;               /* bytes in current input block */
	unsigned block;               /* block index in chunk */
	uint64_t chunk;               /* chunk index */
	uint32_t *cv, cv_buf[54 * 8]; /* chain value stack */
};

void blake3_init(struct blake3 *);
void blake3_update(struct blake3 *, const void *, size_t);
void blake3_out(struct blake3 *, unsigned char *);

#endif
