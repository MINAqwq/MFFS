#include "mkmffs.h"

uint8_t
str_equal(const char *str1, const char *str2)
{
	if ((*str1) != (*str2)) {
		return 0;
	}

	str1++;
	str2++;

	while (*str1 != 0 || *str2 != 0) {
		if ((*str1) != (*str2)) {
			return 0;
		}

		str1++;
		str2++;
	}

	return 1;
}