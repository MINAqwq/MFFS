#include "mffscli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint8_t g_sector_size = 0;

void
driver_configure(uint8_t sector_size)
{
	g_sector_size = sector_size;
}

void
driver_write_lock_state(uint8_t lock_state, FILE *dev)
{
	MffsRootEntry root;
	size_t	      old_pos, lock_offset;

	old_pos = ftell(dev);

	lock_offset = (((size_t)&root.lock) - ((size_t)&root));

	/* seek to lock state */
	fseek(dev, mffs_sector_size_from_enum(g_sector_size) + lock_offset,
	      SEEK_SET);

	fwrite(&lock_state, sizeof(lock_state), 1, dev);

	fseek(dev, old_pos, SEEK_SET);
}

uint8_t
driver_read_root(MffsRootEntry *root_buffer, FILE *dev)
{
	size_t old_pos;

	old_pos = ftell(dev);

	/* seek to second/root sector */
	fseek(dev, mffs_sector_size_from_enum(g_sector_size), SEEK_SET);

	(void)fread(root_buffer, sizeof(*root_buffer), 1, dev);

	fseek(dev, old_pos, SEEK_SET);

	return mffs_is_magic_valid(root_buffer->magic);
}

void
driver_read_root_files(MffsFileEntry *file_entries, FILE *dev)
{
	size_t old_pos;

	old_pos = ftell(dev);

	/* seek to root file entry start */
	fseek(dev,
	      mffs_sector_size_from_enum(g_sector_size) + sizeof(MffsRootEntry),
	      SEEK_SET);

	(void)fread(file_entries,
		    sizeof(*file_entries) *
			mffs_root_fileentry_capacity(g_sector_size),
		    1, dev);

	fseek(dev, old_pos, SEEK_SET);
}

size_t
driver_root_find_free_file_place(MffsFileEntry *file_entries)
{
	size_t root_capacity = mffs_root_fileentry_capacity(g_sector_size);

	while (root_capacity--) {
		if (file_entries[root_capacity].name[0] == 0) {
			return root_capacity;
		}
	}

	return -1;
}

/* find a file in a file entry and return it's index */
int64_t
driver_find_file_in_file_entry(MffsFileEntry *entries, const char *name,
			       size_t sector_capacity)
{
	size_t	       i;
	MffsFileEntry *tmp;

	for (i = 0; i < sector_capacity; i++) {
		tmp = &entries[i];

		/* 0 at name[0] means there is no file */
		if (!tmp->name[0]) {
			continue;
		}

		if (strncmp(tmp->name, name, sizeof(tmp->name))) {
			tmp = NULL;
			continue;
		}

		return i;
	}

	return -1;
}

uint8_t
driver_read_file_entry(const char *virtual_path, MffsFileEntry *file_buffer,
		       size_t *dev_pos_buffer, FILE *dev)
{
	const size_t root_file_capacity =
	    mffs_root_fileentry_capacity(g_sector_size);

	const size_t sector_size_bytes =
	    mffs_sector_size_from_enum(g_sector_size);

	MffsRootEntry	  root;
	MffsFileEntry	  file;
	MffsFileEntry	 *files_buffer;
	MffsSectorMarker *sector_marker;

	char  *path_list;
	char  *path_list_copy;
	size_t path_len, path_list_count, old_pos;

	uint32_t next_sector;

	int64_t search_res;

	if (!driver_read_root(&root, dev)) {
		return 0;
	}

	path_len = strlen(virtual_path);
	if (path_len < 2) {
		return 0;
	}

	path_list = malloc(path_len);
	path_list_copy = path_list;
	if (!path_list) {
		return 0;
	}

	strcpy(path_list_copy, virtual_path);

	/* replace all 0x2f (/) with 0x00 to build some kind of dynamic list we
	 * can seek through */
	path_list_count = str_replace(path_list, '/', 0);
	if (!path_list_count || path_list[0] != 0) {
		goto err_free;
	}

	/* skip first 0 */
	path_list++;

	/* first read root entry files */
	path_list_count--;

	files_buffer = malloc(sizeof(*files_buffer) * root_file_capacity);
	if (!files_buffer) {
		goto err_free;
	}

	driver_read_root_files(files_buffer, dev);

	search_res = driver_find_file_in_file_entry(files_buffer, path_list,
						    root_file_capacity);

	if (search_res == -1) {
		free(files_buffer);
		goto err_free;
	}

	/* copy file entry onto stack */
	memcpy(&file, &files_buffer[search_res], sizeof(file));

	free(files_buffer);

	if (!path_list_count) {
		/* here we know it's from the root so we can calculate that
		 * bitch ass position */
		*dev_pos_buffer = sector_size_bytes + sizeof(root) +
				  (sizeof(*file_buffer) * search_res);
		memcpy(file_buffer, &file, sizeof(*file_buffer));
		free(path_list_copy);
		return 0;
	}

	if (!file.attributes.is_directory) {
		goto err_free;
	}

	old_pos = ftell(dev);

	while (path_list_count--) {
		while (*path_list) {
			path_list++;
		}

		path_list++;

		/* skip a zero byte, that should be a trailing slash */
		if (!(*path_list)) {
			path_list++;
			continue;
		}

		sector_marker = NULL;
		next_sector = file.sector_start;
		do {
			free(sector_marker);
			sector_marker = malloc(sector_size_bytes);
			if (!sector_marker) {
				goto err_rewind;
			}

			fseek(dev, sector_size_bytes * next_sector, SEEK_SET);

			(void)fread(sector_marker, sector_size_bytes, 1, dev);

			/* seek back to sector start for position calculation
			 * later */
			fseek(dev, sector_size_bytes * file.sector_start,
			      SEEK_SET);

			files_buffer = (MffsFileEntry *)&sector_marker->data;

			search_res = driver_find_file_in_file_entry(
			    files_buffer, path_list,
			    mffs_datasector_fileentry_capacity(g_sector_size));

			if (search_res == -1) {
				next_sector = sector_marker->sector_next;
				continue;
			}

			/* file found */

			memcpy(&file, &files_buffer[search_res], sizeof(file));
			*dev_pos_buffer = sector_size_bytes +
					  sizeof(*sector_marker) +
					  (sizeof(*file_buffer) * search_res);
			break;
		} while (sector_marker->sector_next != 0);

		free(sector_marker);

		if (search_res == -1) {
			goto err_rewind;
		}
	}

	memcpy(file_buffer, &file, sizeof(*file_buffer));

	fseek(dev, old_pos, SEEK_SET);
	free(path_list_copy);
	return 1;

err_rewind:
	fseek(dev, old_pos, SEEK_SET);
err_free:
	free(path_list_copy);
	return 0;
}

uint8_t
driver_file_create(const char *virtual_path, FILE *dev)
{
	MffsRootEntry root;
	size_t	      virtual_path_len;
	char	     *virtual_path_list;
	char	     *virtual_path_list_copy;
	size_t	      virtual_path_list_count;
	size_t	      old_pos, free_entry_index;

	MffsFileEntry *root_files;
	MffsFileEntry  new_entry;

	old_pos = ftell(dev);

	driver_read_root(&root, dev);

	if (root.lock) {
		fprintf(stderr, "shit is locked\n");
		return 0;
	}

	driver_write_lock_state(1, dev);

	new_entry.size = 1;
	new_entry.time_created = time(NULL);
	new_entry.time_modified = time(NULL);

	virtual_path_len = strlen(virtual_path);
	virtual_path_list = malloc(virtual_path_len + 1);
	virtual_path_list_copy = virtual_path_list;
	virtual_path_list[virtual_path_len] = 0;

	/* replace all 0x2f (/) with 0x00 to build some kind of dynamic list we
	 * can seek through */
	strcpy(virtual_path_list, virtual_path);
	virtual_path_list_count = str_replace(virtual_path_list, '/', 0);

	virtual_path_list++;
	if (virtual_path_list_count == 1) {
		fprintf(stderr, "we are in root\n");
		/* we are in the root directory */
		root_files =
		    malloc((sizeof(*root_files) *
			    mffs_root_fileentry_capacity(g_sector_size)));
		if (!root_files) {
			goto err_unlock;
		}

		driver_read_root_files(root_files, dev);
		free_entry_index = driver_root_find_free_file_place(root_files);
		fprintf(stderr, "next free root index: %zu\n",
			free_entry_index);
		free(root_files);

		if (free_entry_index == (size_t)-1) {
			goto err_unlock;
		}

		/* seek to postion we want to store our new entry */
		fseek(dev,
		      mffs_sector_size_from_enum(g_sector_size) +
			  sizeof(MffsRootEntry) +
			  (sizeof(*root_files) * free_entry_index),
		      SEEK_SET);

		fprintf(stderr, "seek to : %zu\n",
			mffs_sector_size_from_enum(g_sector_size) +
			    sizeof(MffsRootEntry) +
			    (sizeof(*root_files) + free_entry_index));

		strncpy(new_entry.name, virtual_path_list,
			sizeof(new_entry.name));
		new_entry.sector_start = driver_find_free_sector(dev);
		fprintf(stderr, "sector we gonna claim: %d\n",
			new_entry.sector_start);
		if (new_entry.sector_start == 0) {
			goto err_unlock;
		}

		goto finish;
	}

finish:
	driver_claim_sector(new_entry.sector_start, dev);
	(void)fwrite(&new_entry, sizeof(new_entry), 1, dev);

	fseek(dev, old_pos, SEEK_SET);

	free(virtual_path_list_copy);
	driver_write_lock_state(0, dev);
	return 1;

err_unlock:
	free(virtual_path_list_copy);
	driver_write_lock_state(0, dev);
	return 0;
}

/* Internal find_free_sector helper.
 * Read chunks from dev's allocation map and search for free sectors until
 * max_read is 0.
 * Return 0 on failure or if nothing was found at all.
 */
uint32_t
driver_find_free_sector_from_map(uint32_t max_read, uint32_t pre_res, FILE *dev)
{
	static const size_t read_block_size = 1024;

	uint8_t *buffer;
	uint32_t blocks_read, i, j;

	/* only load in chunks of the map instead of the whole */

	do {
		blocks_read =
		    (max_read >= read_block_size ? read_block_size : max_read);

		buffer = calloc(blocks_read, 1);
		if (!buffer) {
			return 0;
		}

		(void)fread(buffer, blocks_read, 1, dev);

		/* search through the bits one that is 0 */
		for (i = 0; i < blocks_read; i++) {
			for (j = 0; j < 8; j++) {
				if ((buffer[i] & (0x80 >> j)) == 0) {
					free(buffer);
					return pre_res + (i * 8) + j;
				}
			}
		}

		free(buffer);

		/* we read all blocks and found nothing, so we add them to res
		 */
		pre_res += blocks_read * 8;
	} while ((max_read -= blocks_read) != 0);

	return 0;
}

uint32_t
driver_find_free_sector(FILE *dev)
{
	size_t		  old_pos;
	MffsRootEntry	  root;
	MffsAllocationMap map;

	uint32_t res, target_byte;

	driver_read_root(&root, dev);

	old_pos = ftell(dev);

	fseek(dev,
	      root.sector_allocation_map *
		  mffs_sector_size_from_enum(g_sector_size),
	      SEEK_SET);

	(void)fread(&map, sizeof(map.size), 1, dev);

	/* seek to the byte containing the latest allocation */
	target_byte = (uint32_t)(root.sector_latest_allocation / 8);
	fseek(dev, target_byte, SEEK_CUR);

	map.size -= target_byte;

	/* this is the first sector we will check and because we can only add a
	 * relative number to the result before returning, we have to update res
	 * with every seek
	 */
	res = target_byte * 8;

	res = driver_find_free_sector_from_map(map.size, res, dev);
	if (res != 0) {
		goto found;
	}

	/* Since we got our sweet latest allocation, we don't start at the
	 * beginning. If we found nothing in the first run we can start at
	 * the beginning and search from 2 to `target_byte, where we saved
	 * out starting point`*/
	fseek(dev,
	      root.sector_allocation_map *
		      mffs_sector_size_from_enum(g_sector_size) +
		  sizeof(map.size),
	      SEEK_SET);

	res = 0;
	res = driver_find_free_sector_from_map(target_byte, res, dev);
	if (res != 0) {
		goto found;
	}

	/* not found */
	fseek(dev, old_pos, SEEK_SET);
	return 0;

found:
	fseek(dev, old_pos, SEEK_SET);
	return res;
}

void
driver_claim_sector(uint32_t sector, FILE *dev)
{
	MffsRootEntry root;
	size_t	      old_pos;
	uint8_t	      value;

	old_pos = ftell(dev);

	driver_read_root(&root, dev);

	/* seek to start of target byte */
	fseek(dev,
	      root.sector_allocation_map *
		      mffs_sector_size_from_enum(g_sector_size) +
		  sizeof(MffsAllocationMap) + ((uint32_t)(sector / 8)),
	      SEEK_SET);

	(void)fread(&value, sizeof(value), 1, dev);
	fseek(dev, -sizeof(value), SEEK_CUR);

	/* toggle bit on */
	value |= (0x80 >> (sector % 8));

	(void)fputc(value, dev);

	fseek(dev, old_pos, SEEK_SET);
}
