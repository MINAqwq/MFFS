#include "mkmffs.h"

#include <stdio.h>
#include <stdlib.h>

static const char *path_block_dev = NULL;

static const char *arg_name = "MINA_IS_LIT";
static uint32_t	   arg_sector_size = 512;

[[noreturn]] static void
print_help()
{
	fputs("mkmffs version " MKMFFS_VERSION
	      " \n> mkmffs (OPTIONS) [BLOCK_DEV]\n"
	      "[================= <options> =================]\n"
	      "| -n / --name           disk name             |\n"
	      "| -s / --sector-size    sector size in bytes  |\n",
	      stderr);
	exit(0);
}

void
args_parse(int argc, char **argv)
{
	char *arg;
	char *current_arg;

	path_block_dev = argv[--argc];

	arg = NULL;
	while (argc--) {
		current_arg = argv[argc];

		if (current_arg[0] == '-') {
			if (arg == NULL) {
				print_help();
			}

			++current_arg;
			switch (*current_arg) {
			case 'n':
				goto arg_set_name;

			case 's':
				goto arg_set_sector_size;
			}

			if (str_equal(current_arg, "-name")) {
			arg_set_name:
				arg_name = arg;
				goto arg_reset;
			}

			if (str_equal(current_arg, "-sector-size")) {
			arg_set_sector_size:
				arg_sector_size = strtol(arg, NULL, 10);
				goto arg_reset;
			}

		arg_reset:
			arg = NULL;
			continue;
		}

		if (arg) {
			print_help();
		}

		arg = current_arg;
	}
}

int
main(int argc, char **argv)
{
	if (argc < 2) {
		print_help();
	}

	args_parse(argc - 1, argv + 1);

	fprintf(stderr,
		"MFFS SETUP\n=> Disk Name: %s\n=> Sector Size: %d Byte\n",
		arg_name, arg_sector_size);

	blockdev_format(path_block_dev, arg_name, arg_sector_size);

	fputs("finished without error\n", stderr);

	return 0;
}
