#include "model.h"

static inline int get_days_per_month(int m, int y) {
	m = --m % 12;	// shift range from 1-12 to 0-11
	if ((m < 7 && m % 2 == 0) || (m >= 7 && m % 2 == 1))
		return 31;
	if (m % 12 != 1)	// other than February
		return 30;
	if (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0))
		return 29;
	return 28;
}

void m_offset_date(ldate_t *d, int ofs) {
	int dpm = get_days_per_month(d->month, d->year);

	d->day += ofs;
	while (d->month < 1) {
		d->month += 12;
		d->year--;
	}
	while (d->day < 1) {
		if (--d->month < 1) {
			d->month += 12;
			d->year--;
		}
		d->day = get_days_per_month(d->month, d->year);
	}
	while (d->day > dpm) {
		d->day -= dpm;
		d->month++;
		for (; d->month > 12; d->month -= 12, d->year++);
		dpm = get_days_per_month(d->month, d->year);
	}
}

void m_sanitize_date(ldate_t *d) {
	m_offset_date(d, 0);
}
