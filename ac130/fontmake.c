#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

int idlen;
int w, h;
bool has_alpha;
bool top_down;

bool transcode_header(FILE *in, char *guard) {
	unsigned char u8;
	unsigned short u16;

	// idlen
	fread(&u8, 1, 1, in);
	idlen = u8;
	// colour map type
	fread(&u8, 1, 1, in);
	if (u8)
		return false;
	// image type
	fread(&u8, 1, 1, in);
	if (u8 != 3)
		return false;
	// fseek over uninteresting stuff
	fseek(in, 2 + 2 + 1 + 2 + 2, SEEK_CUR);
	// image dimensions
	fread(&u16, 1, 2, in);
	w = u16;
	fread(&u16, 1, 2, in);
	h = u16;
	// bits per pixel
	fread(&u8, 1, 1, in);
	if (u8 != 8) {
		if (u8 != 16)
			return false;
		else
			has_alpha = true;
	} else
		has_alpha = false;
	fread(&u8, 1, 1, in);
	top_down = (u8 & 0x20) == 0;

	// seek to data
	fseek(in, idlen, SEEK_CUR);

	printf("// Generated by fontmake, do not modify\n"
		"#ifndef %s_H\n"
		"#define %s_H\n"
		"\n"
		"#define %s_WIDTH\t%d\n"
		"#define %s_HEIGHT\t%d\n"
		"static const unsigned char %s[] =\n", guard, guard, guard, w, guard, h,
			guard);
	return true;
}

void transcode_data(FILE *in) {
	int x, y, col;
	unsigned char buf[2];

	col = 9;	// assume 8-character tabs just in case
	printf("\t\"");
	for (y = 0; y < h; y++) {
		if (top_down)
			fseek(in, 18 + idlen + (h - y - 1) * w * (has_alpha ? 2 : 1),
				SEEK_SET);
		for (x = 0; x < w; x++) {
			if (col + 4 > 75) {
				col = 5;
				printf("\"\n"
					"\t\"");
			}
			fread(buf, 1, has_alpha ? 2 : 1, in);
			col += printf("\\x%X", (unsigned int)buf[0]);
		}
	}
	printf("\";\n");
}

void transcode_footer(char *guard) {
	printf("#endif // %s_H\n", guard);
}

int main(int argc, char *argv[]) {
	FILE *in;
	char guard[256];
	char *pin, *pout, *ext;
	if (argc != 2) {
		printf("AC-130 fontmake\n"
			"Usage: %s <infile>\n"
			"Resulting C header will be output to stdout.\n", argv[0]);
		return 0;
	}
	if (!(in = fopen(argv[1], "rb"))) {
		printf("Could not open %s for reading!\n", argv[1]);
		return 1;
	}
	// make up a header guard name
	if (!(pin = strrchr(argv[1],
#ifdef WIN32
		'\\'
#else
		'/'
#endif
		)))
		pin = argv[1];
	else
		pin++;
	ext = strrchr(argv[1], '.');
	pout = guard;
	while (*pin && pin != ext)
		*pout++ = toupper(*pin++);
	*pout = 0;

	if (!transcode_header(in, guard)) {
		printf("Invalid TGA file!\n");
		return 1;
	}
	transcode_data(in);
	transcode_footer(guard);
	fclose(in);
	return 0;
}
