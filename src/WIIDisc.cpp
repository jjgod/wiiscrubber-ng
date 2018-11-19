// WIIDisc.cpp: implementation of the CWIIDisc class.
//
//////////////////////////////////////////////////////////////////////

#include "misc.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <openssl/ssl.h>
#include "util.h"
#include "WIIDisc.h"
//#include "ResizePartition.h"
//#include "BootMode.h"

//suk
#define AfxMessageBox(...) {printf (__VA_ARGS__); printf ("\n");}


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

using namespace std;


/* Trucha signature */
u_int8_t trucha_signature[256] = {
	0x57, 0x61, 0x4E, 0x69, 0x4E, 0x4B, 0x6F, 0x4B,
	0x4F, 0x57, 0x61, 0x53, 0x48, 0x65, 0x52, 0x65,
	0x21, 0x8A, 0xB5, 0xBC, 0x89, 0x00, 0x8E, 0x5C,
	0x2B, 0xB6, 0x3E, 0x4D, 0x0A, 0xD7, 0xD2, 0xC4,
	0x97, 0x36, 0x82, 0xDF, 0x57, 0x06, 0x37, 0x27,
	0x96, 0xF1, 0x40, 0xD6, 0xCD, 0x36, 0xE4, 0xEE,
	0xC0, 0x99, 0xAA, 0x49, 0x99, 0x38, 0xA5, 0xC5,
	0xEE, 0xE3, 0x12, 0xF8, 0xBB, 0xE4, 0xBC, 0x52,
	0x1A, 0x3F, 0x31, 0x71, 0x45, 0x68, 0x98, 0xDB,
	0x5A, 0xD9, 0xB2, 0x27, 0x0F, 0x96, 0x15, 0xCF,
	0x2F, 0xBF, 0x18, 0xC8, 0xF7, 0xBD, 0x8D, 0xE5,
	0xA1, 0x9F, 0xDE, 0x5C, 0x83, 0x9A, 0xAE, 0x9D,
	0xD9, 0xDF, 0x0F, 0x1E, 0x47, 0xA7, 0xFA, 0xA1,
	0x80, 0xAC, 0xC8, 0x8F, 0x42, 0xDD, 0x5E, 0x71,
	0x9C, 0x76, 0x39, 0x93, 0x34, 0xC7, 0x79, 0xD5,
	0x66, 0x57, 0x31, 0xEA, 0xF1, 0xDF, 0x87, 0xCB,
	0xBE, 0x96, 0xE9, 0x05, 0x3E, 0xE3, 0xA7, 0xBE,
	0x8F, 0x6F, 0x4E, 0xD1, 0x4D, 0xAC, 0x42, 0xE9,
	0x23, 0x7C, 0x7D, 0x57, 0x43, 0xF6, 0x2C, 0xA9,
	0x4D, 0x5D, 0x93, 0x3E, 0x3C, 0x1B, 0x09, 0xFA,
	0xB1, 0xF3, 0xFF, 0xEF, 0xD6, 0xA6, 0xAE, 0x66,
	0x16, 0xFC, 0x37, 0x63, 0xA8, 0x7A, 0x4C, 0xCB,
	0xF6, 0xC9, 0x22, 0x39, 0xBF, 0x4E, 0xE2, 0x0C,
	0xAB, 0x76, 0x4B, 0xE7, 0x91, 0x54, 0xE1, 0x42,
	0x47, 0xE1, 0x32, 0x1E, 0x87, 0xE0, 0x84, 0x9D,
	0xDC, 0xBB, 0x00, 0x84, 0x35, 0x4D, 0x50, 0x2B,
	0x16, 0x72, 0x64, 0xD6, 0xC1, 0x47, 0xE2, 0x6C,
	0xBD, 0x2D, 0x54, 0x4E, 0x82, 0x35, 0x90, 0xC9,
	0x16, 0xC2, 0xE7, 0x9E, 0xA2, 0x6B, 0x3B, 0x7E,
	0x27, 0x3C, 0x03, 0x5C, 0x89, 0x53, 0x88, 0x9F,
	0xC5, 0xEC, 0x75, 0x86, 0x33, 0x58, 0xF3, 0xF0,
	0x85, 0x47, 0x3E, 0x07, 0x7C, 0xCF, 0xD1, 0x93
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWIIDisc::CWIIDisc (void) {
	// Create and blank the Wii blank table
	tableLen = (u_int64_t) (4699979776LL) / (u_int64_t) (0x8000) * 2;
	pFreeTable = (u_int8_t *) malloc (tableLen);
	memset (pFreeTable, 0, tableLen);
	MarkAsUsed (0, 0x40000);			// Set the header size to used
	
	pBlankSector = (u_int8_t *) malloc (0x8000);
	memset (pBlankSector, 0xFF, 0x8000);
	
	pBlankSector0 = (u_int8_t *) malloc (0x8000);
	memset (pBlankSector0, 0, 0x8000);
}


CWIIDisc::~CWIIDisc () {
	// Free up the memory
	free (pFreeTable);
	free (pBlankSector);
	free (pBlankSector0);
}


#define BUFSIZE ((u_int64_t) 0x8000)

/* This is the function that performs the actual scrubbing */
bool CWIIDisc::CleanupISO (char *csFileIn, char *csFileOut, char *csFileDiff, scrubHeadersMode nHeaderMode) {
	FILE *fIn = NULL;
	FILE *fOut = NULL;
	FILE *fOutDif = NULL;
	bool bStatus = true;	// Used for error checking on read/write
	u_int8_t inData[BUFSIZE];
	
	if (csFileOut) {
		if (!(fOut = fopen (csFileOut, "wb"))) {
			AddToLog ("Unable to create save filename");
			return false;
		}
	}

	if (csFileDiff) {
		// now open the dif file as well
		fOutDif = fopen (csFileDiff, "wb");
		if (NULL == fOutDif) {
			AddToLog ("Unable to create dif filename");
			// close the other output file
			fclose (fOut);
			return false;
		}
	}
	
	if ((fIn = fopen (csFileIn, "rb"))) {
		// open the in and out files
		// read the inblock of 32K
		// check to see if we have to write it -  allow for bigger discs now
		// as well as smaller ones
		u_int64_t i;
		for (i = 0; ((i < (nImageSize / BUFSIZE)) && (!feof (fIn))); i++) {
			bStatus *= (fread (inData, 1, BUFSIZE, fIn) == BUFSIZE);
			if (0x01 == pFreeTable[i]) {
				// block is marked as used so...
				if (fOut)
					bStatus *= (BUFSIZE == fwrite (inData, 1, BUFSIZE, fOut));
				if (fOutDif)
					bStatus *= (BUFSIZE == fwrite (pBlankSector0, 1, 1, fOutDif));
			} else {
				// empty block so...
				if (fOut) {
					if (nHeaderMode == SCRUB_REMOVE_HEADERS) {
						// change back to 1.0 version.
						// As it was pretty trivial for N to check the SHA tables then it seems
						// pointless including them at the cost of 1k per sector
						bStatus *= (BUFSIZE == fwrite (pBlankSector, 1, BUFSIZE, fOut));
					} else {
						// 1.0a version
						bStatus *= (0x0400 == fwrite (inData, 1, 0x0400, fOut));
						bStatus *= (0x7c00 == fwrite (pBlankSector, 1, 0x7c00, fOut));
					}
				}
				if (fOutDif) {
					bStatus *= (0x0001 == fwrite (pBlankSector, 1, 1, fOutDif));
					bStatus *= (BUFSIZE == fwrite (inData, 1, BUFSIZE, fOutDif));
				}
			}
		}

		if (!bStatus) {
			// error in read or write - don't care where, just exit with error
			if (fOutDif)
				fclose (fOutDif);
			if (fOut)
				fclose (fOut);
			fclose (fIn);
			return false;
		}
	}

	if (fOutDif)
		fclose (fOutDif);
	if (fOut)
		fclose (fOut);
	fclose (fIn);
	
	return true;
}


#define HEADER_BUFSIZE 0x440

/* First function to be called */
struct image_file *CWIIDisc::image_init (char *filename, bool force_wii) {
	printf ("--- %s\n", __FUNCTION__);
	FILE *fp;
	struct image_file *image;
	struct part_header *header;
	u_int8_t buffer[HEADER_BUFSIZE];
	
	if ((fp = fopen (filename, "rb")) == NULL) {
		AfxMessageBox (filename);
		return NULL;
	}
	
	// get the filesize and set the range accordingly for future operations
	if (my_fseek (fp, 0L, SEEK_END) == -1) {
		AfxMessageBox ("Unable to seek to EOF");
		fclose (fp);
		return NULL;
	}
	this -> nImageSize = my_ftell (fp);
	
	if (!(image = (struct image_file *) malloc (sizeof (struct image_file)))) {
		AfxMessageBox ("out of memory");
		fclose (fp);
		return NULL;
	}
	memset (image, 0, sizeof (struct image_file));
	image -> fp = fp;
	
	if (!io_read (buffer, HEADER_BUFSIZE, image, 0)) {
		AfxMessageBox ("reading header");
		fclose (fp);
		free (image);
		return NULL;
	}
		
	if (!(header = (struct part_header *) (malloc (sizeof (struct part_header))))) {
		AfxMessageBox ("out of memory");
		fclose (fp);
		free (image);
		return NULL;
	}
	
	// OK, parse the image header
	image_parse_header (header, buffer, false);		// FIXME Always false...
	if (!header -> is_gc && !header -> is_wii) {
		AfxMessageBox ("unknown type for file: %s", filename);
		fclose (fp);
		free (header);
		free (image);
		return NULL;
	}
	
	if (!header -> has_magic) {
		AddToLog ("image has an invalid magic");
	}
	
	image -> is_wii = header -> is_wii;
	if (header -> is_wii) {
		/* Init key */
		if (!CheckAndLoadKey (image)) {
			fclose (fp);
			free (image);
			return NULL;
		}
	}
	// Runtime crash fixed :)
	// Identified by Juster over on GBATemp.net
	// the free was occuring before in the wrong location
	// As a free was being carried out and the next line was checking
	// a value it was pointing to
	free (header);
	printf ("--- %s return\n", __FUNCTION__);
	return image;
};


u_int8_t CWIIDisc::image_parse_header (struct part_header *header, u_int8_t *buffer, bool forceWii) {
	printf ("--- %s\n", __FUNCTION__);
	memset (header, 0, sizeof (struct part_header));
	
	header->console = buffer[0];
	
	// account for the Team Twizlers gotcha
	if (!forceWii) {
		header->is_gc = (header->console == 'G') ||
		(header->console == 'D') ||
		(header->console == 'P') || (header->console == 'U');
		header->is_wii = (header->console == 'R') ||
		(header->console == '_') ||
		(header->console == 'H') ||
		(header->console == '0') || (header->console == '4');
	} else {
		header->is_gc = false;
		header->is_wii = true;
	}
	
	header->gamecode[0] = buffer[1];
	header->gamecode[1] = buffer[2];
	header->region = buffer[3];
	header->publisher[0] = buffer[4];
	header->publisher[1] = buffer[5];
	
	header->has_magic = be32 (&buffer[0x18]) == 0x5d1c9ea3;
	strncpy (header->name, (char *) (&buffer[0x20]), 0x60);
	
	header->dol_offset = be32 (&buffer[0x420]);
	
	header->fst_offset = be32 (&buffer[0x424]);
	header->fst_size = be32 (&buffer[0x428]);
	
	if (header->is_wii) {
		header->dol_offset *= 4;
		header->fst_offset *= 4;
		header->fst_size *= 4;
	}
	printf ("--- %s return\n", __FUNCTION__);
	return header->is_gc || header->is_wii;
}


/* Second function to be called */
int CWIIDisc::image_parse (struct image_file *image) {
	printf ("--- %s\n", __FUNCTION__);
	u_int8_t buffer[HEADER_BUFSIZE];
	u_int8_t *fst;
	u_int8_t i;
	u_int8_t j, valid, nvp;
	
	u_int32_t nfiles;
	
// 	string csText;
	
	if (image->is_wii) {
		AddToLog ("wii image detected");
	
		get_partitions (image);
	} else {
		AddToLog ("gamecube image detected");
		
		image->parts = (struct partition *)
		malloc (sizeof (struct partition));
		memset (&image->parts[0], 0, sizeof (struct partition));
		image->PartitionCount = 1;
		image->ChannelCount = 0;
		image->part_tbl_offset = 0;
		image->chan_tbl_offset = 0;
		image->parts[0].type = PART_DATA;
	}
	
	fstat (fileno (image->fp), &image->st);
	
	nvp = 0;
	for (i = 0; i < image->PartitionCount + 1; ++i) {
		AddToLog ("------------------------------------------------------------------------------");
		
		AddToLog ("Partition:", i);

		switch (image->parts[i].type) {
			case PART_DATA:
				AfxMessageBox ("Partition:%d - DATA", i);
				break;
			case PART_UPDATE:
				AfxMessageBox ("Partition:%d - UPDATE", i);
				break;
			case PART_INSTALLER:
				AfxMessageBox ("Partition:%d - INSTALLER", i);
				break;
			case PART_VC:
				AfxMessageBox ("Partition:%d - VC GAME [%s]", i, image->parts[i].chan_id);
				break;
			default:
				if (0 != i) {
					AfxMessageBox ("Partition:%d - UNKNOWN", i);
				} else {
					AfxMessageBox ("Partition:0 - PART INFO");
				}
				break;
		}

		if (!io_read_part (buffer, HEADER_BUFSIZE, image, i, 0)) {
			AfxMessageBox ("partition header");
			return 1;
		}
		
		valid = 1;
		for (j = 0; j < 6; ++j) {
			if (!isprint (buffer[j])) {
				valid = 0;
				break;
			}
		}
		
		if (!valid) {
			AddToLog ("invalid header for partition:", i);
			continue;
		}
		nvp++;
		
		/* Parse partition header */
		cout << "reparse header" << endl;
		image_parse_header (&image->parts[i].header, buffer, false);		// FIXME Always false
		cout << "reparse header end" << endl;
				
		if (PART_UNKNOWN != image->parts[i].type) {
			AddToLog ("\\partition.bin", image->parts[i].offset, image->parts[i].data_offset);
			MarkAsUsed (image->parts[i].offset, image->parts[i].data_offset);
			
			// add on the boot.bin
			AddToLog ("\\boot.bin",
					  image->parts[i].offset +
					  image->parts[i].data_offset,
					  (u_int64_t) HEADER_BUFSIZE);
			MarkAsUsedDC (image->parts[i].offset +
						  image->parts[i].data_offset, 0,
						  (u_int64_t) HEADER_BUFSIZE,
						  image->parts[i].is_encrypted);
						
			// add on the bi2.bin
			AddToLog ("\\bi2.bin",
					  image->parts[i].offset +
					  image->parts[i].data_offset + HEADER_BUFSIZE,
					  (u_int64_t) 0x2000);
			MarkAsUsedDC (image->parts[i].offset +
						  image->parts[i].data_offset, HEADER_BUFSIZE,
						  (u_int64_t) 0x2000,
						  image->parts[i].is_encrypted);
		}
		io_read_part (buffer, 8, image, i, 0x2440 + 0x14);
		image->parts[i].appldr_size =
		be32 (buffer) + be32 (&buffer[4]);
		if (image->parts[i].appldr_size > 0)
			image->parts[i].appldr_size += 32;
		
		if (image->parts[i].appldr_size > 0) {
			AddToLog ("\\apploader.img",
					  image->parts[i].offset +
					  image->parts[i].data_offset + 0x2440,
					  image->parts[i].appldr_size);
			MarkAsUsedDC (image->parts[i].offset +
						  image->parts[i].data_offset, 0x2440,
						  image->parts[i].appldr_size,
						  image->parts[i].is_encrypted);
		} else {
			AddToLog ("apploader.img not present");
		}
		
		if (image->parts[i].header.dol_offset > 0) {
			io_read_part (buffer, 0x100, image, i,
						  image->parts[i].header.dol_offset);
			image->parts[i].header.dol_size =
			get_dol_size (buffer);
			
			// now check for error condition with bad main.dol
			if (image->parts[i].header.dol_size >=
			    image->parts[i].data_size) {
				// almost certainly an error as it's bigger than the partition
				image->parts[i].header.dol_size = 0;
			}
			MarkAsUsedDC (image->parts[i].offset +
						  image->parts[i].data_offset,
						  image->parts[i].header.dol_offset,
						  image->parts[i].header.dol_size,
						  image->parts[i].is_encrypted);
			
			AddToLog ("\\main.dol ",
					  image->parts[i].offset +
					  image->parts[i].data_offset +
					  image->parts[i].header.dol_offset,
					  image->parts[i].header.dol_size);
		} else {	
			AddToLog ("partition has no main.dol");
		}
		
		if (image->parts[i].is_encrypted) {
			// Now add the TMD.BIN and cert.bin files - as these are part of partition.bin
			// we don't need to mark them as used - we do put them undr partition.bin in the
			// tree though
			
			AddToLog ("\\tmd.bin",
					  image->parts[i].offset +
					  image->parts[i].tmd_offset,
					  image->parts[i].tmd_size);
			AddToLog ("\\cert.bin",
					  image->parts[i].offset +
					  image->parts[i].cert_offset,
					  image->parts[i].cert_size);
			
			// add on the H3
			AddToLog ("\\h3.bin",
					  image->parts[i].offset +
					  image->parts[i].h3_offset,
					  (u_int64_t) 0x18000);
			MarkAsUsedDC (image->parts[i].offset,
						  image->parts[i].h3_offset,
						  (u_int64_t) 0x18000, false);
		}
		
		
		if (image->parts[i].header.fst_offset > 0 &&
		    image->parts[i].header.fst_size > 0) {
			
			AddToLog ("\\fst.bin ",
					  image->parts[i].offset +
					  image->parts[i].data_offset +
					  image->parts[i].header.fst_offset,
					  image->parts[i].header.fst_size);
			
			MarkAsUsedDC (image->parts[i].offset +
						  image->parts[i].data_offset,
						  image->parts[i].header.fst_offset,
						  image->parts[i].header.fst_size,
						  image->parts[i].is_encrypted);
			fst = (u_int8_t *) (malloc ((u_int32_t)
										(image->parts[i].header.
										 fst_size)));
			if (io_read_part
			    (fst,
			     (u_int32_t) (image->parts[i].header.fst_size),
			     image, i,
			     image->parts[i].header.fst_offset) !=
			    image->parts[i].header.fst_size) {
				AfxMessageBox ("fst.bin");
				free (fst);
				return 1;
			}
			
			nfiles = be32 (fst + 8);
			
			if (12 * nfiles > image->parts[i].header.fst_size) {
				AddToLog ("invalid fst for partition", i);
			} else {
				m_csText = "\\";
				
				parse_fst (fst, (char *) (fst + 12 * nfiles), 0, NULL, image, i);
			}
			
			free (fst);
		} else {
			AddToLog ("partition has no fst");
		}
	}
	
	if (!nvp) {
		AddToLog ("no valid partition were found, exiting");
		return 1;
	}
	printf ("--- %s return\n", __FUNCTION__);
	return 0;
}


u_int8_t CWIIDisc::get_partitions (struct image_file * image) {
	u_int8_t buffer[16];
	u_int8_t i, total_parts;
	u_int8_t title_key[16];
	u_int8_t iv[16];
	u_int8_t partition_key[16];
	
	// clear out the old memory allocated
	if (image -> parts) {
		free (image -> parts);
		image -> parts = NULL;
	}
	
	/* Read the partition table (?) */
	io_read (buffer, 16, image, 0x40000);
	
	// store the values for later bit twiddling
	image -> PartitionCount = be32 (&buffer[0]);	// number of partitions is out by one
	image -> ChannelCount = be32 (&buffer[8]);

	AddToLog ("number of partitions:", image -> PartitionCount + 1);
	AddToLog ("number of channels:", image -> ChannelCount);
	
	image -> part_tbl_offset = (u_int64_t) (be32 (&buffer[4])) * ((u_int64_t) (4));
	image -> chan_tbl_offset = (u_int64_t) (be32 (&buffer[12])) * ((u_int64_t) (4));
	AddToLog ("partition table offset: ", image -> part_tbl_offset);
	AddToLog ("channel table offset: ", image -> chan_tbl_offset);
	
	total_parts = image -> PartitionCount + image -> ChannelCount + 1;
	image -> parts = (struct partition *) malloc (total_parts * sizeof (struct partition));
	memset (image -> parts, 0, total_parts * sizeof (struct partition));
	
	for (i = 1; i < total_parts; i++) {
		AddToLog ("--------------------------------------------------------------------------");
		AddToLog ("Partition:", i);
		
		if (i < image -> PartitionCount + 1) {
			io_read (buffer, 8, image, image -> part_tbl_offset + (i - 1) * 8);
			switch (be32 (&buffer[4])) {
				case 0:
					image -> parts[i].type = PART_DATA;
					break;
				case 1:
					image -> parts[i].type = PART_UPDATE;
					break;
				case 2:
					image -> parts[i].type = PART_INSTALLER;
					break;
				default:
					AddToLog ("WARNING: Unknown partition type");
					image -> parts[i].type = PART_UNKNOWN;
					break;
			}
		} else {
			AddToLog ("Virtual console");
			
			// error in WiiFuse as it 'assumes' there are only two partitions before VC games
			// changed to a generic version
			io_read (buffer, 8, image, image -> chan_tbl_offset + (i - image -> PartitionCount - 1) * 8);
			
			image -> parts[i].type = PART_VC;
			image -> parts[i].chan_id[0] = buffer[4];
			image -> parts[i].chan_id[1] = buffer[5];
			image -> parts[i].chan_id[2] = buffer[6];
			image -> parts[i].chan_id[3] = buffer[7];
		}
		
		image -> parts[i].offset = (u_int64_t) (be32 (buffer)) * ((u_int64_t) (4));
		
		AddToLog ("partition offset: ", image -> parts[i].offset);
		
		// mark the block as used
		MarkAsUsed (image -> parts[i].offset, 0x8000);
		
		io_read (buffer, 8, image, image -> parts[i].offset + 0x2b8);
		image -> parts[i].data_offset = (u_int64_t) (be32 (&buffer[0])) * 4;
		image -> parts[i].data_size = (u_int64_t) (be32 (&buffer[4])) * 4;
		
		// now get the H3 offset
		io_read (buffer, 4, image, image -> parts[i].offset + 0x2b4);
		image -> parts[i].h3_offset = (u_int64_t) (be32 (&buffer[0])) * 4;
		
		AddToLog ("partition data offset:", image -> parts[i].data_offset);
		AddToLog ("partition data size:", image -> parts[i].data_size);
		AddToLog ("H3 offset:", image -> parts[i].h3_offset);
		
		tmd_load (image, i);
		if (image -> parts[i].tmd == NULL) {
			AddToLog ("partition has no valid tmd");
		} else {
			sprintf (image->parts[i].title_id_str, "%016llx", image->parts[i].tmd->title_id);
			
			image -> parts[i].is_encrypted = 1;
			image -> parts[i].cached_block = 0xffffffff;
			
			memset (title_key, 0, 16);
			memset (iv, 0, 16);
			
			io_read (title_key, 16, image, image->parts[i].offset + 0x1bf);
			io_read (iv, 8, image, image->parts[i].offset + 0x1dc);
			
			AES_cbc_encrypt (title_key, partition_key, 16, &image->key, iv, AES_DECRYPT);
			
			memcpy (image->parts[i].title_key, partition_key, 16);
			
			AES_set_decrypt_key (partition_key, 128, &image->parts[i].key);
			
			sprintf (image->parts[i].key_c, "0x"
					"%02x%02x%02x%02x%02x%02x%02x%02x"
					"%02x%02x%02x%02x%02x%02x%02x%02x",
				partition_key[0], partition_key[1],
				partition_key[2], partition_key[3],
				partition_key[4], partition_key[5],
				partition_key[6], partition_key[7],
				partition_key[8], partition_key[9],
				partition_key[10], partition_key[11],
				partition_key[12], partition_key[13],
				partition_key[14], partition_key[15]);
		}
	}
	
	return image -> PartitionCount == 0;
}


int CWIIDisc::io_read (u_int8_t *ptr, size_t size, struct image_file *image, u_int64_t offset) {
	size_t bytes;

	if (my_fseek (image -> fp, offset, SEEK_SET) != 0) {
		AfxMessageBox ("fseek failed to %llu\n", offset);
		bytes = -1;
	} else {
		MarkAsUsed (offset, size);
		
		bytes = fread (ptr, 1, size, image->fp);
		if (bytes != size)
			AfxMessageBox ("io_read");
	}
	
	return (bytes);
}



bool CWIIDisc::CheckAndLoadKey (struct image_file *image) {
	FILE *fp_key;
	static u_int8_t LoadedKey[KEY_LENGTH];
	bool ret;
	
	if (!(fp_key = fopen (KEYFILE, "rb"))) {
		AfxMessageBox ("Unable to open key.bin");
		ret = false;
	} else if (fread (LoadedKey, 1, KEY_LENGTH, fp_key) != KEY_LENGTH) {
		AfxMessageBox ("key.bin not 16 bytes in size");
		ret = false;
	} else {
		// now check to see if it's the right key
		// as we don't want to embed the key value in here then lets cheat a little ;)
		// by checking the xor'd difference values
		if ((0x0F != (LoadedKey[0] ^ LoadedKey[1])) ||
			(0xCE != (LoadedKey[1] ^ LoadedKey[2])) ||
			(0x08 != (LoadedKey[2] ^ LoadedKey[3])) ||
			(0x7C != (LoadedKey[3] ^ LoadedKey[4])) ||
			(0xDB != (LoadedKey[4] ^ LoadedKey[5])) ||
			(0x16 != (LoadedKey[5] ^ LoadedKey[6])) ||
			(0x77 != (LoadedKey[6] ^ LoadedKey[7])) ||
			(0xAC != (LoadedKey[7] ^ LoadedKey[8])) ||
			(0x91 != (LoadedKey[8] ^ LoadedKey[9])) ||
			(0x1C != (LoadedKey[9] ^ LoadedKey[10])) ||
			(0x80 != (LoadedKey[10] ^ LoadedKey[11])) ||
			(0x36 != (LoadedKey[11] ^ LoadedKey[12])) ||
			(0xF2 != (LoadedKey[12] ^ LoadedKey[13])) ||
			(0x2B != (LoadedKey[13] ^ LoadedKey[14])) ||
			(0x5D != (LoadedKey[14] ^ LoadedKey[15]))) {
			/* No check for the last byte?! :( */
			AddToLog ("WARNING: Key does not look correct");		// Try using it anyway		
		}

		AES_set_decrypt_key (LoadedKey, 128, &image->key);
		ret = true;
	}

	if (fp_key)
		fclose (fp_key);

	return ret;
}


//////////////////////////////////////////////////////////////////////
// Recreate the original data from a DIF file and a compress file   //
//////////////////////////////////////////////////////////////////////
bool CWIIDisc::RecreateOriginalFile (char *scrubbed, char *diff, char *output) {
	FILE *fInCompress = NULL;
	FILE *fInDif = NULL;
	FILE *fOut = NULL;
	u_int64_t nFileSize;
	
	unsigned long nRead = 0L;
	int i = 0;
	
	unsigned char inData[0x8000];
	unsigned char inDifData[0x8000];
	
	fInCompress = fopen (scrubbed, "rb");
	fInDif = fopen (diff, "rb");
	if ((NULL == fInCompress) || (NULL == fInDif)) {
		// error opning one of the files
		if (NULL != fInCompress) {
			fclose (fInCompress);
		}
		if (NULL != fInDif) {
			fclose (fInDif);
		}
		return false;
	}
	// try and create the output file
	if (NULL == (fOut = fopen (output, "wb"))) {
		// error - close open and return
		fclose (fInDif);
		fclose (fInCompress);
		return false;
	}
	// get the input filesize
	nFileSize = my_fseek (fInCompress, 0L, SEEK_END);
	my_fseek (fInCompress, 0L, SEEK_SET);

	i = 0;
	while (!feof (fInCompress)) {
		// read in a block of data
		nRead = fread (inData, 1, 0x8000, fInCompress);
		if (0x8000 == nRead) {
			
			// read in a byte from the DIFF file
			fread (inDifData, 1, 1, fInDif);
			// if DIFF file flagged as 'change data' then
			if (0xFF == inDifData[0]) {
				// read in block of data
				fread (inDifData, 1, 0x8000, fInDif);
				// ouput it
				fwrite (inDifData, 1, 0x8000, fOut);
			} else {
				// just need to send the input from the compressed to the output
				fwrite (inData, 1, 0x8000, fOut);
			}
		} else {
			// we've read to the end
		}
		
		i++;		
	}
	
	fclose (fInDif);
	fclose (fInCompress);
	fclose (fOut);
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////



u_int32_t CWIIDisc::parse_fst (u_int8_t * fst, const char *names, u_int32_t i,
							   struct tree * tree, struct image_file * image,
							   u_int32_t part)
{
	u_int64_t offset;
	u_int32_t size;
	const char *name;
	u_int32_t j;
// 	string m_csText;		// This is a sort of global varm should be removed. suk
	
	name = names + (be32 (fst + 12 * i) & 0x00ffffff);
	size = be32 (fst + 12 * i + 8);
	
	if (i == 0) {
		// directory so need to go through the directory entries
		for (j = 1; j < size;) {
			j = parse_fst (fst, names, j, tree, image, part);
		}
		return size;
	}
	
	if (fst[12 * i]) {
		m_csText += name;
		m_csText += "\\";
		AddToLog (m_csText);
		//suk                           pParent = m_pParent->AddItemToTree(name, pParent);
		
		for (j = i + 1; j < size;)
			j = parse_fst (fst, names, j, NULL, image, part);
		
		// now remove the directory name we just added
		//		m_csText = m_csText.Left (m_csText.GetLength () - strlen (name) - 1);
		m_csText = m_csText.substr (0, m_csText.length () - strlen (name) - 1);
		return size;
	} else {
		offset = be32 (fst + 12 * i + 4);
		if (image->parts[part].header.is_wii)
			offset *= 4;
		
		//		string csTemp;
		//		csTemp.Format ("%s [0x%lX] [0x%I64X] [0x%lX] [%d]", name,
		//			       part, offset, size, i);
		
		//suk                           m_pParent->AddItemToTree(csTemp, pParent);
		
		
		std::string csTemp = m_csText + name;
		//		csTemp.Format ("%s%s", m_csText, name);
		AddToLog (csTemp, image->parts[part].offset + offset, size);
		
		
		MarkAsUsedDC (image->parts[part].offset +
					  image->parts[part].data_offset, offset, size,
					  image->parts[part].is_encrypted);
		
		image->nfiles++;
		image->nbytes += size;
		
		return i + 1;
	}
}
u_int32_t CWIIDisc::parse_fst_and_save (u_int8_t * fst, const char *names,
										u_int32_t i, struct image_file * image,
										u_int32_t part)
{
	u_int64_t offset;
	u_int32_t size;
	const char *name;
	u_int32_t j;
	string csTemp;
// 	string m_csText;
	
	name = names + (be32 (fst + 12 * i) & 0x00ffffff);
	size = be32 (fst + 12 * i + 8);
	
	if (i == 0) {
		// directory so need to go through the directory entries
		for (j = 1; j < size;) {
			j = parse_fst_and_save (fst, names, j, image, part);
		}
		if (j != 0xFFFFFFFF) {
			return size;
		} else {
			return 0xFFFFFFFF;
		}
	}
	
	if (fst[12 * i]) {
		// directory so...
		// create a directory and change to it
		//suk	mkdir?!
		mkdir (name, 0770);
		chdir (name);
		
		for (j = i + 1; j < size;) {
			j = parse_fst_and_save (fst, names, j, image, part);
		}
		
		// now remove the directory name we just added
		//m_csText = m_csText.Left (m_csText.GetLength () - strlen (name) - 1);
		m_csText = m_csText.substr (0, m_csText.length () - strlen (name) - 1);
		chdir ("..");
		if (j != 0xFFFFFFFF) {
			return size;
		} else {
			return 0xFFFFFFFF;
		}
	} else {
		// it's a file so...
		// create a filename and then save it out
		offset = be32 (fst + 12 * i + 4);
		if (image->parts[part].header.is_wii) {
			offset *= 4;
		}
		// now save it
		if (true ==
		    SaveDecryptedFile (name, image, part, offset, size)) {
			return i + 1;
		} else {
			// Error writing file
			return 0xFFFFFFFF;
		}
	}
}

//suk This should be merged with Reset
void CWIIDisc::image_deinit (struct image_file *image) {
	int32_t i;
	
	if (!image)
		return;
	
	if (image -> PartitionCount + 1 > 0) {
		for (i = 0; i < image -> PartitionCount + 1; ++i)
			if (image -> parts[i].tmd)
				tmd_free (image -> parts[i].tmd);
		free (image -> parts);
	}
	
	fclose (image -> fp);
	free (image);
}

void CWIIDisc::tmd_load (struct image_file *image, u_int32_t part) {
	struct tmd *tmd;
	u_int32_t tmd_offset, tmd_size;
	enum tmd_sig sig = SIG_UNKNOWN;
	
	u_int64_t off, cert_size, cert_off;
	u_int8_t buffer[64];
	u_int16_t i, s;
	
	off = image->parts[part].offset;
	io_read (buffer, 16, image, off + 0x2a4);
	
	tmd_size = be32 (buffer);
	tmd_offset = be32 (&buffer[4]) * 4;
	cert_size = be32 (&buffer[8]);
	cert_off = be32 (&buffer[12]) * 4;
	
	// TODO: ninty way?
	/*
	 * if (cert_size)
	 * image->parts[part].tmd_size =
	 * cert_off - image->parts[part].tmd_offset + cert_size;
	 */
	
	off += tmd_offset;
	
	io_read (buffer, 4, image, off);
	off += 4;
	
	switch (be32 (buffer)) {
		case 0x00010001:
			sig = SIG_RSA_2048;
			s = 0x100;
			break;
			
		case 0x00010000:
			sig = SIG_RSA_4096;
			s = 0x200;
			break;
	}
	
	if (sig == SIG_UNKNOWN)
		return;
	
	tmd = (struct tmd *) malloc (sizeof (struct tmd));
	memset (tmd, 0, sizeof (struct tmd));
	
	tmd->sig_type = sig;
	
	image->parts[part].tmd = tmd;
	image->parts[part].tmd_offset = tmd_offset;
	image->parts[part].tmd_size = tmd_size;
	
	image->parts[part].cert_offset = cert_off;
	image->parts[part].cert_size = cert_size;
	
	tmd->sig = (unsigned char *) malloc (s);
	io_read (tmd->sig, s, image, off);
	off += s;
	
	off = ROUNDUP64B (off);
	
	io_read ((unsigned char *) &tmd->issuer[0], 0x40, image, off);
	off += 0x40;
	
	io_read (buffer, 26, image, off);
	off += 26;
	
	tmd->version = buffer[0];
	tmd->ca_crl_version = buffer[1];
	tmd->signer_crl_version = buffer[2];
	
	tmd->sys_version = be64 (&buffer[4]);
	tmd->title_id = be64 (&buffer[12]);
	tmd->title_type = be32 (&buffer[20]);
	tmd->group_id = be16 (&buffer[24]);
	
	off += 62;
	
	io_read (buffer, 10, image, off);
	off += 10;
	
	tmd->access_rights = be32 (buffer);
	tmd->title_version = be16 (&buffer[4]);
	tmd->num_contents = be16 (&buffer[6]);
	tmd->boot_index = be16 (&buffer[8]);
	
	off += 2;
	
	if (tmd->num_contents < 1)
		return;
	
	tmd->contents = (struct tmd_content *) malloc (sizeof (struct tmd_content) * tmd->num_contents);
	for (i = 0; i < tmd->num_contents; ++i) {
		io_read (buffer, 0x30, image, off);
		off += 0x30;
		
		tmd->contents[i].cid = be32 (buffer);
		tmd->contents[i].index = be16 (&buffer[4]);
		tmd->contents[i].type = be16 (&buffer[6]);
		tmd->contents[i].size = be64 (&buffer[8]);
		memcpy (tmd->contents[i].hash, &buffer[16], 20);
	}
	
	return;
}

void CWIIDisc::tmd_free (struct tmd *tmd)
{
	if (tmd == NULL)
		return;
	
	if (tmd->sig)
		free (tmd->sig);
	
	if (tmd->contents)
		free (tmd->contents);
	
	free (tmd);
}

int CWIIDisc::decrypt_block (struct image_file *image, u_int32_t part,
							 u_int32_t block)
{
	if (block == image->parts[part].cached_block)
		return 0;
	
	
	if (io_read (image->parts[part].dec_buffer, 0x8000, image,
				 image->parts[part].offset +
				 image->parts[part].data_offset +
				 (u_int64_t) (0x8000) * (u_int64_t) (block))
	    != 0x8000) {
		AfxMessageBox ("decrypt read");
		return -1;
	}
	
	AES_cbc_encrypt (&image->parts[part].dec_buffer[0x400],
					 image->parts[part].cache, 0x7c00,
					 &image->parts[part].key,
					 &image->parts[part].dec_buffer[0x3d0], AES_DECRYPT);
	
	image->parts[part].cached_block = block;
	
	return 0;
}

size_t CWIIDisc::io_read_part (unsigned char *ptr, size_t size,
							   struct image_file * image, u_int32_t part,
							   u_int64_t offset)
{
	u_int32_t block = (u_int32_t) (offset / (u_int64_t) (0x7c00));
	u_int32_t cache_offset = (u_int32_t) (offset % (u_int64_t) (0x7c00));
	u_int32_t cache_size;
	unsigned char *dst = ptr;
	
	if (!image->parts[part].is_encrypted)
		return io_read (ptr, size, image,
						image->parts[part].offset + offset);
	
	while (size) {
		if (decrypt_block (image, part, block))
			return (dst - ptr);
		
		cache_size = size;
		if (cache_size + cache_offset > 0x7c00)
			cache_size = 0x7c00 - cache_offset;
		
		memcpy (dst, image->parts[part].cache + cache_offset,
				cache_size);
		dst += cache_size;
		size -= cache_size;
		cache_offset = 0;
		
		block++;
	}
	
	return dst - ptr;
}



unsigned int CWIIDisc::CountBlocksUsed ()
{
	unsigned int nRetVal = 0;;
	u_int64_t nBlock = 0;
	u_int64_t i = 0;
	unsigned char cLastBlock = 0x01;
	
	AddToLog ("------------------------------------------------------------------------------");
	for (i = 0; i < (nImageSize / (u_int64_t) (0x8000)); i++) {
		nRetVal += pFreeTable[i];
		if (cLastBlock != pFreeTable[i]) {
			// change so show
			if (1 == cLastBlock) {
				AddToLog ("Marked Content",
						  nBlock * (u_int64_t) (0x8000),
						  i * (u_int64_t) (0x8000) - 1,
						  (i - nBlock) * 32);
			} else {
				AddToLog ("Empty Content",
						  nBlock * (u_int64_t) (0x8000),
						  i * (u_int64_t) (0x8000) - 1,
						  (i - nBlock) * 32);
			}
			nBlock = i;
			cLastBlock = pFreeTable[i];
		}
		
	}
	// output the final range
	if (1 == cLastBlock) {
		AddToLog ("Marked Content", nBlock * (u_int64_t) (0x8000),
				  nImageSize - 1, (i - nBlock) * 32);
	} else {
		AddToLog ("Empty Content", nBlock * (u_int64_t) (0x8000),
				  nImageSize - 1, (i - nBlock) * 32);
	}
	AddToLog ("------------------------------------------------------------------------------");
	
	return nRetVal;
}

void CWIIDisc::MarkAsUsed (u_int64_t nOffset, u_int64_t nSize)
{
	u_int64_t nStartValue = nOffset;
	u_int64_t nEndValue = nOffset + nSize;
	while ((nStartValue < nEndValue) &&
	       (nStartValue < ((u_int64_t) (4699979776LL) * (u_int64_t) (2))))
	{
		
		pFreeTable[nStartValue / (u_int64_t) (0x8000)] = 1;
		nStartValue = nStartValue + ((u_int64_t) (0x8000));
	}
	
}
void CWIIDisc::MarkAsUsedDC (u_int64_t nPartOffset, u_int64_t nOffset,
							 u_int64_t nSize, bool bIsEncrypted)
{
	u_int64_t nTempOffset;
	u_int64_t nTempSize;
	
	if (true == bIsEncrypted) {
		// the offset and size relate to the decrypted file so.........
		// we need to change the values to accomodate the 0x400 bytes of crypto data
		
		nTempOffset = nOffset / (u_int64_t) (0x7c00);
		nTempOffset = nTempOffset * ((u_int64_t) (0x8000));
		nTempOffset += nPartOffset;
		
		nTempSize = nSize / (u_int64_t) (0x7c00);
		nTempSize = (nTempSize + 1) * ((u_int64_t) (0x8000));
		
		// add on the offset in the first nblock for the case where data straddles blocks
		
		nTempSize += nOffset % (u_int64_t) (0x7c00);
	} else {
		// unencrypted - we use the actual offsets
		nTempOffset = nPartOffset + nOffset;
		nTempSize = nSize;
	}
	MarkAsUsed (nTempOffset, nTempSize);
	
}


void CWIIDisc::AddToLog (string csText) {
	//	m_pParent->AddToLog (csText);	//suk
	fprintf (stderr, "[LOG] %s\n", csText.c_str ());
}
void CWIIDisc::AddToLog (string csText, u_int64_t nValue) {
	//	m_pParent->AddToLog (csText, nValue);
	fprintf (stderr, "[LOG] %s %llu\n", csText.c_str (), nValue);
}

void CWIIDisc::AddToLog (string csText, u_int64_t nValue1, u_int64_t nValue2) {
	//	string csText1;
	//	csText1.Format ("%s [0x%I64X], [0x%I64X]", csText, nValue1, nValue2);
	//	m_pParent->AddToLog (csText1);
	fprintf (stderr, "[LOG] %s [%llX] [%llX] \n", csText.c_str (), nValue1, nValue2);
}

void CWIIDisc::AddToLog (string csText, u_int64_t nValue1, u_int64_t nValue2, u_int64_t nValue3) {
	//	string csText1;
	//	csText1.Format ("%s [0x%I64X], [0x%I64X] [%I64d K]", csText, nValue1,
	//			nValue2, nValue3);
	//	m_pParent->AddToLog (csText1);
	fprintf (stderr, "[LOG] %s [0x%llX], [0x%llX] [%llX K]\n", csText.c_str (), nValue1, nValue2, nValue3);
}

void CWIIDisc::Reset ()
{
	//set them all to clear first
	memset (pFreeTable, 0, ((4699979776LL / 32768LL) * 2L));
	// then set the header size to used
	MarkAsUsed (0, 0x50000);
#if 0	//suk
	hDisc = NULL;
	for (int i = 0; i < 20; i++) {
		hPartition[i] = NULL;
	}
#endif
	// then clear the decrypt key
	u_int8_t key[16];
	
	memset (key, 0, 16);
	
	AES_KEY nKey;
	
	memset (&nKey, 0, sizeof (AES_KEY));
	AES_set_decrypt_key (key, 128, &nKey);
}

bool CWIIDisc::SaveDecryptedFile (string csDestinationFilename,
								  struct image_file *image, u_int32_t part,
								  u_int64_t nFileOffset, u_int64_t nFileSize,
								  bool bOverrideEncrypt)
{
	FILE *fOut;
	
	u_int32_t block = (u_int32_t) (nFileOffset / (u_int64_t) (0x7c00));
	u_int32_t cache_offset =
	(u_int32_t) (nFileOffset % (u_int64_t) (0x7c00));
	u_int64_t cache_size;
	
	
	unsigned char cBuffer[0x8000];
	
	fOut = fopen (csDestinationFilename.c_str (), "wb");
	
	if ((!image->parts[part].is_encrypted) || (true == bOverrideEncrypt)) {
		if (-1 == my_fseek (image->fp, nFileOffset, SEEK_SET)) {
			AfxMessageBox ("io_seek");
			return -1;
		}
		
		while (nFileSize) {
			cache_size = nFileSize;
			
			if (cache_size > 0x8000)
				cache_size = 0x8000;
			
			
			fread (&cBuffer[0], 1, (u_int32_t) (cache_size), image->fp);
			fwrite (cBuffer, 1, (u_int32_t) (cache_size), fOut);
			
			nFileSize -= cache_size;
			
		}
	} else {
		while (nFileSize) {
			if (decrypt_block (image, part, block)) {
				fclose (fOut);
				return false;
			}
			
			cache_size = nFileSize;
			if (cache_size + cache_offset > 0x7c00)
				cache_size = 0x7c00 - cache_offset;
			
			if (cache_size !=
			    fwrite (image->parts[part].cache + cache_offset,
						1, (u_int32_t) (cache_size), fOut)) {
				AddToLog ("Error writing file");
				fclose (fOut);
				return false;
			}
			
			nFileSize -= cache_size;
			cache_offset = 0;
			
			block++;
		}
	}
	fclose (fOut);
	
	return true;
}

bool CWIIDisc::LoadDecryptedFile (string csDestinationFilename,
								  struct image_file * image, u_int32_t part,
								  u_int64_t nFileOffset, u_int64_t nFileSize,
								  int nFSTReference)
{
	FILE *fIn;
	u_int32_t nImageSize;
	u_int64_t nfImageSize;
	u_int8_t *pBootBin = (unsigned char *) calloc (0x440, 1);
	u_int8_t *pPartData;
	u_int64_t nFreeSpaceStart;
	u_int32_t nExtraData;
	u_int32_t nExtraDataBlocks;
	
	
	// create a 64 cluster buffer for the file
	
	fIn = fopen (csDestinationFilename.c_str (), "rb");
	
	if (NULL == fIn) {
		AddToLog ("Error opening file");
		return false;
	}
	// get the size of the file we are trying to load
	
	nfImageSize = my_fseek (fIn, 0L, SEEK_END);
	nImageSize = (u_int32_t) nfImageSize;
	
	// pointer back to the start
	my_fseek (fIn, 0L, SEEK_SET);
	
	
	// now get the filesize we are trying to load and make sure it is smaller
	// or the same size as the one we are trying to replace if so then a simple replace
	if (nFileSize >= nImageSize) {
		// simple replace
		// now need to change the boot.bin if one if fst.bin or main.dol were changed
		
		if (nFileSize != nfImageSize) {
			// we have a different sized file being put in
			// this is obviously smaller but will require a tweak to one of the file
			// entries
			if (0 < nFSTReference) {
				// normal file so change the FST.BIN
				u_int8_t *pFSTBin = (unsigned char *)
				calloc ((u_int32_t)
						(image->parts[part].header.
						 fst_size), 1);
				
				io_read_part (pFSTBin,
							  (u_int32_t) (image->parts[part].
										   header.fst_size),
							  image, part,
							  image->parts[part].header.
							  fst_offset);
				
				// alter the entry for the passed FST Reference
				nFSTReference = nFSTReference * 0x0c;
				
				// update the length for the file
				Write32 (pFSTBin + nFSTReference + 0x08L,
						 nImageSize);
				
				// write out the FST.BIN
				wii_write_data_file (image, part,
									 image->parts[part].
									 header.fst_offset,
									 (u_int32_t) (image->
												  parts[part].
												  header.
												  fst_size),
									 pFSTBin);
				
				// write it out
				wii_write_data_file (image, part, nFileOffset,
									 nImageSize, NULL, fIn);
				
			} else {
				switch (nFSTReference) {
					case 0:
						// - one of the files that should ALWAYS be the correct size
						AfxMessageBox
						("Error as file sizes do not match and they MUST for boot.bin and bi2.bin");
						fclose (fIn);
						free (pBootBin);
						return false;
						break;
					case -1:
						// FST
						io_read_part (pBootBin, 0x440, image,
									  part, 0);
						
						// update the settings for the FST.BIN entry
						// this has to be rounded to the nearest 4 so.....
						if (0 != (nImageSize % 4)) {
							nImageSize =
							nImageSize + (4 -
										  nImageSize
										  % 4);
						}
						Write32 (pBootBin + 0x428L,
								 (u_int32_t) (nImageSize >>
											  2));
						Write32 (pBootBin + 0x42CL,
								 (u_int32_t) (nImageSize >>
											  2));
						// now write it out
						wii_write_data_file (image, part, 0,
											 0x440, pBootBin);
						
						break;
						case -2:
						// main.dol - don't have to do anything
						break;
						case -3:
						// apploader - don't have to do anything
						break;
						case -4:
						// partition.bin
						AfxMessageBox
						("Error as partition.bin MUST be 0x20000 bytes in size");
						fclose (fIn);
						free (pBootBin);
						return false;
						
						break;
						case -5:
						AfxMessageBox
						("Error as tmd.bin MUST be same size");
						fclose (fIn);
						free (pBootBin);
						return false;
						break;
						case -6:
						AfxMessageBox
						("Error as cert.bin MUST be same size");
						fclose (fIn);
						free (pBootBin);
						return false;
						break;
						case -7:
						AfxMessageBox
						("Error as h3.bin MUST be 0x18000 bytes in size");
						fclose (fIn);
						free (pBootBin);
						return false;
						break;
						default:
						AfxMessageBox
						("Unknown file reference passed");
						fclose (fIn);
						
						free (pBootBin);
						return false;
						break;
				}
				// now write it out
				wii_write_data_file (image, part, nFileOffset,
									 nImageSize, NULL, fIn);
			}
		} else {
			// Equal sized file so need to check for the special cases
			if (0 > nFSTReference) {
				switch (nFSTReference) {
					case -1:
					case -2:
					case -3:
						// simple write as files are the same size
						wii_write_data_file (image, part,
											 nFileOffset,
											 nImageSize, NULL,
											 fIn);
						break;
					case -4:
						// Partition.bin
						// it's a direct write
						pPartData =
						(u_int8_t *) calloc (1,
											 (unsigned
											  int)
											 nFileSize);
						
						fread (pPartData, 1,
							   (unsigned int) nFileSize, fIn);
						DiscWriteDirect (image,
										 image->parts[part].
										 offset, pPartData,
										 (unsigned int)
										 nFileSize);
						free (pPartData);
						break;
					case -5:
						// tmd.bin;
					case -6:
						// cert.bin
					case -7:
						// h3.bin
						
						// same for all 3
						pPartData =
						(u_int8_t *) calloc (1,
											 (unsigned
											  int)
											 nFileSize);
						
						fread (pPartData, 1,
							   (unsigned int) nFileSize, fIn);
						DiscWriteDirect (image,
										 image->parts[part].
										 offset + nFileOffset,
										 pPartData,
										 (unsigned int)
										 nFileSize);
						free (pPartData);
						
						break;
					default:
						AddToLog ("Unknown file reference passed");
						break;
				}
			} else {
				// simple write as files are the same size
				wii_write_data_file (image, part, nFileOffset,
									 nImageSize, NULL, fIn);
			}
		}
		
	} else {
		// Alternatively just have to update the FST or boot.bin depending on the file we want to change
		// this will depend on whether the passed index is
		// -ve = Partition data,
		// 0 = given by boot.bin,
		// +ve = normal file
		
		// need to find some free space in the partition first
		nFreeSpaceStart =
		FindRequiredFreeSpaceInPartition (image, part,
										  nImageSize);
		
		if (0 == nFreeSpaceStart) {
			// no free space - so cant do it
			AfxMessageBox
			("Unable to find free space to add the file :(");
			fclose (fIn);
			
			free (pBootBin);
			return false;
		}
		// depending on the passed offset we then need to modify either the
		// fst.bin or the boot.bin
		if (0 < nFSTReference) {
			// normal one - so read out the fst and change the values for the relevant pointer
			// before writing it out
			u_int8_t *pFSTBin = (unsigned char *)
			calloc ((u_int32_t)
					(image->parts[part].header.fst_size),
					1);
			
			io_read_part (pFSTBin,
						  (u_int32_t) (image->parts[part].header.
									   fst_size), image, part,
						  image->parts[part].header.fst_offset);
			
			// alter the entry for the passed FST Reference
			nFSTReference = nFSTReference * 0x0c;
			
			// update the offset for this file
			Write32 (pFSTBin + nFSTReference + 0x04L,
					 u_int32_t (nFreeSpaceStart >> 2));
			// update the length for the file
			Write32 (pFSTBin + nFSTReference + 0x08L, nImageSize);
			
			// write out the FST.BIN
			wii_write_data_file (image, part,
								 image->parts[part].header.
								 fst_offset,
								 (u_int32_t) (image->parts[part].
											  header.fst_size),
								 pFSTBin);
			
			// now write data file out
			wii_write_data_file (image, part, nFreeSpaceStart,
								 nImageSize, NULL, fIn);
			
		} else {
			
			switch (nFSTReference) {
				case -1:	// FST.BIN
					// change the boot.bin file too and write that out
					io_read_part (pBootBin, 0x440, image, part,
								  0);
					
					// update the settings for the FST.BIN entry
					// this has to be rounded to the nearest 4 so.....
					if (0 != (nImageSize % 4)) {
						nImageSize =
						nImageSize + (4 -
									  nImageSize % 4);
					}
					// update the settings for the FST.BIN entry
					Write32 (pBootBin + 0x424L,
							 u_int32_t (nFreeSpaceStart >> 2));
					Write32 (pBootBin + 0x428L,
							 (u_int32_t) (nImageSize >> 2));
					Write32 (pBootBin + 0x42CL,
							 (u_int32_t) (nImageSize >> 2));
					
					// now write it out
					wii_write_data_file (image, part, 0, 0x440,
										 pBootBin);
					
					// now write it out
					wii_write_data_file (image, part,
										 nFreeSpaceStart,
										 nImageSize, NULL, fIn);
					
					
					break;
					case -2:	// Main.DOL
					// change the boot.bin file too and write that out
					io_read_part (pBootBin, 0x440, image, part,
								  0);
					
					// update the settings for the main.dol entry
					Write32 (pBootBin + 0x420L,
							 u_int32_t (nFreeSpaceStart >> 2));
					
					// now write it out
					wii_write_data_file (image, part, 0, 0x440,
										 pBootBin);
					
					// now write main.dol out
					wii_write_data_file (image, part,
										 nFreeSpaceStart,
										 nImageSize, NULL, fIn);
					
					
					break;
					case -3:	// Apploader.img - now this is fun! as we have to
					// move the main.dol and potentially fst.bin too  too otherwise they would be overwritten
					// also what happens if the data for those two has already been moved
					// aaaargh
					
					// check to see what we have to move
					// by calculating the amount of extra data we are trying to stuff in
					nExtraData =
					(u_int32_t) (nImageSize -
								 image->parts[part].
								 appldr_size);
					
					nExtraDataBlocks =
					1 +
					((nImageSize -
					  (u_int32_t) (image->parts[part].
								   appldr_size)) /
					 0x7c00);
					
					// check to see if we have that much free at the end of the area
					// or do we need to try and overwrite
					if (true ==
						CheckForFreeSpace (image, part,
										   image->parts[part].
										   appldr_size + 0x2440,
										   nExtraDataBlocks)) {
						// we have enough space after the current apploader - already moved the main.dol?
						// so just need to write it out.
						wii_write_data_file (image, part,
											 0x2440,
											 nImageSize, NULL,
											 fIn);
						
					} else {
						// check if we can get by with moving the main.dol
						if (nExtraData >
							image->parts[part].header.
							dol_size) {
							// don't really want to be playing around here as we potentially can get
							// overwrites of all sorts of data
							AfxMessageBox
							("Cannot guarantee writing data correctly\nI blame nargels");
							AddToLog ("Cannot guarantee a good write of apploader");
							fclose (fIn);
							
							free (pBootBin);
							return false;
						} else {
							// "just" need to move main.dol
							u_int8_t *pMainDol =
							(u_int8_t *)
							calloc ((u_int32_t)
									(image->
									 parts[part].
									 header.
									 dol_size),
									1);
							
							io_read_part (pMainDol,
										  (u_int32_t)
										  (image->
										   parts[part].
										   header.
										   dol_size),
										  image, part,
										  image->
										  parts[part].
										  header.
										  dol_offset);
							
							// try and get some free space for it
							nFreeSpaceStart =
							FindRequiredFreeSpaceInPartition
							(image, part,
							 (u_int32_t) (image->
										  parts
										  [part].
										  header.
										  dol_size));
							
							// now unmark the original dol area
							MarkAsUnused (image->
										  parts[part].
										  offset +
										  image->
										  parts[part].
										  data_offset +
										  (((image->
											 parts[part].
											 header.
											 dol_offset) /
											0x7c00) *
										   0x8000),
										  image->
										  parts[part].
										  header.
										  dol_size);
							
							if ((0 != nFreeSpaceStart) &&
								(true ==
								 CheckForFreeSpace (image,
													part,
													image->
													parts
													[part].
													appldr_size
													+
													0x2440,
													nExtraDataBlocks)))
							{
								// got space so write it out
								wii_write_data_file
								(image, part,
								 nFreeSpaceStart,
								 (u_int32_t)
								 (image->
								  parts[part].
								  header.
								  dol_size),
								 pMainDol);
								
								// now do the boot.bin file too
								io_read_part
								(pBootBin,
								 0x440, image,
								 part, 0);
								
								// update the settings for the boot.BIN entry
								Write32 (pBootBin +
										 0x420L,
										 u_int32_t
										 (nFreeSpaceStart
										  >> 2));
								
								// now write it out
								wii_write_data_file
								(image, part,
								 0, 0x440,
								 pBootBin);
								
								// now write out the apploader - we don't need to change any other data
								// as the size is inside the apploader
								wii_write_data_file
								(image, part,
								 0x2440,
								 nImageSize,
								 NULL, fIn);
								
							} else {
								// cannot do it :(
								AfxMessageBox
								("Unable to move the main.dol and find enough space for the apploader.");
								AddToLog ("Unable to add larger apploader");
								free (pMainDol);
								free (pBootBin);
								fclose (fIn);
								
								return false;
							}
							
							
						}
					}
					break;
					default:
					// Unable to do these as they are set sizes and lengths
					// boot.bin and bi2.bin
					// partition.bin
					AfxMessageBox
					("Unable to change that file as it is a set size\nin the disc image");
					AddToLog ("Unable to change set size file");
					free (pBootBin);
					fclose (fIn);
					return false;
					break;
			}
			
		}
	}
	
	
	// free the memory we allocated
	free (pBootBin);
	fclose (fIn);
	
	return true;
}



////////////////////////////////////////////////////////////////////////
// here we find the free space, modify the fst.bin and do some other  //
// bit buggering to add 5 files to the image                          //
// As these are filled with blanks then they compress well even after //
// encrypting                                                         //
////////////////////////////////////////////////////////////////////////

bool CWIIDisc::TruchaScrub (image_file * image, unsigned int nPartition)
{
	unsigned char *pFST = NULL;
	unsigned char *pFSTCopy = NULL;
	unsigned char *pBootBin = NULL;
	unsigned char *pFSTDummy = NULL;
	FILE *fOut;
	
	int z = 0;
	
	u_int64_t nFSTOldSize;
	u_int64_t nFSTNewSize;
	u_int32_t nFSTNewDiscSize;
	
	//// find the free space size
	u_int64_t nOffset;
	u_int64_t nSize;
	u_int64_t nSizeForFiles;
	
	FindFreeSpaceInPartition (image->parts[nPartition].offset, &nOffset,
							  &nSize);
	
	// now need to subtract the partition info from the start
	nOffset =
	nOffset - (image->parts[nPartition].offset +
			   image->parts[nPartition].data_offset);
	
	nFSTOldSize = (image->parts[nPartition].header.fst_size);
	nFSTNewSize = (image->parts[nPartition].header.fst_size) + (0x0c + 0x0c) * 5;	//5 extra entries plus extra strings
	nFSTNewDiscSize = u_int32_t (nFSTNewSize >> 2);
	
	// now follows an example bit of code on how to put whatever you like
	// onto a TRUCHA disc by a three stage process
	
	/////////////////////////////////////////////////////////////////////////
	// Step 1 = prepare a modified boot.bin that points to where the new
	// fst.bin will live
	/////////////////////////////////////////////////////////////////////////
	
	// get the boot.bin file out and then save it
	// with modified values for a 'fixed' FST
	pBootBin = (unsigned char *) malloc (0x440);
	io_read_part (pBootBin, 0x440, image, nPartition, 0);
	
	Write32 (pBootBin + 0x424L, u_int32_t (nOffset >> 2));
	Write32 (pBootBin + 0x428L, nFSTNewDiscSize);
	Write32 (pBootBin + 0x42CL, nFSTNewDiscSize);
	// also update the offset to point to the new FST location after the second write
	// save the boot.bin
	fOut = fopen ("ReplacementBoot.bin", "wb");
	fwrite (pBootBin, 1, 0x440, fOut);
	fclose (fOut);
	/////////////////////////////////////////////////////////////////////////
	// Step 2 = create a modified fst.bin that points to where we are going
	// to store the fst.bin we really want to use in the new disc image
	/////////////////////////////////////////////////////////////////////////
	
	// now create the fst.bin we use to chuck our data into the correct place
	// only two entries plus a file name
	pFSTDummy = (unsigned char *) malloc ((0x0C * 2) + 0x10);
	
	// create the dummy FST
	memset (pFSTDummy, 0, 0x28);
	// standard start record
	Write32 (pFSTDummy, 0x01000000);
	Write32 (pFSTDummy + 0x04L, 0x00000000);
	Write32 (pFSTDummy + 0x08L, 0x00000002);
	
	// now the data entry
	Write32 (pFSTDummy + 0x0C, 0x00000000);	// file , first string table entry
	Write32 (pFSTDummy + 0x10, u_int32_t (nOffset >> 2));	// start location
	Write32 (pFSTDummy + 0x14, (u_int32_t) (nFSTNewSize));	// size of new FST
	
	// now the string table entry
	memcpy (pFSTDummy + 0x18L, "FakeFSTGoesHere", 0x0F);
	
	fOut = fopen ("FakeFST.bin", "wb");
	fwrite (pFSTDummy, 1, 0x28, fOut);
	fclose (fOut);
	
	/////////////////////////////////////////////////////////////////////////
	// Step 3 = create the new fst.bin we really want to use
	// in this case we are going to add 5 files into the image that will be
	// filled with blank space as even when encrypted the data will compress
	/////////////////////////////////////////////////////////////////////////
	
	
	// get the fst.bin from the image file
	pFST = (unsigned char *) malloc ((u_int32_t) (nFSTOldSize));
	
	io_read_part (pFST, (u_int32_t) (nFSTOldSize), image, nPartition,
				  image->parts[nPartition].header.fst_offset);
	// allocate enough for the altered version
	pFSTCopy = (unsigned char *) (malloc ((u_int32_t) (nFSTNewSize)));	// extra entry plus string chars
	
	// clear it out first
	memset (pFSTCopy, 0, (u_int32_t) (nFSTNewSize));
	
	
	// modify it to add extra files in the root directory using the name SCRUBPT1 to SCRUBPT5
	
	unsigned int nFiles = be32 (pFST + 8);
	
	unsigned int nStringTableOffset = (nFiles * 0x0c);
	
	// copy the existing data
	// Table first
	memcpy (pFSTCopy, pFST, nStringTableOffset);
	
	// then the string table
	memcpy (pFSTCopy + (nFiles + 5) * 0x0c, pFST + (nFiles * 0x0c),
			(u_int32_t) (nFSTOldSize) - (nFiles * 0x0c));
	
	
	// update the number of files in the copy by adding 5
	Write32 (pFSTCopy + 8, nFiles + 5);
	
	
	// available space is now what was available minus the size of the new FST.BIN rounded up
	// to nearest block
	u_int64_t nFSTTweak =
	(((nFSTNewSize / (u_int64_t) (0x7C00)) +
	  1) * (u_int64_t) (0x7C00));
	
	nSize = nSize - nFSTTweak;
	
	// save the number for later file creation
	
	nSizeForFiles = nSize;
	
	nOffset = nOffset + nFSTTweak;
	
	// now loop around adding the correct data for each of the table entries
	
	u_int32_t nFSTStringTableSize;
	
	nFSTStringTableSize =
	(u_int32_t) (nFSTNewSize) - ((nFiles + 5) * 0x0C);
	
	// now find where the existing data really ends by moving backwards from
	// the end until we find a non 0 character
	// this should stop fstreader getting it's knickers in a twist
	
	u_int32_t nStringPad = (u_int32_t) (nFSTNewSize) - 1;
	
	while (0 == pFSTCopy[nStringPad]) {
		nStringPad--;
	}
	
	// we are now at the first non 0 char
	// so add two to give us the correct offset
	nStringPad += 2;
	
	
	
	
	for (z = 0; z < 5; z++) {
		// string pointer at end of table minus 5 entries plus whatever entry we are at
		Write32 (pFSTCopy + (nFiles + z) * 0x0C,
				 nStringPad - ((nFiles + 5) * 0x0C) + (z * 0x0C));
		
		// then offset - value needs to be divided by 4 after adding on the block modified
		// fst.bin ammendment
		
		Write32 (pFSTCopy + (nFiles + z) * 0x0C + 0x04,
				 (u_int32_t) (nOffset >> 2));
		
		// then length - this will be either the max data size or
		// the actual size depending on the data we have left to pad
		if (nSize >= 0x3FFFFC00) {
			Write32 (pFSTCopy + (nFiles + z) * 0x0C + 0x08,
					 0x3FFFFC00);
			nSize = nSize - 0x3FFFFC00;
			nOffset = nOffset + 0x3FFFFC00;
		} else {
			u_int32_t nTempSize;
			nTempSize = (u_int32_t) (nSize);
			
			Write32 (pFSTCopy + (nFiles + z) * 0x0C + 0x08,
					 nTempSize);
			nSize = 0;
		}
		
		
		// add the string entry for the name "SCRUBPT1 to 5"
		memcpy (&pFSTCopy[nStringPad + (z * 0x0C)], "SCRUBPX.DAT",
				0x0B);
		// then add the part number
		pFSTCopy[nStringPad + (z * 0x0C) + 0x06] = '1' + z;
	}
	
	// save the fst.bin
	
	fOut = fopen ("SCRUBBEDFST.bin", "wb");
	fwrite (pFSTCopy, 1, (u_int32_t) (nFSTNewSize), fOut);
	fclose (fOut);
	
	/////////////////////////////////////////////////////////////////////////
	// now we need to create the blank files
	/////////////////////////////////////////////////////////////////////////
#if 0	//suk
	// create the progress bar etc.....
	CProgressBox *pProgressBox;
	MSG msg;
	
	pProgressBox = new CProgressBox (m_pParent);
	
	pProgressBox->Create (IDD_PROGRESSBOX);
	
	pProgressBox->ShowWindow (SW_SHOW);
	pProgressBox->SetRange (0,
							(int) ((nSizeForFiles /
									(u_int64_t) (0x7c00))));
	
	pProgressBox->SetPosition (0);
	pProgressBox->SetWindowMessage ("Saving blank files");
#endif
	int nPosition = 0;
	
	for (z = 1; z < 6; z++) {
		// create a file of the right size to pad it
		string csfName;
		
		// FIXME
		//		csfName.Format ("SCRUBP%d.DAT", z);
		
		if (0 != nSizeForFiles) {
			
			if ((u_int64_t) 0x3FFFFC00 <= nSizeForFiles) {
				if (1 == z)	// only create the first full file if we need to
				{
					fOut = fopen (csfName.c_str (), "wb");
					// full block
					// output in 0x7c00 blocks as the figure must be a multiple of this
					for (u_int32_t x = 0; x < 33825; x++) {
						fwrite (pBlankSector, 1, 0x7c00, fOut);
						nPosition++;
#if 0	//suk
						pProgressBox->
						SetPosition
						(nPosition);
						// do the message pump thang
						
						if (PeekMessage (&msg,
										 NULL,
										 0,
										 0,
										 PM_REMOVE)) {
							// PeekMessage has found a message--process it 
							if (msg.message !=
							    WM_CANCELLED) {
								TranslateMessage (&msg);	// Translate virt. key codes 
								DispatchMessage (&msg);	// Dispatch msg. to window 
							} else {
								// quit message received - simply exit
								
								AddToLog ("Save cancelled");
								delete pProgressBox;
								if (NULL !=
								    fOut) {
									fclose (fOut);
								}
								free (pBootBin);
								free (pFST);
								free (pFSTCopy);
								free (pFSTDummy);
								return false;
							}
						}
#endif
					}
					fclose (fOut);
				} else {
					nPosition += 33825L;
					//					pProgressBox->SetPosition (nPosition);
				}
				
				nSizeForFiles =
				nSizeForFiles -
				(u_int64_t) 0x3FFFFC00;
			} else {
				fOut = fopen (csfName.c_str (), "wb");
				// partially filled block - must be at the end
				// output in 0x7c00 blocks as the figure must be a multiple of this
				for (u_int32_t x = 0; x < nSizeForFiles;
				     x += 0x7C00) {
					fwrite (pBlankSector, 1, 0x7c00,
							fOut);
					nPosition++;
#if 0	//suk
					pProgressBox->SetPosition (nPosition);
					
					// do the message pump thang
					
					if (PeekMessage (&msg,
									 NULL,
									 0, 0, PM_REMOVE)) {
						// PeekMessage has found a message--process it 
						if (msg.message !=
						    WM_CANCELLED) {
							TranslateMessage (&msg);	// Translate virt. key codes 
							DispatchMessage (&msg);	// Dispatch msg. to window 
						} else {
							// quit message received - simply exit
							
							AddToLog ("Save cancelled");
							delete pProgressBox;
							if (NULL != fOut) {
								fclose (fOut);
							}
							free (pBootBin);
							free (pFST);
							free (pFSTCopy);
							free (pFSTDummy);
							return false;
						}
					}
#endif
				}
				nSizeForFiles = 0;
				fclose (fOut);
			}
			
		}
	}
	//	delete pProgressBox;
	
	// Phew! - now free up the memory
	free (pBootBin);
	free (pFST);
	free (pFSTCopy);
	free (pFSTDummy);
	return true;
	
}

void CWIIDisc::FindFreeSpaceInPartition (int64_t nPartOffset,
										 u_int64_t *pStart,
										 u_int64_t *pSize)
{
	
	// go through the data block table to find the first unused block starting
	// at the offest provided and use that to search to the end of the data or
	// the first marked.
	// As WII games all use a (  << 2) implementation of size then we just need to
	// used the supplied values divided by 4 for the returns
	
	char cLastBlock = 1;	// assume (!) that we have data at the partition start
	
	
	*pSize = 0;
	*pStart = 0;
	
	for (u_int64_t i = (nPartOffset / (u_int64_t) (0x8000));
	     i < (nImageSize / (u_int64_t) (0x8000)); i++) {
		if (cLastBlock != pFreeTable[i]) {
			// change 
			if (1 == cLastBlock) {
				// we have the first empty space
				*pStart = i * (u_int64_t) (0x8000);
				*pSize = (u_int64_t) (0x7c00);
			} else {
				// now found the end so simply can return as we have switched from a free to a used
				// we have to tweak the start though as we need to remove a few values
				return;
			}
		} else {
			// same so if we have a potential run
			if (0 == cLastBlock) {
				// add the block to the size
				*pSize += (u_int64_t) (0x7c00);
			}
		}
		cLastBlock = pFreeTable[i];
	}
	
	// if we get here then there is no free space OR we have passed the end of the
	// image after starting
	if (0 == cLastBlock) {
		// all okay and values are correct
	} else {
		// no free space
	}
}

////////////////////////////////////////////////////////////
// Inverse of the be32 function - writes a 32 bit value   //
// high to low                                            //
////////////////////////////////////////////////////////////
void CWIIDisc::Write32 (u_int8_t * p, u_int32_t nVal)
{
	p[0] = (nVal >> 24) & 0xFF;
	p[1] = (nVal >> 16) & 0xFF;
	p[2] = (nVal >> 8) & 0xFF;
	p[3] = (nVal) & 0xFF;
	
}
////////////////////////////////////////////////////////////////////////////////////////
// This function takes two FSTs and merges them using a passed offset as the start    //
// location for the new data.                                                         //
// UNFINISHED - UNFINISHED - UNFINISHED ETC...........................................//
////////////////////////////////////////////////////////////////////////////////////////
bool CWIIDisc::MergeAndRelocateFSTs (unsigned char *pFST1,
									 u_int32_t nSizeofFST1,
									 unsigned char *pFST2,
									 u_int32_t nSizeofFST2,
									 unsigned char *pNewFST,
									 u_int32_t * nSizeofNewFST,
									 u_int64_t nNewOffset,
									 u_int64_t nOldOffset)
{
	
	u_int32_t nFilesFST1 = 0;
	u_int32_t nFilesFST2 = 0;
//s	u_int32_t nFilesNewFST = 0;
	u_int32_t nStringTableOffset;
	
//s	u_int64_t nOffsetCalc = 0;
	u_int32_t nStringPad = 0;
	
	// extract the data for FST 1
	nFilesFST1 = be32 (pFST1 + 8);
	
	// extract the data for FST 2
	nFilesFST2 = be32 (pFST1 + 8);
	
	// merge the two entry offset tables (as we then will know where the string table starts)
	// copy the first one over
	memcpy (pNewFST, pFST1, nFilesFST1 * 0x0C);
	memcpy (pNewFST + (nFilesFST1 * 0x0C), pFST2 + 0x0C,
			(nFilesFST2 - 1) * 0x0C);
	
	nStringTableOffset = (nFilesFST1 + nFilesFST2 - 1) * 0x0c;
	
	// now copy the string tables
	memcpy (pNewFST + nStringTableOffset, pFST1 + (nFilesFST1 * 0x0C),
			nSizeofFST1 - (nFilesFST1 * 0x0C));
	
	// now search back to find the first non 0 character
	nStringPad = nSizeofFST1 + ((nFilesFST2 - 1) * 0x0C);
	
	while (0 == pNewFST[nStringPad]) {
		nStringPad--;
	}
	nStringPad += 2;
	
	// so that we then know the real positional offset to write to
	
	memcpy (pNewFST + nStringPad, pFST2 + (nFilesFST2 * 0x0C),
			nSizeofFST2 - (nFilesFST2 * 0x0C));
	
	// we then need to go through both sets of data in the copied FST2 data to mark
	// them correctly
	
	// HOW TO HANDLE DUPLICATE FILENAMES???
	// 
	
	
	
	// TO BE DONE - BUT NOT IN THIS APPLICATION ;)
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////
// Invert of the mark as used - to allow for                                //
// creation of a DIF file for a specific area e.g. mariokart partition 3    //
// Not really used these days                                               //
//////////////////////////////////////////////////////////////////////////////
void CWIIDisc::MarkAsUnused (u_int64_t nOffset, u_int64_t nSize)
{
	u_int64_t nStartValue = nOffset;
	u_int64_t nEndValue = nOffset + nSize;
	while ((nStartValue < nEndValue) &&
	       (nStartValue < ((u_int64_t) (4699979776LL) * (u_int64_t) (2))))
	{
		
		pFreeTable[nStartValue / (u_int64_t) (0x8000)] = 0;
		nStartValue = nStartValue + ((u_int64_t) (0x8000));
	}
	
}

bool CWIIDisc::DiscWriteDirect (struct image_file *image, u_int64_t nOffset,
								u_int8_t * pData, unsigned int nSize)
{
	
	int64_t nSeek;
	
	// Simply seek to the right place
	nSeek = my_fseek (image->fp, nOffset, SEEK_SET);
	
	if (-1 == nSeek) {
		//		m_pParent->AddToLog ("Seek error for write");
		AfxMessageBox ("io_seek");
		return false;
	}
	
	if (nSize != fwrite (pData, 1, nSize, image->fp)) {
		//		m_pParent->AddToLog ("Write error");
		AfxMessageBox ("Write error");
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
// The infamous TRUCHA signing bug                                      //
// where we change the reserved bytes in the ticket until the SHA has a //
// 0 in the first location                                              //
//////////////////////////////////////////////////////////////////////////

/* Isn't there a predefined constant for this?! */
#define U_INT32_T_MAX (~(u_int32_t) 0)

bool CWIIDisc::wii_trucha_signing (struct image_file * image, int partition)
{
	u_int8_t *buf, hash[20];
	u_int32_t size, val;
	
	/* Store TMD size */
	size = (u_int32_t) (image->parts[partition].tmd_size);
	
	/* Allocate memory */
	buf = (u_int8_t *) calloc (size, 1);
	
	if (!buf) {
		return false;
	}
	
	/* Jump to the partition TMD and read it */
	my_fseek (image->fp,
			  image->parts[partition].offset +
			  image->parts[partition].tmd_offset, SEEK_SET);
	if (size != fread (buf, 1, size, image->fp)) {
		return false;
	}
	
	/* Overwrite signature with trucha signature */
	memcpy (buf + 0x04, trucha_signature, 256);
	
	/* SHA-1 brute force */
	hash[0] = 1;
	for (val = 0; ((val <= U_INT32_T_MAX) && (hash[0] != 0x00)); val++) {
		/* Modify TMD "reserved" field */
		memcpy (buf + 0x19A, &val, sizeof (val));
		
		/* Calculate SHA-1 hash */
		SHA1 (buf + 0x140, size - 0x140, hash);
	}
	
	// check for the bug where the first byte of the hash is 0
	if (hash[0] == 0x00) {
		/* Write modified TMD data */
		my_fseek (image->fp,
					image->parts[partition].offset +
					image->parts[partition].tmd_offset,
					SEEK_SET);
		
		// write it out
		if (size != fwrite (buf, 1, size, image->fp)) {
			// error writing
			return false;
		} else {
			return true;
		}
	}
	return false;
}

// calculate the number of clusters
int CWIIDisc::wii_nb_cluster (struct image_file *iso, int partition)
{
	int nRetVal = 0;
	
	nRetVal = (int) (iso->parts[partition].data_size / SIZE_CLUSTER);
	
	return nRetVal;
}

// calculate the group hash for a cluster
bool CWIIDisc::wii_calc_group_hash (struct image_file * iso, int partition,
									int cluster)
{
	u_int8_t h2[SIZE_H2], h3[SIZE_H3], h4[SIZE_H4];
	u_int32_t group;
	
	/* Calculate cluster group */
	group = cluster / NB_CLUSTER_GROUP;
	
	/* Get H2 hash of the group */
	if (false ==
	    wii_read_cluster_hashes (iso, partition, cluster, NULL, NULL,
								 h2)) {
		return false;
	}
	
	/* read the H3 table offset */
	io_read (h3, SIZE_H3, iso,
			 iso->parts[partition].offset +
			 iso->parts[partition].h3_offset);
	
	
	/* Calculate SHA-1 hash */
	sha1 (h2, SIZE_H2, &h3[group * 0x14]);
	
	/* Write new H3 table */
	if (false ==
	    DiscWriteDirect (iso,
						 iso->parts[partition].h3_offset +
						 iso->parts[partition].offset, h3, SIZE_H3)) {
		return false;
	}
	
	
	/* Calculate H4 */
	sha1 (h3, SIZE_H3, h4);
	
	/* Write H4 */
	if (false ==
	    DiscWriteDirect (iso,
						 iso->parts[partition].tmd_offset +
						 OFFSET_TMD_HASH + iso->parts[partition].offset,
						 h4, SIZE_H4)) {
		return false;
	}
	
	
	return true;
}

int CWIIDisc::wii_read_cluster (struct image_file *iso, int partition,
								int cluster, u_int8_t * data,
								u_int8_t * header)
{
	u_int8_t buf[SIZE_CLUSTER];
	u_int8_t iv[16];
	u_int8_t *title_key;
	u_int64_t offset;
	
	
	/* Jump to the specified cluster and copy it to memory */
	offset = iso->parts[partition].offset +
	iso->parts[partition].data_offset +
	(u_int64_t) ((u_int64_t) cluster * (u_int64_t) SIZE_CLUSTER);
	
	// read the correct location block in
	io_read (buf, SIZE_CLUSTER, iso, offset);
	
	/* Set title key */
	title_key = &(iso->parts[partition].title_key[0]);
	
	/* Copy header if required */
	if (header) {
		/* Set IV key to all 0's */
		memset (iv, 0, sizeof (iv));
		
		/* Decrypt cluster header */
		aes_cbc_dec (buf, header, SIZE_CLUSTER_HEADER, title_key, iv);
	}
	
	/* Copy data if required */
	if (data) {
		/* Set IV key to correct location */
		memcpy (iv, &buf[OFFSET_CLUSTER_IV], 16);
		
		/* Decrypt cluster data */
		aes_cbc_dec (&buf[0x400], data, SIZE_CLUSTER_DATA, title_key,
					 &iv[0]);
		
	}
	
	return 0;
}

bool CWIIDisc::wii_write_cluster (struct image_file *iso, int partition,
								  int cluster, u_int8_t * in)
{
	u_int8_t h0[SIZE_H0];
	u_int8_t h1[SIZE_H1];
	u_int8_t h2[SIZE_H2];
	
	u_int8_t *data;
	u_int8_t *header;
	u_int8_t *title_key;
	
	u_int8_t iv[16];
	
	u_int32_t group,
	subgroup, f_cluster, nb_cluster, pos_cluster, pos_header;
	
	u_int64_t offset;
	
	u_int32_t i;
	//int ret = 0;
	
	/* Calculate cluster group and subgroup */
	group = cluster / NB_CLUSTER_GROUP;
	subgroup = (cluster % NB_CLUSTER_GROUP) / NB_CLUSTER_SUBGROUP;
	
	/* First cluster in the group */
	f_cluster = group * NB_CLUSTER_GROUP;
	
	/* Get number of clusters in this group */
	nb_cluster = wii_nb_cluster (iso, partition) - f_cluster;
	if (nb_cluster > NB_CLUSTER_GROUP)
		nb_cluster = NB_CLUSTER_GROUP;
	
	/* Allocate memory */
	data = (u_int8_t *) calloc (SIZE_CLUSTER_DATA * NB_CLUSTER_GROUP, 1);
	header = (u_int8_t *) calloc (SIZE_CLUSTER_HEADER * NB_CLUSTER_GROUP,
								  1);
	if (!data || !header)
		return false;
	
	/* Read group clusters and headers */
	for (i = 0; i < nb_cluster; i++) {
		u_int8_t *d_ptr = &data[SIZE_CLUSTER_DATA * i];
		u_int8_t *h_ptr = &header[SIZE_CLUSTER_HEADER * i];
		
		/* Read cluster */
		if (wii_read_cluster
		    (iso, partition, f_cluster + i, d_ptr, h_ptr)) {
			free (data);
			free (header);
			return false;
		}
	}
	
	/* Calculate new cluster H0 table */
	for (i = 0; i < SIZE_CLUSTER_DATA; i += 0x400) {
		u_int32_t idx = (i / 0x400) * 20;
		
		/* Calculate SHA-1 hash */
		sha1 (&in[i], 0x400, &h0[idx]);
	}
	
	/* Write new cluster and H0 table */
	pos_header = ((cluster - f_cluster) * SIZE_CLUSTER_HEADER);
	pos_cluster = ((cluster - f_cluster) * SIZE_CLUSTER_DATA);
	
	memcpy (&data[pos_cluster], in, SIZE_CLUSTER_DATA);
	memcpy (&header[pos_header + OFFSET_H0], h0, SIZE_H0);
	
	/* Calculate H1 */
	for (i = 0; i < NB_CLUSTER_SUBGROUP; i++) {
		u_int32_t pos =
		SIZE_CLUSTER_HEADER *
		((subgroup * NB_CLUSTER_SUBGROUP) + i);
		u_int8_t tmp[SIZE_H0];
		
		/* Cluster exists? */
		if ((pos / SIZE_CLUSTER_HEADER) > nb_cluster)
			break;
		
		/* Get H0 */
		memcpy (tmp, &header[pos + OFFSET_H0], SIZE_H0);
		
		/* Calculate SHA-1 hash */
		sha1 (tmp, SIZE_H0, &h1[20 * i]);
	}
	
	/* Write H1 table */
	for (i = 0; i < NB_CLUSTER_SUBGROUP; i++) {
		u_int32_t pos =
		SIZE_CLUSTER_HEADER *
		((subgroup * NB_CLUSTER_SUBGROUP) + i);
		
		/* Cluster exists? */
		if ((pos / SIZE_CLUSTER_HEADER) > nb_cluster)
			break;
		
		/* Write H1 table */
		memcpy (&header[pos + OFFSET_H1], h1, SIZE_H1);
	}
	
	/* Calculate H2 */
	for (i = 0; i < NB_CLUSTER_SUBGROUP; i++) {
		u_int32_t pos =
		(NB_CLUSTER_SUBGROUP * i) * SIZE_CLUSTER_HEADER;
		u_int8_t tmp[SIZE_H1];
		
		/* Cluster exists? */
		if ((pos / SIZE_CLUSTER_HEADER) > nb_cluster)
			break;
		
		/* Get H1 */
		memcpy (tmp, &header[pos + OFFSET_H1], SIZE_H1);
		
		/* Calculate SHA-1 hash */
		sha1 (tmp, SIZE_H1, &h2[20 * i]);
	}
	
	/* Write H2 table */
	for (i = 0; i < nb_cluster; i++) {
		u_int32_t nb = SIZE_CLUSTER_HEADER * i;
		
		/* Write H2 table */
		memcpy (&header[nb + OFFSET_H2], h2, SIZE_H2);
	}
	
	/* Set title key */
	title_key = &(iso->parts[partition].title_key[0]);
	
	/* Encrypt headers */
	for (i = 0; i < nb_cluster; i++) {
		u_int8_t *ptr = &header[SIZE_CLUSTER_HEADER * i];
		
		u_int8_t phData[SIZE_CLUSTER_HEADER];
		
		/* Set IV key */
		memset (iv, 0, 16);
		
		/* Encrypt */
		aes_cbc_enc (ptr, (u_int8_t *) phData, SIZE_CLUSTER_HEADER,
					 title_key, iv);
		memcpy (ptr, (u_int8_t *) phData, SIZE_CLUSTER_HEADER);
	}
	
	/* Encrypt clusters */
	for (i = 0; i < nb_cluster; i++) {
		u_int8_t *d_ptr = &data[SIZE_CLUSTER_DATA * i];
		u_int8_t *h_ptr = &header[SIZE_CLUSTER_HEADER * i];
		
		u_int8_t phData[SIZE_CLUSTER_DATA];
		
		
		/* Set IV key */
		memcpy (iv, &h_ptr[OFFSET_CLUSTER_IV], 16);
		
		/* Encrypt */
		aes_cbc_enc (d_ptr, (u_int8_t *) phData, SIZE_CLUSTER_DATA,
					 title_key, iv);
		memcpy (d_ptr, (u_int8_t *) phData, SIZE_CLUSTER_DATA);
	}
	
	/* Jump to first cluster in the group */
	offset = iso->parts[partition].offset +
	iso->parts[partition].data_offset +
	(u_int64_t) ((u_int64_t) f_cluster *
			     (u_int64_t) SIZE_CLUSTER);
	
	/* Write new clusters */
	for (i = 0; i < nb_cluster; i++) {
		u_int8_t *d_ptr = &data[SIZE_CLUSTER_DATA * i];
		u_int8_t *h_ptr = &header[SIZE_CLUSTER_HEADER * i];
		
		/* Write cluster header */
		if (true ==
		    DiscWriteDirect (iso, offset, h_ptr,
							 SIZE_CLUSTER_HEADER)) {
			// written ok, add value to offset
			offset = offset + SIZE_CLUSTER_HEADER;
			
			if (true ==
			    DiscWriteDirect (iso, offset, d_ptr,
								 SIZE_CLUSTER_DATA)) {
				offset = offset + SIZE_CLUSTER_DATA;
			} else {
				free (data);
				free (header);
				return false;
				
			}
		} else {
			// free memory and return error
			free (data);
			free (header);
			return false;
		}
	}
	
	/* Recalculate global hash table */
	if (wii_calc_group_hash (iso, partition, cluster)) {
		free (data);
		free (header);
		return false;
	}
	
	/* Free memory */
	free (data);
	free (header);
	
	return true;
}


bool CWIIDisc::wii_read_cluster_hashes (struct image_file * iso,
										int partition, int cluster,
										u_int8_t * h0, u_int8_t * h1,
										u_int8_t * h2)
{
	u_int8_t buf[SIZE_CLUSTER_HEADER];
	
	/* Read cluster header */
	if (wii_read_cluster (iso, partition, cluster, NULL, buf))
		return false;
	
	if (NULL != h0)
		memcpy (h0, buf + OFFSET_H0, SIZE_H0);
	if (NULL != h1)
		memcpy (h1, buf + OFFSET_H1, SIZE_H1);
	if (NULL != h2)
		memcpy (h2, buf + OFFSET_H2, SIZE_H2);
	
	return true;
}

int CWIIDisc::wii_read_data (struct image_file *iso, int partition,
							 u_int64_t offset, u_int32_t size,
							 u_int8_t ** out)
{
	u_int8_t *buf, *tmp;
	u_int32_t cluster_start, clusters, i, offset_start;
	
	
	/* Calculate some needed information */
	cluster_start =
	(u_int32_t) (offset / (u_int64_t) (SIZE_CLUSTER_DATA));
	clusters =
	(u_int32_t) (((offset +
			       (u_int64_t) (size)) /
			      (u_int64_t) (SIZE_CLUSTER_DATA))) -
	(cluster_start - 1);
	offset_start =
	(u_int32_t) (offset -
			     (cluster_start *
			      (u_int64_t) (SIZE_CLUSTER_DATA)));
	
	/* Allocate memory */
	buf = (u_int8_t *) calloc (clusters * SIZE_CLUSTER_DATA, 1);
	if (!buf)
		return 1;
	
	/* Read clusters */
	for (i = 0; i < clusters; i++)
		if (wii_read_cluster
		    (iso, partition, cluster_start + i,
		     &buf[SIZE_CLUSTER_DATA * i], NULL))
			return 1;
	
	/* Allocate memory */
	tmp = (u_int8_t *) calloc (size, 1);
	if (!tmp)
		return 1;
	
	/* Copy specified data */
	memcpy (tmp, buf + offset_start, size);
	
	/* Free unused memory */
	free (buf);
	
	/* Set pointer address */
	*out = tmp;
	
	return 0;
}


void CWIIDisc::sha1 (u_int8_t *data, u_int32_t len, u_int8_t *hash) {
	SHA1 (data, len, hash);
}

void CWIIDisc::aes_cbc_enc (u_int8_t * in, u_int8_t * out, u_int32_t len, u_int8_t * key, u_int8_t * iv) {
	AES_KEY aes_key;
	
	/* Set encryption key */
	AES_set_encrypt_key (key, 128, &aes_key);
	
	/* Decrypt data */
	AES_cbc_encrypt (in, out, len, &aes_key, iv, AES_ENCRYPT);
}

void CWIIDisc::aes_cbc_dec (u_int8_t * in, u_int8_t * out, u_int32_t len, u_int8_t * key, u_int8_t * iv) {
	AES_KEY aes_key;
	
	/* Set decryption key */
	AES_set_decrypt_key (key, 128, &aes_key);
	
	/* Decrypt data */
	AES_cbc_encrypt (in, out, len, &aes_key, iv, AES_DECRYPT);
}

u_int64_t CWIIDisc::
FindRequiredFreeSpaceInPartition (struct image_file *image,
								  u_int64_t nPartition,
								  u_int32_t nRequiredSize)
{
	
	// search through the free space to find a free area that is at least
	// the required size. We can then return the position of the free space
	// relative to the data area of the partition
	char cLastBlock = 1;	// assume (!) that we have data at the partition start
	
	u_int32_t nRequiredBlocks = (nRequiredSize / 0x7c00);
	
	if (0 != (nRequiredSize % 0x7c00)) {
		// we require an extra block
		nRequiredBlocks++;
	}
	
	u_int64_t nReturnOffset = 0;
	
	u_int64_t nStartOffset =
	image->parts[nPartition].offset +
	image->parts[nPartition].data_offset;
	
	u_int64_t nEndOffset =
	nStartOffset + image->parts[nPartition].data_size;
	u_int64_t nCurrentOffset = nStartOffset;
	
	u_int64_t nMarkedStart = 0;
	u_int32_t nFreeBlocks = 0;
	u_int32_t nBlock = 0;
	
	// now go through the marked list to find the free area of the required size
	while (nCurrentOffset < nEndOffset) {
		nBlock = (u_int32_t) (nCurrentOffset / (u_int64_t) (0x8000));
		if (cLastBlock != pFreeTable[nBlock]) {
			// change 
			if (1 == cLastBlock) {
				// we have the first empty space
				nMarkedStart = nCurrentOffset;
				nFreeBlocks = 1;
			}
			// else we just store the value and wait for one of the other fallouts
		} else {
			// same so if we have a potential run
			if (0 == cLastBlock) {
				// add the block to the size
				nFreeBlocks++;
				// now check to see if we have enough
				if (nFreeBlocks >= nRequiredBlocks) {
					// BINGO! - convert into relative offset from data start
					// and in encrypted format
					nReturnOffset =
					((nMarkedStart -
					  nStartOffset) /
					 (u_int64_t) 0x8000) *
					(u_int64_t) (0x7c00);
					return nReturnOffset;
				}
			}
		}
		cLastBlock = pFreeTable[nBlock];
		
		nCurrentOffset = nCurrentOffset + (u_int64_t) (0x8000);
	}
	
	// if we get here then we didn't find some space :(
	
	return 0;
}

/////////////////////////////////////////////////////////////
// Check to see if we have free space for so many blocks   //
/////////////////////////////////////////////////////////////

bool CWIIDisc::CheckForFreeSpace (image_file * image, u_int32_t nPartition,
								  u_int64_t nOffset, u_int32_t nBlocks)
{
	
	// convert offset to block representation
	u_int32_t nBlockOffsetStart = 0;
	
	nBlockOffsetStart =
	(u_int32_t) ((image->parts[nPartition].data_offset +
			      image->parts[nPartition].offset) /
			     (u_int64_t) 0x8000);
	nBlockOffsetStart =
	nBlockOffsetStart +
	(u_int32_t) (nOffset / (u_int64_t) 0x7c00);
	if (0 != nOffset % 0x7c00) {
		// starts part way into a block so need to check the number of blocks plus one
		nBlocks++;
		// and the start is up by one as we know that there must be data in the current
		// block
		nBlockOffsetStart++;
	}
	
	for (u_int32_t x = 0; x < nBlocks; x++) {
		if (1 == pFreeTable[nBlockOffsetStart + x]) {
			return false;
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
// Routine deletes the highlighted partition                            //
// does this by moving all the sucessive data up in the partition table //
// to overwrite the deleted partition                                   //
// It then updates the partition count                                  //
// Also works on channels                                               //
//////////////////////////////////////////////////////////////////////////
bool CWIIDisc::DeletePartition (image_file * image, u_int32_t nPartition)
{
	
	u_int8_t buffer[16];
	u_int64_t WriteLocation;
	int i;
	
	memset (buffer, 0, 16);
	
	// check the partition is either a partition or a channel
	if (PART_VC == image->parts[nPartition].type) {
		// use the channels
		
		// find out which partition we are really deleting
		// as the value is offset by the number of real partitions
		nPartition = nPartition - image->PartitionCount;
		
		// update the count of channels in the correct location
		Write32 (buffer, image->ChannelCount - 1);
		DiscWriteDirect (image, (u_int64_t) 0x40008, buffer, 4);
		
		// create the updated channel list in the correct location on the disc 
		WriteLocation =
		image->chan_tbl_offset +
		(u_int64_t) (8) * (u_int64_t) (nPartition - 1);
		
		for (i = nPartition; i < image->ChannelCount; i++) {
			// read the next partition info
			io_read (buffer, 8, image,
					 image->chan_tbl_offset +
					 (u_int64_t) (8) * (u_int64_t) (i));
			// write it out over the deleted one
			DiscWriteDirect (image, WriteLocation, buffer, 8);
			WriteLocation = WriteLocation + 8;
		}
		// now overwrite the last one with blanks
		memset (buffer, 0, 16);
		DiscWriteDirect (image, WriteLocation, buffer, 8);
		
	} else {
		// it's the partition table
		
		// update the count of partitions
		Write32 (buffer, image->PartitionCount - 1);
		DiscWriteDirect (image, (u_int64_t) 0x40000, buffer, 4);
		
		// create the partition table
		WriteLocation =
		image->part_tbl_offset +
		(u_int64_t) (8) * (u_int64_t) (nPartition - 1);
		
		for (i = nPartition; i < image->PartitionCount; i++) {
			// read the next partition info
			io_read (buffer, 8, image,
					 image->part_tbl_offset +
					 (u_int64_t) (8) * (u_int64_t) (i));
			// write it out over the deleted one
			DiscWriteDirect (image, WriteLocation, buffer, 8);
			WriteLocation = WriteLocation + 8;
		}
		// now overwrite the last one with blanks
		memset (buffer, 0, 16);
		DiscWriteDirect (image, WriteLocation, buffer, 8);
		
	}
	
	return true;
}

//////////////////////////////////////////////////////////////////////////
// Resize the partition data size field                                 //
// as some discs have 'interesting' values in here                      //
//////////////////////////////////////////////////////////////////////////
bool CWIIDisc::ResizePartition (image_file * image, u_int32_t nPartition)
{
	u_int64_t nCurrentSize = 0;
	u_int64_t nMinimumSize = 0;
	u_int64_t nMaximumSize = 0;
	u_int64_t nNewSize = 0;
	
	//u_int8_t buffer[16];
	
	// Get size of current partition
	nCurrentSize = image->parts[nPartition].data_size;
	nNewSize = nCurrentSize;
	
	// calculate maximum size (based on next partition start)
	// or disc size if the last one
	
	if (nPartition == image->PartitionCount) {
		nMaximumSize = nImageSize;
	} else {
		nMaximumSize = image->parts[nPartition + 1].offset;
	}
	nMaximumSize = nMaximumSize - image->parts[nPartition].offset;
	nMaximumSize = nMaximumSize - image->parts[nPartition].data_offset;
	
	// calculate minimum size by looking for where the data is
	// on the disc backwards from the current partition data end
	// create the window with the data
	
	nMinimumSize =
	SearchBackwards (nMaximumSize,
					 image->parts[nPartition].offset +
					 image->parts[nPartition].data_offset);
	
	// create window and 
	// and then ask for the values
	printf ("CANNOT RESIZE YET\n");
	exit (55);
#if 0
	CResizePartition *pWindow = new CResizePartition ();
	
	
	pWindow->SetRanges (nMinimumSize, nCurrentSize, nMaximumSize);
	
	if (IDOK == pWindow->DoModal ()) {
		// if values changed and OK pressed then update the correct pointer in the disc
		// image
		nNewSize = pWindow->GetNewSize ();
		
		delete pWindow;
		if (nNewSize == nCurrentSize) {
			AddToLog ("Sizes the same");
			return false;
		}
		// now simply write out the new size and store it
		image->parts[nPartition].data_size = nNewSize;
		Write32 (buffer, (u_int32_t) ((u_int64_t) nNewSize >> 2));
		DiscWriteDirect (image,
						 image->parts[nPartition].offset + 0x2bc,
						 buffer, 4);
		
		return true;
	}
	// don't even need to reparse as the values will be updated internally
	delete pWindow;
	return false;
#endif
}


u_int64_t CWIIDisc::SearchBackwards (u_int64_t nStartPosition,
									 u_int64_t nEndPosition)
{
	
	u_int64_t nCurrentBlock;
	u_int64_t nEndBlock;
	u_int64_t nStartBlock;
	
	nCurrentBlock =
	(nStartPosition + nEndPosition - 1) / (u_int64_t) (0x8000);
	nStartBlock = nCurrentBlock;
	
	nEndBlock = nEndPosition / (u_int64_t) (0x8000);
	
	while (nCurrentBlock > nEndBlock) {
		if (0 == pFreeTable[nCurrentBlock]) {
			nCurrentBlock--;
		} else {
			// if it's the first block then we are at the start position
			if (nStartBlock == nCurrentBlock) {
				return (nCurrentBlock - nEndBlock +
						1) * ((u_int64_t) (0x8000));
			} else {
				return (nCurrentBlock -
						nEndBlock) * ((u_int64_t) (0x8000));
			}
		}
	}
	return 0;
}


////////////////////////////////////////////////////////////////////////////
// Modification of the write_cluster function to write multiple clusters  //
// in one sitting. This means the disc access should then be minimized    //
// It also allows for a file to be used for the input instead of a memory //
// pointer as that allows for larger files to be updated. I'm talking to  //
// you Okami.....                                                         //
////////////////////////////////////////////////////////////////////////////
bool CWIIDisc::wii_write_clusters (struct image_file * iso, int partition,
								   int cluster, u_int8_t * in,
								   u_int32_t nClusterOffset,
								   u_int32_t nBytesToWrite, FILE * fIn)
{
	u_int8_t h0[SIZE_H0];
	u_int8_t h1[SIZE_H1];
	u_int8_t h2[SIZE_H2];
	
	u_int8_t *data;
	u_int8_t *header;
	u_int8_t *title_key;
	
	u_int8_t iv[16];
	
	u_int32_t group,
	subgroup, f_cluster, nb_cluster, pos_cluster, pos_header;
	
	u_int64_t offset;
	
	u_int32_t i;
	int j;
	
	//int ret = 0;
	
	int nClusters = 0;
	
	/* Calculate cluster group and subgroup */
	group = cluster / NB_CLUSTER_GROUP;
	subgroup = (cluster % NB_CLUSTER_GROUP) / NB_CLUSTER_SUBGROUP;
	
	/* First cluster in the group */
	f_cluster = group * NB_CLUSTER_GROUP;
	
	/* Get number of clusters in this group */
	nb_cluster = wii_nb_cluster (iso, partition) - f_cluster;
	if (nb_cluster > NB_CLUSTER_GROUP)
		nb_cluster = NB_CLUSTER_GROUP;
	
	/* Allocate memory */
	data = (u_int8_t *) calloc (SIZE_CLUSTER_DATA * NB_CLUSTER_GROUP, 1);
	header = (u_int8_t *) calloc (SIZE_CLUSTER_HEADER * NB_CLUSTER_GROUP,
								  1);
	if (!data || !header)
		return false;
	
	// if we are replacing a full set of clusters then we don't
	// need to do any reading as we just need to overwrite the
	// blanked data
	
	
	// calculate number of clusters of data to write
	nClusters = ((nBytesToWrite + nClusterOffset - 1) / SIZE_CLUSTER_DATA) + 1;
	
	if (nBytesToWrite != (NB_CLUSTER_GROUP * SIZE_CLUSTER_DATA)) {
		/* Read group clusters and headers */
		for (i = 0; i < nb_cluster; i++) {
			u_int8_t *d_ptr = &data[SIZE_CLUSTER_DATA * i];
			u_int8_t *h_ptr = &header[SIZE_CLUSTER_HEADER * i];
			
			/* Read cluster */
			if (wii_read_cluster
			    (iso, partition, f_cluster + i, d_ptr, h_ptr)) {
				free (data);
				free (header);
				return false;
			}
		}
	} else {
		// memory already cleared
	}
	
	// now overwrite the data in the correct location
	// be it from file data or from the memory location
	/* Write new cluster and H0 table */
	pos_header = ((cluster - f_cluster) * SIZE_CLUSTER_HEADER);
	pos_cluster = ((cluster - f_cluster) * SIZE_CLUSTER_DATA);
	
	
	// we read from either memory or a file
	if (NULL != fIn) {
		fread (&data[pos_cluster + nClusterOffset], 1, nBytesToWrite,
		       fIn);
	} else {
		// data
		memcpy (&data[pos_cluster + nClusterOffset], in,
				nBytesToWrite);
	}
	
	// now for each cluster we need to...
	for (j = 0; j < nClusters; j++) {
		// clear the data for the table
		memset (h0, 0, SIZE_H0);
		
		/* Calculate new clusters H0 table */
		for (i = 0; i < SIZE_CLUSTER_DATA; i += 0x400) {
			u_int32_t idx = (i / 0x400) * 20;
			
			/* Calculate SHA-1 hash */
			sha1 (&data
			      [pos_cluster + (j * SIZE_CLUSTER_DATA) + i],
			      0x400, &h0[idx]);
		}
		
		// save the H0 data
		memcpy (&header[pos_header + (j * SIZE_CLUSTER_HEADER)], h0,
				SIZE_H0);
		
		// now do the H1 data for the subgroup
		/* Calculate H1's */
		sha1 (&header[pos_header + (j * SIZE_CLUSTER_HEADER)],
		      SIZE_H0, h1);
		
		// now copy to all the sub cluster locations
		for (int k = 0; k < NB_CLUSTER_SUBGROUP; k++) {
			// need to get the position of the first block we are changing
			// which is the start of the subgroup for the current cluster 
			u_int32_t nSubGroup =
			((cluster +
			  j) % NB_CLUSTER_GROUP) /
			NB_CLUSTER_SUBGROUP;
			
			u_int32_t pos =
			(SIZE_CLUSTER_HEADER * nSubGroup *
			 NB_CLUSTER_SUBGROUP) +
			(0x14 *
			 ((cluster + j) % NB_CLUSTER_SUBGROUP));
			
			memcpy (&header
					[pos + (k * SIZE_CLUSTER_HEADER) + OFFSET_H1],
					h1, 20);
		}
		
	}
	
	
	// now we need to calculate the H2's for all subgroups
	/* Calculate H2 */
	for (i = 0; i < NB_CLUSTER_SUBGROUP; i++) {
		u_int32_t pos =
		(NB_CLUSTER_SUBGROUP * i) * SIZE_CLUSTER_HEADER;
		
		/* Cluster exists? */
		if ((pos / SIZE_CLUSTER_HEADER) > nb_cluster)
			break;
		
		/* Calculate SHA-1 hash */
		sha1 (&header[pos + OFFSET_H1], SIZE_H1, &h2[20 * i]);
	}
	
	/* Write H2 table */
	for (i = 0; i < nb_cluster; i++) {
		/* Write H2 table */
		memcpy (&header[(SIZE_CLUSTER_HEADER * i) + OFFSET_H2], h2,
				SIZE_H2);
	}
	
	// update the H3 key table here
	/* Calculate SHA-1 hash */
	sha1 (h2, SIZE_H2, &h3[group * 0x14]);
	
	
	// now encrypt and write
	
	/* Set title key */
	title_key = &(iso->parts[partition].title_key[0]);
	
	/* Encrypt headers */
	for (i = 0; i < nb_cluster; i++) {
		u_int8_t *ptr = &header[SIZE_CLUSTER_HEADER * i];
		
		u_int8_t phData[SIZE_CLUSTER_HEADER];
		
		/* Set IV key */
		memset (iv, 0, 16);
		
		/* Encrypt */
		aes_cbc_enc (ptr, (u_int8_t *) phData, SIZE_CLUSTER_HEADER,
					 title_key, iv);
		memcpy (ptr, (u_int8_t *) phData, SIZE_CLUSTER_HEADER);
	}
	
	/* Encrypt clusters */
	for (i = 0; i < nb_cluster; i++) {
		u_int8_t *d_ptr = &data[SIZE_CLUSTER_DATA * i];
		u_int8_t *h_ptr = &header[SIZE_CLUSTER_HEADER * i];
		
		u_int8_t phData[SIZE_CLUSTER_DATA];
		
		
		/* Set IV key */
		memcpy (iv, &h_ptr[OFFSET_CLUSTER_IV], 16);
		
		/* Encrypt */
		aes_cbc_enc (d_ptr, (u_int8_t *) phData, SIZE_CLUSTER_DATA,
					 title_key, iv);
		memcpy (d_ptr, (u_int8_t *) phData, SIZE_CLUSTER_DATA);
	}
	
	/* Jump to first cluster in the group */
	offset = iso->parts[partition].offset +
	iso->parts[partition].data_offset +
	(u_int64_t) ((u_int64_t) f_cluster *
			     (u_int64_t) SIZE_CLUSTER);
	
	for (i = 0; i < nb_cluster; i++) {
		u_int8_t *d_ptr = &data[SIZE_CLUSTER_DATA * i];
		u_int8_t *h_ptr = &header[SIZE_CLUSTER_HEADER * i];
		
		if (true ==
		    DiscWriteDirect (iso, offset, h_ptr,
							 SIZE_CLUSTER_HEADER)) {
			// written ok, add value to offset
			offset = offset + SIZE_CLUSTER_HEADER;
			
			if (true ==
			    DiscWriteDirect (iso, offset, d_ptr,
								 SIZE_CLUSTER_DATA)) {
				offset = offset + SIZE_CLUSTER_DATA;
			} else {
				free (data);
				free (header);
				return false;
				
			}
		} else {
			// free memory and return error
			free (data);
			free (header);
			return false;
		}
	}
	
	
	// already calculated the H3 and H4 hashes - rely on surrounding code to
	// read and write those out
	
	/* Free memory */
	free (data);
	free (header);
	
	return true;
}

////////////////////////////////////////////////////////////////////////////
// Heavily optimised file write routine so that the minimum number of     //
// SHA calculations have to be performed                                  //
// We do this by writing in 1 clustergroup per write and calculate the    //
// Offset to write the data in the minimum number of chunks               //
// A bit like lumpy chunk packer from the Atari days......................//
////////////////////////////////////////////////////////////////////////////
bool CWIIDisc::wii_write_data_file (struct image_file * iso, int partition,
									u_int64_t offset, u_int64_t size,
									u_int8_t * in, FILE * fIn)
{
	u_int32_t cluster_start, clusters, offset_start;
	
	u_int64_t i;
	
	u_int32_t nClusterCount;
	u_int32_t nWritten = 0;
	
	
	/* Calculate some needed information */
	cluster_start =
	(u_int32_t) (offset / (u_int64_t) (SIZE_CLUSTER_DATA));
	clusters =
	(u_int32_t) (((offset +
			       size) / (u_int64_t) (SIZE_CLUSTER_DATA)) -
			     (cluster_start - 1));
	offset_start =
	(u_int32_t) (offset -
			     (cluster_start *
			      (u_int64_t) (SIZE_CLUSTER_DATA)));
	
	
	// read the H3 and H4
	io_read (h3, SIZE_H3, iso,
			 iso->parts[partition].offset +
			 iso->parts[partition].h3_offset);
	
	/* Write clusters */
	i = 0;
	nClusterCount = 0;
#if 0
	CProgressBox *pProgressBox;
	
	pProgressBox = new CProgressBox (m_pParent);
	
	pProgressBox->Create (IDD_PROGRESSBOX);
	
	pProgressBox->ShowWindow (SW_SHOW);
	pProgressBox->SetRange (0, clusters);
	
	pProgressBox->SetPosition (0);
	
	
	pProgressBox->SetWindowMessage ("Replacing file: please wait");
#endif
	while (i < size) {
#if 0	//suk
		pProgressBox->SetPosition (nClusterCount);
		
		// do the message pump thang
		
		if (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) {
			// PeekMessage has found a message--process it 
			if (msg.message != WM_CANCELLED) {
				TranslateMessage (&msg);	// Translate virt. key codes 
				DispatchMessage (&msg);	// Dispatch msg. to window 
			} else {
				// quit message received - simply exit
				delete pProgressBox;
				AddToLog ("Load cancelled - disc probably unusable");
				AfxMessageBox
				("Load cancelled - disc probably unusable in current state");
				return false;
			}
		}
#endif
		// now the fun bit as we need to cater for the start position changing as well as the 
		// wrap over 
		if ((0 != ((cluster_start + nClusterCount) % 64)) ||
		    (0 != offset_start)) {
			// not at the start so our max size is different
			// and also our cluster offset
			nWritten =
			(NB_CLUSTER_GROUP -
			 (cluster_start % 64)) * SIZE_CLUSTER_DATA;
			nWritten = nWritten - offset_start;
			
			// max check
			if (nWritten > size) {
				nWritten = (u_int32_t) size;
			}
			
			if (false ==
			    wii_write_clusters (iso, partition, cluster_start,
									in, offset_start, nWritten,
									fIn)) {
				// Error
				//				delete pProgressBox;
				AfxMessageBox ("Error writing clusters");
				return false;
			}
			// round up the cluster count
			nClusterCount =
			NB_CLUSTER_GROUP -
			(cluster_start % NB_CLUSTER_GROUP);
		} else {
			// potentially full block
			nWritten = NB_CLUSTER_GROUP * SIZE_CLUSTER_DATA;
			
			// max check
			if (nWritten > (size - i)) {
				nWritten = (u_int32_t) (size - i);
			}
			
			if (false ==
			    wii_write_clusters (iso, partition,
									cluster_start + nClusterCount,
									in, offset_start, nWritten,
									fIn)) {
				// Error
				//				delete pProgressBox;
				AfxMessageBox ("Error writing clusters");
				return false;
			}
			// we simply add a full cluster block
			nClusterCount = nClusterCount + NB_CLUSTER_GROUP;
			
		}
		offset_start = 0;
		i += nWritten;
		
		
	}
	
	//	delete pProgressBox;
	
	// write out H3 and H4
	
	if (false ==
	    DiscWriteDirect (iso,
						 iso->parts[partition].h3_offset +
						 iso->parts[partition].offset, h3, SIZE_H3)) {
		AfxMessageBox ("Unable to write H3 table");
		return false;
	}
	
	
	/* Calculate H4 */
	sha1 (h3, SIZE_H3, h4);
	
	/* Write H4 */
	if (false ==
	    DiscWriteDirect (iso,
						 iso->parts[partition].tmd_offset +
						 OFFSET_TMD_HASH + iso->parts[partition].offset,
						 h4, SIZE_H4)) {
		AfxMessageBox ("Unable to write H4 value");
		return false;
	}
	// sign it
	wii_trucha_signing (iso, partition);
	
	return true;
}


bool CWIIDisc::SetBootMode (image_file * image)
{
	u_int8_t cOldValue;
	int i;
	
	unsigned char cModes[5] = { 'R', '_', 'H', '0', '4' };
// 	string csText;
	
	// get the current first byte of data from the passed ISO
	io_read (&cOldValue, 1, image, 0);
	
	for (i = 0; i < 5; i++) {
		if (cOldValue == cModes[i]) {
			break;
		}
	}
	printf ("NYI\n");
	exit (56);
#if 0
	// check for error - not found
	// should NEVER get error as it would have failed the initial parse
	// routine
	if (5 == i) {
		// FIXME
		//		csText.Format ("Current mode not valid = %x [%c]", cOldValue,
		//			       cOldValue);
		//		AfxMessageBox (csText);
		return false;
	}
	// Create the change display
	// create window and 
	// and then ask for the values
	
	CBootMode *pWindow = new CBootMode ();
	pWindow->SetBootMode (i);
	
	if (IDOK == pWindow->DoModal ()) {
		// if values changed and OK pressed then update the correct pointer in the disc
		// image
		if (i != pWindow->GetBootMode ()) {
			// changed so alter byte
			DiscWriteDirect (image, 0,
							 &cModes[pWindow->GetBootMode ()], 1);
			csText.Format ("Boot mode now [%c]",
						   cModes[pWindow->GetBootMode ()]);
			AddToLog (csText);
		} else {
			// same value
			AddToLog ("Same boot mode - no action taken");
		}
		
		
	} else {
		AddToLog ("Boot change cancelled");
	}
	delete pWindow;
	return true;
#endif
}

bool CWIIDisc::AddPartition (image_file * image, bool bChannel,
							 u_int64_t nOffset, u_int64_t nDataSize,
							 u_int8_t * pText)
{
	
	// just try and see if this works at the moment
	u_int8_t buffer[16];
	u_int64_t WriteLocation;
	
	memset (buffer, 0, 16);
	
	// check the partition is either a partition or a channel
	if (true == bChannel) {
		// use the channels
		// update the count of channels in the correct location
		Write32 (buffer, image->ChannelCount + 1);
		DiscWriteDirect (image, (u_int64_t) 0x40008, buffer, 4);
		
		// check to see if we actually have any channels defined and hence a value in the channel table offset
		if (0 == image->chan_tbl_offset) {
			// we need to create the table from scratch
			image->chan_tbl_offset = 0x41000;
			Write32 (buffer, 0x41000 >> 2);
			DiscWriteDirect (image, (u_int64_t) 0x4000C, buffer,
							 4);
		}
		// create the updated channel list in the correct location on the disc 
		WriteLocation =
		image->chan_tbl_offset +
		(u_int64_t) (8) * (u_int64_t) (image->ChannelCount);
		// write out the correct data block
		// set the buffer for start location and channel name
		Write32 (buffer, (u_int32_t) (nOffset >> 2));
		buffer[4] = pText[0];
		buffer[5] = pText[1];
		buffer[6] = pText[2];
		buffer[7] = pText[3];
		
		DiscWriteDirect (image, WriteLocation, buffer, 8);
		
	} else {
		// it's the partition table
		
		// update the count of partitions
		Write32 (buffer, image->PartitionCount + 1);
		DiscWriteDirect (image, (u_int64_t) 0x40000, buffer, 4);
		
		// create the partition table entry
		WriteLocation =
		image->part_tbl_offset +
		(u_int64_t) (8) * (u_int64_t) (image->PartitionCount);
		
		// set the buffer
		Write32 (buffer, (u_int32_t) (nOffset >> 2));
		Write32 (buffer + 4, 0);
		
		DiscWriteDirect (image, WriteLocation, buffer, 8);
		
	}
	
	// now create the necessary fake entries for all the data block values
	// h3 = 0x2b4
	Write32 (buffer, 0x8000 >> 2);
	// data offset = 0x2b8
	Write32 (buffer + 4, 0x20000 >> 2);
	// data size = 0x2bc
	Write32 (buffer + 8, (u_int32_t) (nDataSize >> 2));
	DiscWriteDirect (image, nOffset + 0x2b4, buffer, 12);
	
	// Should now create a fake boot.bin etc. to avoid disc reads and allow you to modify the
	// partition
	
	
	
	return true;
}

/////////////////////////////////////////////////////////////
// Function to find out the maximum size of a partition    //
// that can be added to the current image                  //
/////////////////////////////////////////////////////////////
u_int64_t CWIIDisc::GetFreeSpaceAtEnd (image_file *image) {
	u_int64_t nRetVal;
	struct partition part;
	
	// simple enough calculation in that we simply take the last partitions
	// offset and size and 0x1800 off the image size
	
	if (image -> PartitionCount == 0) {
		// no partitions here. We now use the image size minus the
		// default offset of 0x50000
		nRetVal = nImageSize - 0x50000;
	} else {
		// it's equal to the image minus size of the last partition and offset
		part = image -> parts[image -> PartitionCount];
		nRetVal = nImageSize - part.offset - part.data_offset - part.data_size;
	}
	
	return nRetVal;
}

////////////////////////////////////////////////////////////
// Gets the start of the partion space                    //
////////////////////////////////////////////////////////////
u_int64_t CWIIDisc::GetFreePartitionStart (image_file * image)
{
	u_int64_t nRetVal;
	struct partition part;
	
	if (image -> PartitionCount == 0) {
		// default offset of 0x50000
		nRetVal = 0x50000;
	} else {
		// get the first free byte at the end
		part = image -> parts[image -> PartitionCount];
		nRetVal = part.offset +	part.data_offset + part.data_size;
	}
	
	
	
	return nRetVal;
}
/////////////////////////////////////////////////////////////
// Goes through the partitions moving them up and updating //
// the partition table                                     //
/////////////////////////////////////////////////////////////

bool CWIIDisc::DoTheShuffle (image_file *image) {
	bool bRetVal = false;
	u_int64_t nStoreLocation = 0x50000;
	u_int64_t nPartitionStart;
	u_int64_t nLength = 0;
	u_int64_t nWriteLocation;
	u_int8_t nBuffer[4];
	u_int8_t i;
	
	for (i = 1; i < image->PartitionCount + 1; i++) {		
		// get the length and start of the partition
		nPartitionStart = image->parts[i].offset;
		nLength = image->parts[i].data_size + 0x20000;
		
		// check to see if we need to move it
		if (nPartitionStart != nStoreLocation) {
			// move the partition down
			
			if (false ==
			    CopyDiscDataDirect (image, i, nPartitionStart,
									nStoreLocation, nLength)) {
				// cancelled
				return false;
			}
			// show we have modified something
			
			bRetVal = true;
			Write32 (nBuffer, (u_int32_t) (nStoreLocation >> 2));
			// update the correct table
			if (i > image->PartitionCount) {
				// use the channel table
				// create the updated channel list in the correct location on the disc 
				nWriteLocation =
				image->chan_tbl_offset +
				(u_int64_t) (8) * (u_int64_t) (i -
											   image->
											   PartitionCount
											   - 1);
				DiscWriteDirect (image, nWriteLocation,
								 nBuffer, 4);
			} else {
				// use the partition table
				nWriteLocation =
				image->part_tbl_offset +
				(u_int64_t) (8) * (u_int64_t) (i - 1);
				DiscWriteDirect (image, nWriteLocation,
								 nBuffer, 4);
			}
		}
		nStoreLocation = nLength + nStoreLocation;
	}
	return bRetVal;
}

//////////////////////////////////////////////////////////////
// Copy data between two parts of the disc image            //
//////////////////////////////////////////////////////////////
bool CWIIDisc::CopyDiscDataDirect (image_file * image, int nPart,
								   u_int64_t nSource, u_int64_t nDest,
								   u_int64_t nLength)
{
	// optomise for 32k chunks
	u_int64_t nCount;
	u_int32_t nBlocks = 0;
	u_int32_t nBlockCount = 0;
	u_int32_t nReadCount = 0;
	
	u_int8_t *pData;
	
	// try and use 1 meg at a time
	pData = (u_int8_t *) malloc (0x100000);
	
	nCount = 0;
	nBlocks = 0;
	nBlockCount = (u_int32_t) ((nLength / 0x100000) + 1);
#if 0	//suk
	// now open a progress bar
	CProgressBox *pProgressBox = new CProgressBox (m_pParent);
	
	pProgressBox->Create (IDD_PROGRESSBOX);
	
	pProgressBox->ShowWindow (SW_SHOW);
	pProgressBox->SetRange (0, nBlockCount);
	
	pProgressBox->SetPosition (0);
	
	string csTempString;
	
	csTempString.Format ("Copying down partition %d", nPart);
	pProgressBox->SetWindowMessage (csTempString);
#endif
	// now the loop
	while (nCount < nLength) {
		
		//		pProgressBox->SetPosition (nBlocks);
		
		nReadCount = 0x100000;
		if (nReadCount > (nLength - nCount)) {
			nReadCount = (u_int32_t) (nLength - nCount);
		}
		
		io_read (pData, nReadCount, image, nSource);
#if 0	//suk
		// usual message pump
		if (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) {
			// PeekMessage has found a message--process it 
			if (msg.message != WM_CANCELLED) {
				TranslateMessage (&msg);	// Translate virt. key codes 
				DispatchMessage (&msg);	// Dispatch msg. to window 
			} else {
				AddToLog ("Cancelled - probably unusable now");
				delete pProgressBox;
				return false;
			}
		}
#endif
		
		DiscWriteDirect (image, nDest, pData, nReadCount);
		
		nDest = nDest + nReadCount;
		nSource = nSource + nReadCount;
		nBlocks++;
		nCount = nCount + nReadCount;
	}
	//	delete pProgressBox;
	
	
	free (pData);
	return true;
}
////////////////////////////////////////////////////////////////////
// Save a decrypted partition out                                 //
////////////////////////////////////////////////////////////////////
bool CWIIDisc::SaveDecryptedPartition (string csName, image_file * image,
									   u_int32_t nPartition)
{
	// now open a progress bar
	u_int64_t nStartOffset;
	u_int64_t nLength;
	u_int64_t nOffset = 0;
	
	u_int8_t *pData;
	FILE *fOut;
	u_int32_t nBlockCount = 0;
	
	fOut = fopen (csName.c_str (), "wb");
	if (NULL == fOut) {
		// Error
		return false;
	}
	
	// now get the parameters we need to save
	nStartOffset = image->parts[nPartition].offset;
	nLength = image->parts[nPartition].data_size;
	
	pData = (u_int8_t *) malloc (0x20000);
	
	// save the first 0x20000 bytes direct as thats the partition.bin
	io_read (pData, 0x20000, image, nStartOffset);
	fwrite (pData, 1, 0x20000, fOut);
#if 0	//suk
	CProgressBox *pProgressBox = new CProgressBox (m_pParent);
	
	pProgressBox->Create (IDD_PROGRESSBOX);
	
	pProgressBox->ShowWindow (SW_SHOW);
	pProgressBox->SetRange (0, (u_int32_t) (nLength / 0x8000));
	
	pProgressBox->SetPosition (0);
	
	pProgressBox->SetWindowMessage ("Saving partition");
#endif
	// then step through the clusters
	nStartOffset = 0;
	for (u_int64_t nCount = 0; nCount < nLength; nCount = nCount + 0x8000) {
		//		pProgressBox->SetPosition (nBlockCount);
		io_read_part (pData, 0x7c00, image, nPartition, nOffset);
#if 0	//suk
		// usual message pump
		if (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) {
			// PeekMessage has found a message--process it 
			if (msg.message != WM_CANCELLED) {
				TranslateMessage (&msg);	// Translate virt. key codes 
				DispatchMessage (&msg);	// Dispatch msg. to window 
			} else {
				AddToLog ("Cancelled save");
				delete pProgressBox;
				free (pData);
				return false;
			}
		}
#endif
		fwrite (pData, 1, 0x7c00, fOut);
		nOffset = nOffset + 0x7c00;
		nBlockCount++;
	}
	
	//	delete pProgressBox;
	fclose (fOut);
	free (pData);
	return true;
}
////////////////////////////////////////////////////////
// Load a decrypted partition of data and fill the    //
// partition up with it                               //
////////////////////////////////////////////////////////
bool CWIIDisc::LoadDecryptedPartition (string csName, image_file * image,
									   u_int32_t nPartition)
{
	
	// now open a progress bar
	u_int64_t nStartOffset;
	u_int64_t nLength;
	
	u_int8_t *pData;
	FILE *fIn;
//s	u_int32_t nBlockCount = 0;
	
	u_int64_t nFileSize;
	
	fIn = fopen (csName.c_str (), "rb");
	if (NULL == fIn) {
		// Error
		return false;
	}
	
	// now get the parameters we need to save
	nStartOffset = image->parts[nPartition].offset;
	nLength = image->parts[nPartition].data_size;
	
	
	// now check the size of the file we are trying to read in
	nFileSize = my_fseek (fIn, 0L, SEEK_END);
	my_fseek (fIn, 0L, SEEK_SET);
	
	// now account for the partition header and the actual number of clusters of data
	if (nLength < (((nFileSize - 0x20000) / 0x7c00) * 0x8000)) {
		// not enough space for the partition load
		AfxMessageBox ("File too big to load into partition");
		fclose (fIn);
		return false;
	}
	
	pData = (u_int8_t *) malloc (0x20000);
	
	// save the first 0x20000 bytes direct as thats the partition.bin
	fread (pData, 1, 0x20000, fIn);
	DiscWriteDirect (image, nStartOffset, pData, 0x20000);
	
	// now really need to parse the header for the new partition as the
	// title key etc. will be different
	get_partitions (image);
	
	// now write the file out
	if (false ==
	    wii_write_data_file (image, nPartition, 0, nFileSize - 0x20000,
							 NULL, fIn)) {
		fclose (fIn);
		free (pData);
		return false;
	}
	
	
	fclose (fIn);
	free (pData);
	return true;
}

//////////////////////////////////////////////////////////////////////
// Shrink the data up in the partition                              //
// we just move the data up in the partition by finding out where   //
// the free space in the middle is and copying the data down from   //
// above it                                                         //
// we then update the fst.bin to take off however much we did       //
// to save time we copy from one cluster group star to another as   //
// then we don't need to recalculate the sha tables, just copy them //
// we also need to sign at the end                                  //
//////////////////////////////////////////////////////////////////////

bool CWIIDisc::DoPartitionShrink (image_file * image, u_int32_t nPartition)
{
	
	u_int64_t nClusterSource;
	u_int64_t nSourceDataOffset;
	u_int64_t nClusterDestination;
	u_int64_t nDestinationDataOffset;
	
	u_int64_t nSourceClusterGroup;
	u_int64_t nDestClusterGroup;
	u_int64_t nClusterGroups;
	
	u_int32_t nDifference;
	
	u_int64_t i;
	
	// allocate space for the fst.bin
	
	u_int8_t *pFST = (u_int8_t *)
	malloc ((u_int32_t)
			(image->parts[nPartition].header.fst_size));
	
	// allocate space for the data size (as we modify it
	u_int8_t nDataSize[4];
	
	// read the fst.bin and data size files
	io_read_part (pFST,
				  (u_int32_t) (image->parts[nPartition].header.fst_size),
				  image, nPartition,
				  image->parts[nPartition].header.fst_offset);
	io_read (nDataSize, 0x0004, image,
			 image->parts[nPartition].offset + 0x2bC);
	
	// find the first empty block from main.dol onwards
	nClusterDestination =
	FindFirstData (image->parts[nPartition].offset +
			       image->parts[nPartition].data_offset +
			       image->parts[nPartition].header.dol_offset,
			       image->parts[nPartition].data_size -
			       image->parts[nPartition].header.dol_offset,
			       false);
	
	// check for error condition
	if (0 == nClusterDestination) {
		AfxMessageBox
		("Unable to find space to remove or main.dol incorrect");
		free (pFST);
		return false;
	}
	// change it to a higher cluster boundary
	nDestinationDataOffset =
	nClusterDestination - (image->parts[nPartition].offset +
						   image->parts[nPartition].data_offset);
	nDestClusterGroup =
	(nDestinationDataOffset / (0x8000 * NB_CLUSTER_GROUP)) + 1;
	nDestinationDataOffset =
	nDestClusterGroup * NB_CLUSTER_GROUP * 0x7c00;
	nClusterDestination =
	nDestClusterGroup * NB_CLUSTER_GROUP * 0x8000 +
	image->parts[nPartition].offset +
	image->parts[nPartition].data_offset;
	
	// now find the start of the data
	nClusterSource = FindFirstData (nClusterDestination,
									image->parts[nPartition].data_size -
									nDestClusterGroup * NB_CLUSTER_GROUP *
									0x8000, true);
	
	if (0 == nClusterSource) {
		AfxMessageBox
		("Unable to find space to remove or main.dol incorrect");
		free (pFST);
		return false;
		
	}
	// change to a lower cluster boundary
	nSourceDataOffset =
	nClusterSource - (image->parts[nPartition].offset +
					  image->parts[nPartition].data_offset);
	nSourceClusterGroup =
	((nSourceDataOffset / 0x8000) / NB_CLUSTER_GROUP);
	nSourceDataOffset = nSourceClusterGroup * NB_CLUSTER_GROUP * 0x7c00;
	nClusterSource =
	nSourceClusterGroup * NB_CLUSTER_GROUP * 0x8000 +
	image->parts[nPartition].offset +
	image->parts[nPartition].data_offset;
	
	// calculate number we need to copy
	nClusterGroups =
	(image->parts[nPartition].data_size /
	 (0x8000 * NB_CLUSTER_GROUP)) - nSourceClusterGroup;
	
	
	// check to see if it's worth doing
	
	if (nSourceClusterGroup == nDestClusterGroup) {
		// same source/dest so pointless
		AfxMessageBox
		("Pointless doing it as source and dest are the same group");
		free (pFST);
		return false;
	}
	// read the h3 table
	io_read (h3, SIZE_H3, image,
			 image->parts[nPartition].offset +
			 image->parts[nPartition].h3_offset);
	
	// move the data down
	CopyDiscDataDirect (image, nPartition, nClusterSource,
						nClusterDestination,
						nClusterGroups * 0x8000 * NB_CLUSTER_GROUP);
	
	// update the h3 and save out as the write file use it
	for (i = 0; i < nClusterGroups; i++) {
		memcpy (&h3[(nDestClusterGroup + i) * 0x14],
				&h3[(nSourceClusterGroup + i) * 0x14], 0x14);
	}
	DiscWriteDirect (image,
					 image->parts[nPartition].offset +
					 image->parts[nPartition].h3_offset, h3, SIZE_H3);
	
	// now update the fst table entries
	nDifference =
	(u_int32_t) (((nSourceClusterGroup -
			       nDestClusterGroup) * NB_CLUSTER_GROUP *
			      0x7c00) >> 2);
	
	u_int32_t nFSTEntries = be32 (pFST + 8);
	u_int32_t nTempOffset;
	
	for (i = 0; i < nFSTEntries; i++) {
		// if a file
		if (pFST[i * 0x0C] == 0x00) {
			// get current offset
			nTempOffset = be32 (pFST + i * 0x0c + 4);
			// take off difference
			nTempOffset = nTempOffset - nDifference;
			// save entry
			Write32 (pFST + i * 0x0c + 4, nTempOffset);
		}
	}
	// save the fst.bin
	wii_write_data_file (image, nPartition,
						 image->parts[nPartition].header.fst_offset,
						 image->parts[nPartition].header.fst_size, pFST);
	
	// update the data size in boot.bin
	u_int32_t nSize = be32 (nDataSize);
	
	nDifference =
	(u_int32_t) (((nSourceClusterGroup -
			       nDestClusterGroup) * NB_CLUSTER_GROUP *
			      0x8000) >> 2);
	
	Write32 (nDataSize, nSize - nDifference);
	
	// save it
	
	DiscWriteDirect (image, image->parts[nPartition].offset + 0x2bc,
					 nDataSize, 4);
	// sign it
	
	wii_trucha_signing (image, nPartition);
	
	free (pFST);
	// free the memory
	return true;
}

//////////////////////////////////////////////////////////////////////
// Search for the first block of data that is marked as either used //
// or unused                                                        //
//////////////////////////////////////////////////////////////////////
u_int64_t CWIIDisc::FindFirstData (u_int64_t nStartOffset, u_int64_t nLength,
								   bool bUsed)
{
	u_int64_t nBlock = nStartOffset / 0x8000;
	u_int64_t nEndBlock = (nStartOffset + nLength - 1) / 0x8000;
	
	while (nBlock < nEndBlock) {
		if (true == bUsed) {
			if (1 == pFreeTable[nBlock]) {
				return nBlock * 0x8000;
			}
		} else {
			if (0 == pFreeTable[nBlock]) {
				return nBlock * 0x8000;
			}
		}
		nBlock++;
	}
	return 0;
	
}


///////////////////////////////////////////////////////////////
// Save all the files in a partition to the passed directory //
///////////////////////////////////////////////////////////////
bool CWIIDisc::ExtractPartitionFiles (image_file * image,
									  u_int32_t nPartition,
									  const char * cDirPathName)
{
	u_int8_t *fst;
	u_int32_t nfiles;
	
	// get the working directory
#define _MAX_PATH 4096		//suk
	char buffer[_MAX_PATH];
	
	// Get the current working directory:
	if (getcwd (buffer, _MAX_PATH) == NULL) {
		AddToLog ("_getcwd error");
		return false;
	}
	// change to the new directory
	
	chdir (cDirPathName);
	fst = (u_int8_t *) (malloc ((u_int32_t)
								(image->parts[nPartition].header.
								 fst_size)));
	
	if (io_read_part
	    (fst, (u_int32_t) (image->parts[nPartition].header.fst_size),
	     image, nPartition,
	     image->parts[nPartition].header.fst_offset) !=
	    image->parts[nPartition].header.fst_size) {
		AfxMessageBox ("fst.bin");
		free (fst);
		return false;
	}
	
	nfiles = be32 (fst + 8);
#if 0
	// create a progress window and pass the value to the parse function
	// where the message pump will run
	CProgressBox *pProgressBox;
	
	pProgressBox = new CProgressBox (m_pParent);
	
	pProgressBox->Create (IDD_PROGRESSBOX);
	
	pProgressBox->ShowWindow (SW_SHOW);
	pProgressBox->SetRange (0, nfiles);
	
	pProgressBox->SetPosition (0);
	
	pProgressBox->
	SetWindowMessage ("Saving partition of data. Please wait");
#endif
	AddToLog ("Saving partition of data. Please wait");
	u_int32_t nFiles =
	parse_fst_and_save (fst, (char *) (fst + 12 * nfiles), 0,
						image, nPartition);
	
	//	delete pProgressBox;
	free (fst);
	//change back to the working directory
	chdir (buffer);
	
	if (nFiles != nfiles) {
		AddToLog ("Error writing files out");
		return false;
	}
	return true;
}
