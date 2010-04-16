#include <stdlib.h>
#include <string.h>

void strwtf(const char *in, const char *delim, char ***out) {
	char *p, *q, **r, *s;
	int count = 0;

    s = (char *)in;
	for (p = (char *)in; *p; p++) {
		for (q = (char *)delim; *q; q++) {
			if (*p == *q) {
			    if (p - s > 0)
                    count++;
                s = p + 1;
				break;
			}
		}
	}
	*out = malloc(sizeof(*out) * (count + 2));
	memset(*out, 0, sizeof(*out) * (count + 2));
	s = (char *)in;
	r = *out;
	for (p = (char *)in; *p; p++) {
		for (q = (char *)delim; *q; q++) {
			if (*p == *q) {
			    if (p - s > 0) {
                    *r = malloc(p - s + 1);
                    strncpy(*r, s, p - s);
                    r++;
			    }
				s = p + 1;
				break;
			}
		}
	}
	*r = malloc(p - s + 1);
	strncpy(*r, s, p - s);
}

int main(int argc, char *argv[]) {
	char s[] = "  omfg  wttttf lol";
	char delim[] = " t";
	char **out, **p;
	strwtf(s, delim, &out);
	for (p = out; *p; p++) {
	    printf("\"%s\"\n", *p);
		free(*p);
	}
	free(out);
	return 0;
}
