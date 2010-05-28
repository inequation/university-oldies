#include <stdio.h>
#include "model.h"
#include "view.h"

static char *monthNames[] = {
	"January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December"
};

void v_print(ldate_t *d, char *fmt) {
	for (; *fmt; fmt++) {
		switch (*fmt) {
			case 'd':
				printf("%d", d->day);
				break;
			case 'm':
				printf("%d", d->month);
				break;
			case 'y':
				printf("%d", d->year);
				break;
			case 'M':
				printf("%s", monthNames[(d->month - 1)
					% 12]);
				break;
			default:
				fputc(*fmt, stdout);
				break;
		}
	}
}
