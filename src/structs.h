// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#ifndef STRUCTS_H_INCLUDED
#define STRUCTS_H_INCLUDED

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <openssl/aes.h>


enum tmd_sig {
	SIG_UNKNOWN = 0,
	SIG_RSA_2048,
	SIG_RSA_4096
};


struct tmd_content {
	u_int32_t cid;
	u_int16_t index;
	u_int16_t type;
	u_int64_t size;
	u_int8_t hash[20];
};


struct tmd {
	enum tmd_sig sig_type;
	u_int8_t * sig;
	char issuer[64];
	u_int8_t version;
	u_int8_t ca_crl_version;
	u_int8_t signer_crl_version;
	u_int64_t sys_version;
	u_int64_t title_id;
	u_int32_t title_type;
	u_int16_t group_id;
	u_int32_t access_rights;
	u_int16_t title_version;
	u_int16_t num_contents;
	u_int16_t boot_index;
	struct tmd_content *contents;
};


struct part_header {
	char console;
	u_int8_t is_gc;
	u_int8_t is_wii;
	char gamecode[2];
	char region;
	char publisher[2];
	u_int8_t has_magic;
	char name[0x60];
	u_int64_t dol_offset;
	u_int64_t dol_size;
	u_int64_t fst_offset;
	u_int64_t fst_size;
};


enum partition_type {
	PART_UNKNOWN = 0,
	PART_DATA,
	PART_UPDATE,
	PART_INSTALLER,
	PART_VC
};


struct partition {
	u_int64_t offset;
	struct part_header header;
	u_int64_t appldr_size;
	u_int8_t is_encrypted;
	u_int64_t tmd_offset;
	u_int64_t tmd_size;
	struct tmd *tmd;
	u_int64_t h3_offset;
	char title_id_str[17];
	enum partition_type type;
	char chan_id[5];
	char key_c[35];
	AES_KEY key;
	u_int8_t title_key[16];
	u_int64_t data_offset;
	u_int64_t data_size;
	u_int64_t cert_offset;
	u_int64_t cert_size;
	u_int8_t dec_buffer[0x8000];
	u_int32_t cached_block;
	u_int8_t cache[0x7c00];
};


struct image_file {
	FILE *fp;
	u_int8_t is_wii;
	struct partition *parts;
	struct stat st;
	u_int64_t nfiles;
	u_int64_t nbytes;
	u_int8_t PartitionCount;
	u_int8_t ChannelCount;
	u_int64_t part_tbl_offset;
	u_int64_t chan_tbl_offset;
	AES_KEY key;
};

#endif
