// fills a square matrix with a spiral, then prints it out

#include <stdlib.h>
#include <stdio.h>

#define R	4

int tab[R * R];

int main(int argc, char *argv[]) {
	int i, j, k, l;
	int xlen = R, ylen = R - 1;
	int xdir = 1, ydir = 1;
	
	memset(tab, -1, sizeof(tab));
		
	for (i = 0, k = 0, l = 0; i < R; i++) {
		for (j = 0; j < xlen; j++, k += xdir, l++)
			tab[k] = l;
		k += (R - 1) * xdir;
		xlen--;
		xdir = -xdir;
		
		for (j = 0; j < ylen; j++, k += R * ydir, l++)
			tab[k] = l;
		k += (R + 1) * -ydir;
		ylen--;
		ydir = -ydir;
	}
	
	for (i = 0; i < R; i++) {
		for (j = 0; j < R; j++)
			printf("%3d ", tab[i * R + j]);
		fputc('\n', stdout);
	}
}
