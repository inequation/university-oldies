#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

int sum(int n, ...) {
	int i, s = 0;
	va_list v;

	va_start(v, n);
	for (i = 0; i < n; i++)
		s += va_arg(v, int);
	va_end(v);
	return s;
}

int main(int argc, char *argv[]) {
	int nmbrs[] = {7, 4, 8, 0};
	int *p = nmbrs;

	while (*p)
		printf("%d + \n", *p++);

	printf(" = %d\n", sum(3, nmbrs[0], nmbrs[1], nmbrs[2]));

	return 0;
}
