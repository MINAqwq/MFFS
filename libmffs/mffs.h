#ifndef LIBMFFS_H
#define LIBMFFS_H

#include <stddef.h>
#include <stdint.h>

#define MFFS_SECTOR_SIZE_512B 0
#define MFFS_SECTOR_SIZE_1K   1
#define MFFS_SECTOR_SIZE_2K   2
#define MFFS_SECTOR_SIZE_4K   3

typedef struct __attribute__((packed)) {
	uint32_t size;
	uint8_t	 eight_sectors[0];
} MffsAllocationMap;

typedef struct __attribute__((packed)) {
	uint32_t sector_before; /* The data sector before the current (0 if
				   current is the first) */
	uint32_t sector_next;	/* The data sector holding the data that comes
				   next (0 if there are no more) */
	char data[0];		/* [variable length] data (SECTOR_SIZE -
				   sizeof(MffsSectorMarker)) */
} MffsSectorMarker;

typedef struct __attribute__((packed)) {
	uint8_t is_directory : 1; /* entry contains file entries */
	uint8_t is_hidden : 1;	  /* entry is marked as hidden */
	uint8_t is_link_soft : 1; /* data links to another entry */
	uint8_t is_link_hard : 1; /* links to another entry on driver level */
	uint8_t is_code : 1;	  /* data is executable code */
	uint8_t is_system : 1;	  /* modifying should need higher priviliges */
	uint8_t _reserved0 : 1;
	uint8_t _reserved1 : 1;
} MffsFileEntryAttributes;

typedef struct __attribute__((packed)) {
	char			name[47]; /* file name */
	MffsFileEntryAttributes attributes;
	uint32_t		size; /* number of reserved sectors for data */
	uint32_t		sector_start;  /* first data sector number */
	uint32_t		time_created;  /* file creation time (unix) */
	uint32_t		time_modified; /* last modify time (unix) */
} MffsFileEntry;

typedef struct __attribute__((packed)) {
	char	 magic[3];    /* magic bytes */
	uint8_t	 sector_size; /* sector size */
	char	 name[15];    /* disk name */
	uint8_t	 lock;	      /* fs lock preventing multiple write actions */
	uint32_t size;	      /* number of sectors the root dir takes */
	uint32_t sector_allocation_map;	   /* allocation map sector number */
	uint32_t sector_latest_allocation; /* latest allocated sector number */
	MffsFileEntry entries[0];	   /* [variable length] file entries */
} MffsRootEntry;

static size_t
mffs_root_fileentry_capacity(uint8_t sector_size)
{
	switch (sector_size) {
	case MFFS_SECTOR_SIZE_512B:
		return (512 - sizeof(MffsRootEntry)) / sizeof(MffsFileEntry);
	case MFFS_SECTOR_SIZE_1K:
		return (1024 - sizeof(MffsRootEntry)) / sizeof(MffsFileEntry);
	case MFFS_SECTOR_SIZE_2K:
		return (2048 - sizeof(MffsRootEntry)) / sizeof(MffsFileEntry);
	case MFFS_SECTOR_SIZE_4K:
		return (4906 - sizeof(MffsRootEntry)) / sizeof(MffsFileEntry);
	default:
		return 0;
	}
}

static size_t
mffs_datasector_fileentry_capacity(uint8_t sector_size)
{
	switch (sector_size) {
	case MFFS_SECTOR_SIZE_512B:
		return (512 - sizeof(MffsSectorMarker)) / sizeof(MffsFileEntry);
	case MFFS_SECTOR_SIZE_1K:
		return (1024 - sizeof(MffsSectorMarker)) /
		       sizeof(MffsFileEntry);
	case MFFS_SECTOR_SIZE_2K:
		return (2048 - sizeof(MffsSectorMarker)) /
		       sizeof(MffsFileEntry);
	case MFFS_SECTOR_SIZE_4K:
		return (4906 - sizeof(MffsSectorMarker)) /
		       sizeof(MffsFileEntry);
	default:
		return 0;
	}
}

static size_t
mffs_sector_size_from_enum(uint8_t sector_size)
{
	switch (sector_size) {
	case MFFS_SECTOR_SIZE_512B:
		return 512;
	case MFFS_SECTOR_SIZE_1K:
		return 1024;
	case MFFS_SECTOR_SIZE_2K:
		return 2048;
	case MFFS_SECTOR_SIZE_4K:
		return 4906;
	default:
		return 0;
	}
}

#endif
