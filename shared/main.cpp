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

void Producer(LPCWSTR buffName, const size_t & delay, const size_t & buffSize, const size_t & numMessages, const size_t & chunkMsg)
{
	//just testing the constructor
	CircularBuffer cBuffer(buffName, buffSize, true, 256);
	srand(time(NULL));
	/*the current message variable will tell the producer how many
	messages are left to send */

	//int diff = 320 % 256;
	//int paddingx = 256 - diff;
	size_t msgLeft = numMessages;

	
	while (msgLeft > 0)
	{
		/*skapa meddelandet sen skicka (OBS skicka även längden på meddelandet), går det inte, lägg en sleep och testa igen*/
		//a bool variable to check if the message was sent
		bool msgSent = false;

		/*calculating the size of the message*/
		size_t msgSize;
		if (chunkMsg == 0)
			msgSize = random(1, random(1, buffSize / 4));
		else
			msgSize = random(1, chunkMsg);

		/*Creating the message*/
		char* message = new char[msgSize];
		size_t mLength = randomString(message, msgSize);
		
		/*A loop that tries to sent the message over and over until it's sent*/
		while (!msgSent)
		{
			if (cBuffer.push(message, mLength))
			{
				//printf("Producer\nId: %d\nMessage: %s\n", mHeader.id, message);
				msgSent = true;
				msgLeft--;
				Sleep(delay);
			}
			else
			{
				Sleep(10);
			}
		}

		/*deleting the message so that there will be no memory leaks*/
		delete message;
	}

	//perhaps another while loop to control that all the clients have recieved their messages

	/*In both of these function they should create a circularbuffer object
	and then try to send messages, i guess.
	
	You need to fix so that it starts when all of the consumers and shit have
	been connected. I think that the producer is supposed to be the last one to connect.
	
	Even if the producer is the first, we need to start sending when there is at least
	one client present.
	
	delay will be used for the sleep function between the messages*/

	//CircularBuffer::CircularBuffer(LPCWSTR buffName, const size_t & buffSize, const bool & isProducer, const size_t & chunkSize)
}

void Consumer(LPCWSTR buffName, const size_t & delay, const size_t & buffSize, const size_t & numMessages)
{
	CircularBuffer cBuffer(buffName, buffSize, false, 256);
	size_t msgLeft = numMessages;
	while (msgLeft > 0)
	{
		//Creating the variable that will hold the message
		char* msg = NULL; 
		size_t length;

		bool msgRecieved = false;
		while (!msgRecieved)
		{
			if (cBuffer.pop(msg, length))
			{
				msgRecieved = false;
				msgLeft--;
				Sleep(delay);
			}
			else
			{
				Sleep(10); //is this where i'm supposed to use the delay?
			}
		}
		delete msg;
	}
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

		The third argument will be memorySize (buffsize), which will be the size
		of memory we will allocate for the shared memory

		The fourth is numMessages, which will dictate how many messages
		this executable will sent between the producer and consumers

		The fifth is random|msgSize. It will tell the application if
		we will send random sized messages of a fixed size. If it's a
		fixed size, i guess we should send 1/4 of the reserved memory
		in each message.
		*/
		size_t delay = atoi(args[2]);
		size_t buffSize = atoi(args[3]);
		size_t numMessages = atoi(args[4]);
		size_t chunkSize;

		if (strcmp(args[5], "random") == 0)
			chunkSize = 0;
		else
			chunkSize = atoi(args[5]);

		if (strcmp(args[1], "Producer") == 0)
			Producer((LPCWSTR)"theSuperMap", delay, buffSize, numMessages, chunkSize);
		else if (strcmp(args[1], "Consumer") == 0)
			Consumer((LPCWSTR)"theSuperMap", delay, buffSize, numMessages);
	}
	return 0;
}