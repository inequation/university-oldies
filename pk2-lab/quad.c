#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

float a = 0.f, b = 0.f, c = 0.f;

int parse_eq(const char *s) {
	if (!sscanf(s, "%fx^2%+fx%+f")) {
		if (!sscanf(s, "%fx%+f")) {
			if (!sscanf(s, "%f"))
				return 0;
		}
	}
	return 1;
}

int main(int argc, char *argv[]) {
	char s[1024] = {0};
	int i;

	for (i = 1; i < argc; i++) {
		strncat(s, argv[i], sizeof(s));
		strncat(s, " ", sizeof(s));
	}
	if (!parse_eq) {
		fprintf(stderr, "Error in equation\n");
		return 1;
	}
	if (a == 0.f) {
		// linear or constant
		if (b == 0.f)
			printf("Function is constant:\ny = %f\nThere is no such x that f(x) = 0\n", c);
		else
			printf("Function is linear:\ny = %fx%+f\nf(%f) = 0\n", b, c, c/b);
	} else {
		// quadratic
		float delta = b * b - 4 * a * c;
		printf("Function is quadratic:\ny = %fx^2+%+f+%+f\n", a, b, c);
		if (delta < 0.f)
			printf("There is no such x that f(x) = 0\n");
		else if (delta == 0.f)
			printf("f(%f) = 0\n", -b / 2 * a);
		else {
			float inv2a = 1.f / 2 * a;
			delta = sqrtf(delta);
			printf("f(%f) = 0 and f(%f) = 0\n", (-b - delta) * inv2a, (-b + delta) * inv2a);
		}
	}
	return 0;
}
