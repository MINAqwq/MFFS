#include "mffscli.h"

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

size_t
str_replace(char *str, char c, char x)
{
	size_t replacement_count;

	replacement_count = 0;

	while (*str) {
		if (*str == c) {
			*str = x;
			replacement_count++;
		}

		str++;
	}

	return replacement_count;
}
