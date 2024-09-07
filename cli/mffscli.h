#include <stdint.h>
#include <stdio.h>

#include "../libmffs/mffs.h"

#ifndef MFFSCLI_VERSION
#define MFFSCLI_VERSION "_selfcomp"
#endif

#ifndef GIT_HASH
#define GIT_HASH "no-git?"
#endif

/* ===================== driver ===================== */

/* global driver configuration */
void driver_configure(uint8_t sector_size);

/* write to lock state (won't respected nor read the actual lock state) */
void driver_write_lock_state(uint8_t lock_state, FILE *dev);

/* read the root entry from a mffs formatted blockdevice */
uint8_t driver_read_root(MffsRootEntry *root_buffer, FILE *dev);

/* read alle root entry file entries
 * file_entries needs enough space to store all root entries
 * hint: (sizeof(*file_entries) * mffs_root_fileentry_capacity(g_sector_size))
 */
void driver_read_root_files(MffsFileEntry *file_entries, FILE *dev);

/* as the name says, iterate through the root file places, search a free
 * place and return the index. -1 on error btw. */
size_t driver_root_find_free_file_place(MffsFileEntry *file_entries);

/* find and read a file entry from mffs formatted blockdevice */
uint8_t driver_read_file_entry(const char    *virtual_path,
			       MffsFileEntry *file_buffer,
			       size_t *dev_pos_buffer, FILE *dev);

/* TODO: create a new empty file at the given virtual path */
uint8_t driver_file_create(const char *virtual_path, FILE *dev);

/* TODO: set the attributes of the file at the given virtual path */
uint8_t driver_file_set_attr(const char		    *virtual_path,
			     MffsFileEntryAttributes attr, FILE *dev);

/* TODO */
uint8_t driver_file_write();

/* find a free sector and return it's number (0 if none is free) */
uint32_t driver_find_free_sector(FILE *dev);

/* claim a sector by setting it's bit in the allocation map to 1 (will give a
 * shit if the bit is already run) */
void driver_claim_sector(uint32_t sector, FILE *dev);

/* ===================== strings ===================== */

/* return 0 if the null delimited strings are not equal */
uint8_t str_equal(const char *str1, const char *str2);

/* replace all c in str with x and return the number of replacements */
size_t str_replace(char *str, char c, char x);
