#include "../libmffs/mffs.h"
#include "mkmffs.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
error(const char *msg)
{
	fputs(msg, stderr);
	exit(1);
}

/* calculate and return the number of blocks on the device */
size_t
blockdev_calc_blocks(FILE *dev, uint32_t sector_size)
{
	size_t pos_start, pos_end;

	pos_start = ftell(dev);

	fseek(dev, 0, SEEK_END);
	pos_end = ftell(dev);
	fseek(dev, pos_start, SEEK_SET);

	/* 	fprintf(stderr, "pos_start: %lu\npos_end: %lu\ncurrent: %lu\n",
			pos_start, pos_end, ftell(dev)); */
	return (size_t)((pos_end - pos_start) / sector_size);
}

/* create the allocation map and return the last claimed sector */
uint32_t
blockdev_create_allocation_map(size_t available_blocks, size_t sector_size,
			       size_t sector_dl, FILE *dev)
{
	uint32_t map_size, latest_allocation;
	uint8_t	 mask;
	size_t	 pos_current, needed_blocks, unused_blocks;
	uint8_t	 sector_buffer;

	pos_current = ftell(dev);

	map_size = (uint32_t)ceil(((float)available_blocks / 8.f));

	/* calculate how much blocks we need to save our bitmap */
	needed_blocks = (size_t)ceil((float)map_size / (sector_size));

	latest_allocation = needed_blocks + 1;

	unused_blocks = available_blocks - needed_blocks;

	fprintf(stderr,
		"=> Create Allocation Map\n==> Size: %X\n==> Needed Blocks: "
		"%zu\n==> Unused "
		"Blocks: %zu\n",
		map_size, needed_blocks, unused_blocks);

	/* seek to block start */
	fseek(dev, sector_dl * sector_size, SEEK_SET);

	fwrite(&map_size, sizeof(map_size), 1, dev);

	/* write first byte */
	if (needed_blocks >= 6) {
		needed_blocks -= 6;
		sector_buffer = 0xFF;
	} else {
		mask = 0b00100000;
		sector_buffer = 0b11000000;
		while (needed_blocks != 0) {
			needed_blocks--;
			sector_buffer |= mask;
			mask >>= 1;
		}
	}
	fwrite(&sector_buffer, sizeof(sector_buffer), 1, dev);

	if (!needed_blocks) {
		goto write_unused;
	}

	sector_buffer = 0xFF;
	while (needed_blocks >= (sizeof(sector_buffer) * 8)) {
		needed_blocks -= (sizeof(sector_buffer) * 8);

		fwrite(&sector_buffer, sizeof(sector_buffer), 1, dev);
	}

	if (!needed_blocks) {
		goto write_unused;
	}

	mask = 0b10000000;
	sector_buffer = 0;
	while (needed_blocks--) {
		sector_buffer |= mask;
		mask >>= 1;
	}

	fwrite(&sector_buffer, sizeof(sector_buffer), 1, dev);

write_unused:

	sector_buffer = 0;
	while (unused_blocks--) {
		fwrite(&sector_buffer, sizeof(sector_buffer), 1, dev);
	}

	fseek(dev, pos_current, SEEK_SET);

	return latest_allocation;
}

void
blockdev_format(const char *blockdev, const char *disk_name,
		uint32_t sector_size)
{
	FILE	     *dev;
	char	      buf;
	size_t	      blocks, root_files;
	MffsRootEntry root_entry;

	if (sector_size != 512 && sector_size != 1024 && sector_size != 2048 &&
	    sector_size != 4096) {
		error(
		    "Invalid sector size! Valid are:\n512\n1024\n2048\n4096\n");
	}

	dev = fopen(blockdev, "rb+");
	if (!dev) {
		error("couldn't open device");
	}

	/* skip first sector (MBR lies there) */
	fseek(dev, sector_size, SEEK_SET);

	blocks = blockdev_calc_blocks(dev, sector_size);
	fprintf(stderr, "=> Blocks: %zu\n", blocks);
	if (blocks < 5) {
		error("not enough blocks to make something useful tbh\n");
	}

	root_entry.magic[0] = 'F';
	root_entry.magic[1] = 'C';
	root_entry.magic[2] = 'K';

	root_entry.lock = 0;

	switch (sector_size) {
	case 512:
		root_entry.sector_size = MFFS_SECTOR_SIZE_512B;
		break;
	case 1024:
		root_entry.sector_size = MFFS_SECTOR_SIZE_1K;
		break;
	case 2048:
		root_entry.sector_size = MFFS_SECTOR_SIZE_2K;
		break;
	case 4096:
		root_entry.sector_size = MFFS_SECTOR_SIZE_4K;
		break;
	}

	memset(root_entry.name, 0, sizeof(root_entry.name));
	strncpy(root_entry.name, disk_name, sizeof(root_entry.name));

	/* root dir eats 1 sector after creation */
	root_entry.size = 1;

	root_entry.sector_allocation_map = 2;
	root_entry.sector_latest_allocation =
	    blockdev_create_allocation_map(blocks, sector_size, 2, dev);

	fprintf(stderr, "=> Latest Allocation: %d\n",
		root_entry.sector_latest_allocation);

	fputs("=> Creating root entry\n", stderr);
	fwrite(&root_entry, sizeof(root_entry), 1, dev);

	/* Calculate how many file entries the root dir could take and set all
	 * file names to 0.
	 * This wil make them invalid/free.
	 */
	buf = 0;
	root_files = mffs_root_fileentry_capacity(sector_size);
	fprintf(stderr, "=> Reserving %zu root files\n", root_files);
	while (root_files--) {
		fwrite(&buf, sizeof(buf), 1, dev);
		fseek(dev, sizeof(MffsFileEntry) - 1, SEEK_CUR);
	}

	fflush(dev);
	fclose(dev);
}
