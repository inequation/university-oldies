#ifndef VIEW_H
#define VIEW_H

#include "model.h"

/*
Format string:
d - day, numerically
m - month, numerically
M - month, string
y - year, numerically
*/
void v_print(ldate_t *d, char *fmt);

#endif // VIEW_H
