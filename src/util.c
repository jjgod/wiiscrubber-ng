#include "misc.h"


u_int16_t be16 (const u_int8_t *p) {
	return (p[0] << 8) | p[1];
}


u_int32_t be32 (const u_int8_t *p) {
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}


u_int64_t be64 (const u_int8_t *p) {
	return ( ( u_int64_t ) be32 (p) << 32 ) | be32 (p + 4);
}


u_int32_t get_dol_size (const u_int8_t *header) {
	u_int8_t i;
	u_int32_t offset, size;
	u_int32_t max = 0;

	// iterate through the 7 code segments
	for (i = 0; i < 7; ++i) {
		offset = be32 (&header[i * 4]);
		size = be32 (&header[0x90 + i * 4]);
		if (offset + size > max)
			max = offset + size;
	}

	// iterate through the 11 data segments
	for (i = 0; i < 11; ++i) {
		offset = be32 (&header[0x1c + i * 4]);
		size = be32 (&header[0xac + i * 4]);
		if (offset + size > max)
			max = offset + size;
	}

	return (max);
}

