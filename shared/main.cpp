/* Application producer: David Wigelius */
#include "CircularBuffer.h"

using namespace std;

#pragma region the random functions
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

	for (size_t i = 0; i < rLen; ++i) {
		s[i] = alphanum[random(1, sizeof(alphanum))];
	}

	s[rLen] = '\0';

	return rLen;
}
#pragma endregion

bool Producer()
{
	return false;
}

bool Consumer()
{
	return false;
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

		The second will be delay, it will be sent in milliseconds

		The third argument will be memorySize, which will be the size
		of memory we will allocate for the shared memory

		The fourth is numMessages, which will dictate how many messages
		this executable will sent between the producer and consumers

		The fifth is random|msgSize. It will tell the application if
		we will send random sized messages of a fixed size. If it's a
		fixed size, i guess we should send 1/4 of the reserved memory
		in each message.
		*/

		//if (strcmp(args[1], "Producer") == 0)
		//else if (strcmp(args[1], "Consumer") == 0)
	}
}