// WIIDisc.h: interface for the CWIIDisc class.
//
//////////////////////////////////////////////////////////////////////

#ifndef WIIDISC_H_INCLUDED
#define WIIDISC_H_INCLUDED

#include "misc.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <openssl/aes.h>
#include "structs.h"

using namespace std;

/* Name of the file containing the Wii private key */
#define KEYFILE "key.bin"

#define SIZE_H0						0x0026CUL
#define SIZE_H1						0x000A0UL
#define SIZE_H2						0x000A0UL
#define SIZE_H3						0x18000UL
#define SIZE_H4						0x00014UL
#define SIZE_PARTITION_HEADER		0x20000UL
#define SIZE_CLUSTER				0x08000UL
#define SIZE_CLUSTER_HEADER			0x00400UL
#define SIZE_CLUSTER_DATA			(SIZE_CLUSTER - SIZE_CLUSTER_HEADER)

/*
 * ADDRESSES
 */ 
/* Absolute addresses */ 
#define OFFSET_GAME_TITLE			0x00020UL
#define OFFSET_PARTITIONS_INFO		0x40000UL
#define OFFSET_REGION_BYTE			0x4E003UL
#define OFFSET_REGION_CODE			0x4E010UL
/* Relative addresses */ 
#define OFFSET_H0					0x00000UL
#define OFFSET_H1					0x00280UL
#define OFFSET_H2					0x00340UL
#define OFFSET_PARTITION_TITLE_KEY	0x001BFUL
#define OFFSET_PARTITION_TITLE_ID	0x001DCUL
#define OFFSET_PARTITION_TMD_SIZE	0x002A4UL
#define OFFSET_PARTITION_TMD_OFFSET	0x002A8UL
#define OFFSET_PARTITION_H3_OFFSET	0x002B4UL
#define OFFSET_PARTITION_INFO		0x002B8UL
#define OFFSET_CLUSTER_IV			0x003D0UL
#define OFFSET_FST_NB_FILES			0x00008UL
#define OFFSET_FST_ENTRIES			0x0000CUL
#define OFFSET_TMD_HASH				0x001F4UL

/*
 * OTHER
 */ 
#define NB_CLUSTER_GROUP			64
#define NB_CLUSTER_SUBGROUP			8

typedef enum {
	SCRUB_SCRUB_ONLY,
	SCRUB_DIFF_ONLY,
	SCRUB_SCRUB_AND_DIFF,
} scrubMode;
	

// typedef enum {
// 	SCRUB_NORMAL,
// 	SCRUB_TRUCHA
// } scrubType;

typedef enum {
	SCRUB_REMOVE_HEADERS,
	SCRUB_KEEP_HEADERS
} scrubHeadersMode;

#define KEY_LENGTH 16

class CWIIDisc {
public:
	CWIIDisc (void);
	virtual ~CWIIDisc ();
	
	bool CleanupISO (char *csFileIn, char *csFileOut, char *csFileDiff, scrubHeadersMode nHeaderMode);
	
	struct image_file *image_init (char *filename, bool force_wii);
	
	void Reset (void);
	
	bool RecreateOriginalFile (char *scrubbed, char *diff, char *output);
	
	bool ExtractPartitionFiles (struct image_file *image, u_int32_t nPartition, const char * cDirPathName);
	
	bool DoPartitionShrink (struct image_file *image,
							u_int32_t nPartition);
	
	bool LoadDecryptedPartition (string csName,
								 struct image_file *image,
								 u_int32_t nPartition);
	
	bool SaveDecryptedPartition (string csName,
								 struct image_file *image,
								 u_int32_t nPartition);
	
	bool DoTheShuffle (struct image_file *image);
	
	u_int64_t GetFreePartitionStart (struct image_file *image);
	
	u_int64_t GetFreeSpaceAtEnd (struct image_file *image);
	
	bool AddPartition (struct image_file *image, bool bChannel,
					   u_int64_t nOffset, u_int64_t nDataSize,
					   u_int8_t * pText);
	
	bool SetBootMode (struct image_file *image);
	
	int64_t nImageSize;
	
	
	bool ResizePartition (struct image_file *image,
						  u_int32_t nPartition);
	
	bool DeletePartition (struct image_file *image,
						  u_int32_t nPartition);
	
	bool CheckForFreeSpace (struct image_file *image,
							u_int32_t nPartition, u_int64_t nOffset,
							u_int32_t nBlocks);
	
	u_int64_t FindRequiredFreeSpaceInPartition (struct image_file *image,
												u_int64_t nPartition,
												u_int32_t nRequiredSize);
	

	
	
	bool wii_write_data_file (struct image_file *iso, int partition,
							  u_int64_t offset, u_int64_t size,
							  u_int8_t * in, FILE * fIn = NULL);
	
	bool wii_write_clusters (struct image_file *iso, int partition,
							 int cluster, u_int8_t * in,
							 u_int32_t nClusterOffset,
							 u_int32_t nBytesToWrite, FILE * fIn);
	
	int wii_read_data (struct image_file *iso, int partition,
					   u_int64_t offset, u_int32_t size,
					   u_int8_t ** out);
	
	bool wii_read_cluster_hashes (struct image_file *iso, int partition,
								  int cluster, u_int8_t * h0,
								  u_int8_t * h1, u_int8_t * h2);
	
	bool wii_write_cluster (struct image_file *iso, int partition,
							int cluster, u_int8_t * in);
	
	int wii_read_cluster (struct image_file *iso, int partition,
						  int cluster, u_int8_t * data,
						  u_int8_t * header);
	
	bool wii_calc_group_hash (struct image_file *iso, int partition,
							  int cluster);
	
	int wii_nb_cluster (struct image_file *iso, int partition);
	
	
	bool wii_trucha_signing (struct image_file *image, int partition);
	
	bool DiscWriteDirect (struct image_file *image, u_int64_t nOffset,
						  u_int8_t * pData, unsigned int nSize);
	
	void MarkAsUnused (u_int64_t nOffset, u_int64_t nSize);
	
	bool MergeAndRelocateFSTs (unsigned char *pFST1,
							   u_int32_t nSizeofFST1,
							   unsigned char *pFST2,
							   u_int32_t nSizeofFST2,
							   unsigned char *pNewFST,
							   u_int32_t * nSizeofNewFST,
							   u_int64_t nNewOffset,
							   u_int64_t nOldOffset);
	
	bool TruchaScrub (struct image_file *image, unsigned int nPartition);
	
	bool SaveDecryptedFile (string csDestinationFilename,
							struct image_file *image, 
							u_int32_t part,
							u_int64_t nFileOffset, u_int64_t nFileSize,
							bool bOverrideEncrypt = false);
	
	bool LoadDecryptedFile (string csDestinationFilename,
							struct image_file *image, 
							u_int32_t part,
							u_int64_t nFileOffset, u_int64_t nFileSize,
							int nFSTReference);
		
	void MarkAsUsed (u_int64_t nOffset, u_int64_t nSize);
	
	void MarkAsUsedDC (u_int64_t nPartOffset, u_int64_t nOffset,
					   u_int64_t nSize, bool bIsEncrypted);
	
	unsigned int CountBlocksUsed ();
	
	u_int32_t parse_fst (u_int8_t * fst, const char *names, u_int32_t i,
						 struct tree *tree, struct image_file *image,
						 u_int32_t part);
	
	u_int8_t get_partitions (struct image_file *image);
	
	void tmd_load (struct image_file *image, u_int32_t part);
	
	void tmd_free (struct tmd *tmd);
		
	size_t io_read_part (unsigned char *ptr, size_t size,
						 struct image_file *image, u_int32_t part,
						 u_int64_t offset);
	
	int decrypt_block (struct image_file *image, u_int32_t part,
					   u_int32_t block);
	
	
	
	string m_csText;
	int image_parse (struct image_file *image);		// TODO Make private
	void image_deinit (struct image_file *image);
private:
	u_int64_t tableLen;
	u_int8_t *pFreeTable;
	u_int8_t *pBlankSector;
	u_int8_t *pBlankSector0;
	
	// save the tables instead of reading and writing them all the time
	u_int8_t h3[SIZE_H3];
	u_int8_t h4[SIZE_H4];

	u_int8_t image_parse_header (struct part_header *header, u_int8_t *buffer, bool forceWii);

	int io_read (u_int8_t *ptr, size_t size, struct image_file *image, u_int64_t offset);

	bool CheckAndLoadKey (struct image_file *image = NULL);
	
	void AddToLog (string csText);
	
	void AddToLog (string csText, u_int64_t nValue);
	
	void AddToLog (string csText, u_int64_t nValue1, u_int64_t nValue2);
	
	void AddToLog (string csText, u_int64_t nValue1, u_int64_t nValue2,
		       u_int64_t nValue3);
	
	void aes_cbc_dec (u_int8_t * in, u_int8_t * out, u_int32_t len,
			  u_int8_t * key, u_int8_t * iv);
	
	void aes_cbc_enc (u_int8_t * in, u_int8_t * out, u_int32_t len,
			  u_int8_t * key, u_int8_t * iv);
	
	void sha1 (u_int8_t * data, u_int32_t len, u_int8_t * hash);
	
	u_int32_t parse_fst_and_save (u_int8_t * fst, const char *names, u_int32_t i, struct image_file *image, u_int32_t part);
	
	u_int64_t FindFirstData (u_int64_t nStartOffset, u_int64_t nLength, bool bUsed = true);
	
	bool CopyDiscDataDirect (struct image_file *image, int nPart, u_int64_t nSource, u_int64_t nDest, u_int64_t nLength);
	
	u_int64_t SearchBackwards (u_int64_t nStartPosition, u_int64_t nEndPosition);
	
	void FindFreeSpaceInPartition (int64_t nPartOffset, u_int64_t * pStart, u_int64_t * pSize);
	
	void Write32 (u_int8_t * p, u_int32_t nVal);
};


#endif
