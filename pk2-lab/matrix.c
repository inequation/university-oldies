// fills a square matrix with a spiral, then prints it out

#include <stdlib.h>
#include <stdio.h>

#define R	7

int tab[R * R];

int main(int argc, char *argv[]) {
	int i, j = 0, k = 0;
	int xlen = R, ylen = R - 1;
	int xdir = 1, ydir = 1;
	
	memset(tab, -1, sizeof(tab));

	while (ylen > 0 || xlen > 0) {
		for (i = 0; i < xlen; i++, j += xdir, k++)
			*(tab + j) = k;
		j += (R - 1) * xdir;
		xlen--;
		xdir = -xdir;
		
		for (i = 0; i < ylen; i++, j += R * ydir, k++)
			*(tab + j) = k;
		j += (R + 1) * -ydir;
		ylen--;
		ydir = -ydir;
	}
	
	for (i = 0; i < R; i++) {
		for (j = 0; j < R; j++)
			printf("%3d ", tab[i * R + j]);
		fputc('\n', stdout);
	}
}
