#include <stdint.h>

#ifndef MKMFFS_VERSION
#define MKMFFS_VERSION "_selfcomp"
#endif

/* ===================== format  ===================== */

/* format a blockdevice to MFFS */
void blockdev_format(const char *blockdev, const char *disk_name,
		     uint32_t sector_size);

/* ===================== strings ===================== */

/* return 0 if the null delimited strings are not equal */
uint8_t str_equal(const char *str1, const char *str2);
