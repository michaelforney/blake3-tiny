#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arg.h"
#include "blake3.h"

static const char *argv0;
static unsigned char *out;
static size_t outlen = 32;

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-bct] [-l length] [file...]\n", argv0);
	exit(1);
}

static int
sumfile(const char *name, FILE *file, unsigned char *out, size_t outlen)
{
	char buf[16384];
	struct blake3 ctx;
	size_t len;

	blake3_init(&ctx);
	do {
		len = fread(buf, 1, sizeof(buf), file);
		if (len > 0)
			blake3_update(&ctx, buf, len);
	} while (len == sizeof(buf));
	if (ferror(file)) {
		fprintf(stderr, "%s: read %s: ", argv0, name);
		perror(NULL);
		return 1;
	}
	blake3_out(&ctx, out, outlen);
	return 0;
}

static int
sum(const char *name, FILE *file)
{
	size_t i;

	if (sumfile(name, file, out, outlen) != 0)
		return 1;
	for (i = 0; i < outlen; i++)
		printf("%02x", out[i]);
	printf("  %s\n", name);
	return 0;
}

static int
hexval(int c)
{
	if ('0' <= c && c <= '9')
		return c - '0';
	if ('a' <= c && c <= 'f')
		return c - 'a' + 10;
	if ('A' <= c && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

static int
checkfile(const char *name, const char *mode, const char *str, unsigned char *out, size_t len)
{
	FILE *file;
	int c1, c2;
	size_t i;

	file = fopen(name, mode);
	if (!file) {
		fprintf(stderr, "%s: open %s: ", argv0, name);
		perror(NULL);
		return 1;
	}
	sumfile(name, file, out, len);
	for (i = 0; i < len; i++) {
		c1 = hexval(str[i * 2]);
		c2 = hexval(str[i * 2 + 1]);
		if (c1 == -1 || c2 == -2) {
			fprintf(stderr, "%s: skipping invalid checksum line\n", argv0);
			return 1;
		}
		if (out[i] != (c1 << 4 | c2)) {
			printf("%s: FAILED\n", name);
			return 1;
		}
	}
	printf("%s: OK\n", name);
	return 0;
}

static int
check(const char *name, FILE *file)
{
	const char *mode;
	char buf[8192], *pos, *end;
	size_t len;
	int ret = 0, skip = 0;

	buf[sizeof(buf) - 2] = 0;
	while (fgets(buf, sizeof(buf), file)) {
		if (buf[sizeof(buf) - 2]) {
			fprintf(stderr, "%s: skipping line that is too long\n", argv0);
			buf[sizeof(buf) - 2] = 0;
			skip = 1;
			ret = 1;
			continue;
		}
		if (skip) {
			skip = 0;
			continue;
		}
		pos = strchr(buf, ' ');
		if (!pos || pos == buf || (pos[1] != ' ' && pos[1] != '*') || (pos - buf) & 1) {
			fprintf(stderr, "%s: skipping invalid checksum line\n", argv0);
			ret = 1;
			continue;
		}
		mode = pos[1] == ' ' ? "r" : "rb";
		len = (pos - buf) / 2;
		if (len > outlen) {
			outlen = len;
			free(out);
			out = malloc(len);
			if (!out) {
				perror(argv0);
				return 1;
			}
		}
		*pos = '\0';
		pos += 2;
		end = strchr(pos, '\n');
		if (end)
			*end = '\0';
		ret |= checkfile(pos, mode, buf, out, len);
	}
	if (ferror(file)) {
		fprintf(stderr, "%s: read %s: ", argv0, name);
		perror(NULL);
		ret = 1;
	}
	return ret;
}

int
main(int argc, char *argv[])
{
	int (*func)(const char *, FILE *) = sum;
	FILE *file;
	char *end;
	const char *name, *mode = NULL;
	int ret = 0;

	argv0 = argc ? argv[0] : "b3sum";
	ARGBEGIN {
	case 'b':
		mode = "rb";
		break;
	case 'c':
		func = check;
		break;
	case 'l':
		outlen = strtoul(EARGF(usage()), &end, 10);
		if (*end)
			usage();
		break;
	case 't':
		mode = "r";
		break;
	default:
		usage();
	} ARGEND

	out = malloc(outlen);
	if (!out) {
		perror(NULL);
		return 1;
	}

	if (argc == 0) {
		if (!mode || strcmp(mode, "r") == 0 || freopen(NULL, mode, stdin)) {
			ret |= func("<stdin>", stdin);
		} else {
			fprintf(stderr, "%s: reopen stdin: ", argv0);
			perror(NULL);
			ret = 1;
		}
	} else {
		if (!mode)
			mode = "r";
		for (; argc > 0; argc--, argv++) {
			name = *argv;
			file = fopen(name, mode);
			if (file) {
				ret |= func(name, file);
				fclose(file);
			} else {
				fprintf(stderr, "%s: open %s: ", argv0, name);
				perror(NULL);
				ret = 1;
			}
		}
	}

	return ret;
}
