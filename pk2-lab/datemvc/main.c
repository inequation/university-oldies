#include <stdlib.h>
#include <stdio.h>
#include "model.h"
#include "view.h"

void get_data(ldate_t *d, int *ofs) {
	char buf[16];
	printf("Enter day:\n");
	d->day = atof(fgets(buf, sizeof(buf) - 1, stdin));
	printf("Enter month:\n");
	d->month = atof(fgets(buf, sizeof(buf) - 1, stdin));
	printf("Enter year:\n");
	d->year = atof(fgets(buf, sizeof(buf) - 1, stdin));
	printf("Enter offset in days:\n");
	*ofs = atof(fgets(buf, sizeof(buf) - 1, stdin));
}

int main(int argc, char *argv) {
	ldate_t d;
	int ofs;

	get_data(&d, &ofs);
	printf("Before sanitization: ");
	v_print(&d, "d-m-y\n");
	m_sanitize_date(&d);
	printf("After sanitization: ");
	v_print(&d, "d M, y\n");
	m_offset_date(&d, ofs);
	printf("After offsetting: ");
	v_print(&d, "d M, y\n");
	return 0;
}
