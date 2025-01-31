#pragma once

#include <stdint.h>
#include <assert.h>

constexpr int F11_BLOCK_SIZE = 512;

#define BLOCKDATASIZE(ds) ((ds + (F11_BLOCK_SIZE - 1)) / F11_BLOCK_SIZE)

//--------------------------------
// Home Block structure
// LBN 0 : Boot sector
// LBN 1 : Home Block

#define F11_HOME_VBN   (2)
#define F11_HOME_LBN   (F11_HOME_VBN - 1)

#define HM1_C_LEVEL1    (0401)
#define HM1_C_LEVEL2    (0402)

//--------------------------------
// Known file number
constexpr int F11_INDEXF_SYS = 1;
constexpr int F11_BITMAP_SYS = 2;
constexpr int F11_BADBLK_SYS = 3;
constexpr int F11_000000_SYS = 4;
constexpr int F11_CORIMG_SYS = 5;

constexpr uint8_t F11_HEADER_FID_OFFSET = 0x17;
constexpr uint8_t F11_HEADER_MAP_OFFSET = 0x2E;

// World protection (Full access to SYS,OWN,GRP)
constexpr uint16_t F11_DEFAULT_FILE_PROTECTION = 0xE000;
// Default Owner
constexpr uint16_t F11_DEFAULT_FILE_OWNER = (001 << 8) + 002; // [001,002]

constexpr int NAM_SIZE = 3; // Size of file name in words
constexpr int EXT_SIZE = 1; // Size of file extension in words

constexpr int MAX_FILE_VERSION = 017777; // TODO : VERIFY THAT

#define EOL ("\r\n")

//--------------------------------
#pragma pack(push, 1)

typedef struct _F11_HomeBlock
{
	uint16_t hm1_w_ibmapsize;
	uint16_t hm1_w_ibmaplbn_hi;
	uint16_t hm1_w_ibmaplbn_lo;
	uint16_t hm1_w_maxfiles;
	uint16_t hm1_w_cluster;
	uint16_t hm1_w_devtype;
	uint16_t hm1_w_structlev;
	uint8_t  hm1_t_volname[12];
	uint8_t  hm1_b_fill_1[4];
	uint16_t hm1_w_volowner;
	uint16_t hm1_w_protect;
	uint16_t hm1_w_volchar;
	uint16_t hm1_w_deffileprot;
	uint16_t hm1_b_fill_2[3];
	uint8_t  hm1_b_window;
	uint8_t  hm1_b_extend;
	uint8_t  hm1_b_dirlimit;
	uint8_t  hm1_t_lastrev[7];
	uint16_t hm1_w_modifcnt;
	uint16_t hm1_w_fill_3;
	uint16_t hm1_w_checksum1;
	uint8_t  hm1_t_credate[14];
	uint8_t  hm1_b_fill_4[382];
	uint32_t hm1_l_serialnum;
	uint8_t  hm1_b_fill_5[12];
	uint8_t  hm1_t_volname2[12];
	uint8_t  hm1_t_ownername[12];
	uint8_t  hm1_t_format[12];
	uint16_t hm1_t_fill_6;
	uint16_t hm1_w_checksum2;
} F11_HomeBlock_t;

//---------------------------------------------------------------------
// File Header

typedef struct _F11_FileHeader
{
	uint8_t  fh1_b_idoffset;
	uint8_t  fh1_b_mpoffset;
	uint16_t fh1_w_fid_num;
	uint16_t fh1_w_fid_seq;
	uint16_t fh1_w_struclev;
	uint16_t fh1_w_fileowner;
	uint16_t fh1_w_fileprot;
	uint8_t  fh1_b_userchar;
	uint8_t  fh1_b_syschar;
	uint16_t fh1_w_ufat;
	uint8_t  fh1_b_fill_1[494];
	uint16_t fh1_w_checksum;
} F11_FileHeader_t;

//--------------------------------
// User Attributes Area

// User characteristics (ref: 3.4.1.8)
#define uc_nid  (0x02)
#define uc_wbc  (0x04)
#define uc_rck  (0x08)
#define uc_wck  (0x10)
#define uc_cnb  (0x20)
#define uc_dlk  (0x40)
#define uc_con  (0x80)

// System characteristics (ref: 3.4.1.8)
#define sc_spl	(0x10)
#define sc_dir	(0x20)
#define sc_bad	(0x40)
#define sc_mdl	(0x80)

// Record Type (ref: 6.1.1)
#define rt_fix  (0x01)
#define rt_vlr  (0x02)
#define rt_seq  (0x04)

// Record Attributes (ref: 6.1.2)
#define ra_ftn  (0x01) // Fortran carriage Control
#define ra_cr   (0x02) // Carriage Control ; ra_cr and ra_ftn are mutually exclusive
#define ra_prn  (0x04)


typedef struct _F11_UserAttrArea {
	uint8_t		ufcs_rectype;
	uint8_t		ufcs_recattr;
	uint16_t	ufcs_recsize;
	uint16_t	ufcs_highvbn_hi;
	uint16_t	ufcs_highvbn_lo;
	uint16_t	ufcs_eofblck_hi;
	uint16_t	ufcs_eofblck_lo;
	uint16_t	ufcs_ffbyte;
} F11_UserAttrArea_t;

//--------------------------------
// Ident Area (size: 46 bytes)
// Ref: 3.4.2

typedef struct f11_IdentArea {
	uint16_t	filename[NAM_SIZE];	// File name
	uint16_t	filetype[EXT_SIZE];    // Revision Number
	uint16_t	version;
	uint16_t	revision;
	uint8_t		revision_date[7];
	uint8_t		revision_time[6];
	uint8_t		creation_date[7];
	uint8_t		creation_time[6];
	uint8_t		expiration_date[7];
	uint8_t		reserved;
} F11_IdentArea_t;


//--------------------------------
// Retrieval Pointers

typedef struct F11_format1 {
	uint8_t		hi_lbn;
	uint8_t		blk_count;
	uint16_t	lo_lbn;
} F11_Format1_t;

typedef struct F11_format2 {
	uint16_t	blk_count;
	uint16_t	lbn;
} F11_Format2_t;

typedef struct F11_format3 {
	uint16_t	blk_count;
	uint16_t	hi_lbn;
	uint16_t	lo_lbn;
} F11_Format3_t;

//--------------------------------
// Map Area
// Ref: 3.4.3

typedef union PtrsFormat {
	F11_Format1_t fm1;
	F11_Format2_t fm2;
	F11_Format3_t fm3;
} PtrsFormat_t;

typedef struct f11_MapArea {
	uint8_t		ext_SegNumber;
	uint8_t		ext_RelVolNo;
	uint16_t	ext_FileNumber;
	uint16_t	ext_FileSeqNumber;
	uint8_t		CTSZ;
	uint8_t		LBSZ;
	uint8_t		USE;
	uint8_t		MAX;
	PtrsFormat_t pointers;
} F11_MapArea_t;

//---------------------------------------------------------------------
// Directory Structure (ref: 4.2)

typedef struct DirectoryRecord {
	uint16_t	fileNumber;		// File Number
	uint16_t	fileSeq;		// File Sequence Number
	uint16_t	fileRVN;		// Relative Volume Number (not used)
	uint16_t	fileName[NAM_SIZE];	// File name (9 characters encoded in Radix50)
	uint16_t	fileType[EXT_SIZE]; // File extension (3 characters encoded in Radix50)
	uint16_t	version;		// File Version number
} DirectoryRecord_t;

//---------------------------------------------------------------------
// Storage Control Block (ref: 5.2.1)

typedef struct _SCB_DataFreeBlks {
	uint16_t	nbFreeBlks;			// Number of free blocks in nth block
	uint16_t	firstFreeBlockPtr;	// Free block pointer in nth bitmap block
} SCB_DataFreeBlks_t;

typedef struct _SCB_Data_large {
	uint16_t	unitSizeLogBlks_hi;	// size of unit in logical blocks
	uint16_t	unitSizeLogBlks_lo;	// size of unit in logical blocks
	uint8_t		unused[246];		// not used (zero)
} SCB_Data_large_t;

#define NB_SCB_RECORDS  ((sizeof(SCB_Data_large_t) - sizeof(uint32_t)) / sizeof(SCB_DataFreeBlks_t))

typedef struct _SCB_Data_small {
	SCB_DataFreeBlks_t free_blks[NB_SCB_RECORDS];
	uint32_t	unitSizeLogBlks;	// size of unit in logical blocks
} SCB_Data_small_t;

typedef struct _SCB {
	uint8_t		unused[3];		// should be 0
	uint8_t		nbBitmapBlks;	// Number of storage bitmap blocks
	union {
		uint16_t smallUnit;
		SCB_Data_large_t largeUnit;
	} blocks;
} SCB_t;

// Max number of words for pointer area
constexpr int MAX_POINTERS = (F11_BLOCK_SIZE - (F11_HEADER_MAP_OFFSET * 2) - sizeof(uint16_t) - (sizeof(F11_MapArea_t) - sizeof(PtrsFormat_t))) / 2;
// Pointer size in words (pointer format 3 not supported)
constexpr int POINTER_SIZE = 2;
// Max number of pointers in a header
constexpr int NB_POINTERS_PER_HEADER = (F11_BLOCK_SIZE - (F11_HEADER_MAP_OFFSET * 2) - sizeof(uint16_t) - (sizeof(F11_MapArea_t) - sizeof(PtrsFormat_t))) / 4;
// Each pointer can address a maximum of 256 blocks
constexpr int BLOCKS_PER_POINTER = 256;

#pragma pack(pop)
