
#include "../radvd.h"
#include <string.h>
#include <stdio.h>

struct range {
	double lower;
	double upper;
};

int main(int argc, char * argv[])
{
	int i;
	struct range range[] = {
		{0, 1},
		{0, 0.0001},
		{1, 2},
		{5, 5000000},
		{-1, 1},
	};
	
	for (i = 0; i < sizeof(range)/sizeof(range[0]); ++i) {
		int j;
		int const COUNT = 1000;
		for (j = 0; j < COUNT; ++j) {
			double x = rand_between(range[i].lower, range[i].upper);
			if (x < range[i].lower)
				return 1;
			if (x > range[i].upper)
				return 1;
		}
	}
	return 0;
}
