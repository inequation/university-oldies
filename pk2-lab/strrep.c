#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	FILE *in, *out;
	int i, j, c, ret, found;

	if (argc < 4 || argc % 2 != 0) {
		printf("Check the goddamn arguments!\n");
		return 1;
	}

	in = fopen(argv[1], "rb");
	out = fopen("strrep.tmp", "wb");
	while (!feof(in)) {
		ret = ftell(in);
		found = 0;
		for (i = 0, j = 0; i < argc - 2; i += 2, j = 0) {
			while (!feof(in) && argv[i + 2][j] && (c = fgetc(in)) == argv[i + 2][j])
				j++;
			if (argv[i + 2][j]) {
				// no match
				fseek(in, ret, SEEK_SET);
				continue;
			}
			// match
			for (j = 0; argv[i + 3][j]; j++)
				fputc(argv[i + 3][j], out);
			found = 1;
			break;
		}
		if (!found) {
			if ((c = fgetc(in)) == EOF)
				break;
			fputc(c, out);
		}
	}

	fclose(in);
	fclose(out);

	remove(argv[1]);
	rename("strrep.tmp", argv[1]);

	return 0;
}
