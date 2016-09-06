/* Application producer: David Wigelius */
#include "CircularBuffer.h"

using namespace std;
size_t random(size_t min, size_t max)
{
	int range, result, cutoff;

	if (min >= max)
		return min;
	range = max - min + 1;
	cutoff = (RAND_MAX / range) * range;

	do {
		result = rand();
	} while (result >= cutoff);

	return result % range + min;
}

size_t randomString(char *s, const size_t maxSize) {

	size_t rLen = random(1, maxSize);
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";

	for (int i = 0; i < rLen; ++i) {
		s[i] = alphanum[random(1, sizeof(alphanum))];
	}

	s[rLen] = '\0';

	return rLen;
}

int main(int argc, char* args[])
{
	if (argc >= 6)
	{
		//the first element is the executable command to
		//get to this point, so for this program the first element is garbage.

		/*
		The first usable element (1) will determine if the program
		will behave as a producer or a comsumer.

		The second 
		*/

		//if (strcmp(args[1], "Producer") == 0)
		//else if (strcmp(args[1], "Consumer") == 0)
	}
}