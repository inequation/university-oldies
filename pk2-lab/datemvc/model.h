#ifndef MODEL_H
#define MODEL_H

typedef struct ldate_s {
	int year;
	int month;
	int day;
} ldate_t;

void m_offset_date(ldate_t *d, int ofs);

#endif // MODEL_H

