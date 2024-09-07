#include "mffscli.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum {
	CLI_ARGUMENT_NONE = 0,
	CLI_ARGUMENT_MOVE,
	CLI_ARGUMENT_COPY,
	CLI_ARGUMENT_DELETE,
	CLI_ARGUMENT_CREATE
} CliArgument;

static const char *path_block_dev = NULL;

static CliArgument argument_type = -1;

static char *g_arg_src = NULL;
static char *g_arg_dst = NULL;

static uint8_t opt_quite = 0;
static uint8_t opt_sector_size = MFFS_SECTOR_SIZE_512B;

[[noreturn]] static void
print_help()
{
	if (!opt_quite) {
		fputs("mffscli version " MFFSCLI_VERSION "-" GIT_HASH
		      "\n> mffscli (OPTIONS) [INSTRUCTION] [BLOCK_DEV]\n"
		      "[=============== <instructions> ===============]\n"
		      "| -m / --move                Move a file       |\n"
		      "| -c / --copy                Copy a file       |\n"
		      "| -d / --delete              Delete a file     |\n"
		      "| -x / --create              Create a new file |\n"
		      "| -l / --list                List a directory  |\n"
		      "[================= <options> ==================]\n"
		      "| -s / --sector-size         Sector size       |\n"
		      "| -q / --quite               Suppress output   |\n",
		      stderr);
	}

	exit(1);
}

void
args_parse(int argc, char **argv)
{
	char *arg_src;
	char *arg_dst;
	char *current_arg;

	path_block_dev = argv[--argc];

	arg_src = NULL;
	arg_dst = NULL;
	argument_type = CLI_ARGUMENT_NONE;
	while (argc--) {
		current_arg = argv[argc];

		if (current_arg[0] == '-') {
			++current_arg;
			switch (*current_arg) {
			case 'm':
				goto args_write_move;

			case 'q':
				goto args_write_silent;
			case 's':
				goto args_write_sector_size;
			}

			if (str_equal(current_arg, "-move")) {
			args_write_move:
				if (argument_type || !arg_src || !arg_dst) {
					print_help();
				}

				argument_type = CLI_ARGUMENT_MOVE;
				g_arg_src = arg_src;
				g_arg_dst = arg_dst;
				goto args_reset;
			}

			if (str_equal(current_arg, "-quite")) {
			args_write_silent:
				if (opt_quite || arg_src || arg_dst) {
					print_help();
				}

				opt_quite = 1;
				goto args_reset;
			}

			if (str_equal(current_arg, "-sector-size")) {
			args_write_sector_size:
				if (opt_quite || !arg_src || arg_dst) {
					print_help();
				}

				switch (strtol(arg_src, NULL, 10)) {
				case 512:
					opt_sector_size = MFFS_SECTOR_SIZE_512B;
					break;
				case 1024:
					opt_sector_size = MFFS_SECTOR_SIZE_1K;
					break;
				case 2048:
					opt_sector_size = MFFS_SECTOR_SIZE_2K;
					break;
				case 4096:
					opt_sector_size = MFFS_SECTOR_SIZE_4K;
					break;
				default:
					print_help();
				};
				goto args_reset;
			}

		args_reset:
			arg_src = NULL;
			arg_dst = NULL;
			continue;
		}

		if (!arg_src) {
			arg_src = current_arg;
		}

		else if (!arg_dst) {
			arg_dst = current_arg;
		}

		else {
			print_help();
		}
	}
}

int
main(int argc, char **argv)
{
	if (argc < 2) {
		print_help();
	}

	args_parse(argc - 1, argv + 1);

	if (!argument_type) {
		print_help();
	}

	FILE *test;
	test = fopen("testblock", "rb+");
	driver_configure(opt_sector_size);
	if (!driver_file_create("/testfile", test)) {
		fputs("not worked :c\n", stderr);
	}

	fclose(test);

	return 0;
}
