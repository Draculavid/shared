#pragma once
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <time.h>
#include <iostream>

class CircularBuffer
{
private:
	//The handles for the filemaps and mutex
	HANDLE hMapFile, sMapFile, mutex;

	//variables
	size_t chunkSize; //the multiple
	size_t buffSize;
	size_t idOrOffset; /*Used as id for producer, offset for consumer*/

	//the variables that hold the addresses for the shared values
	size_t* tail;
	size_t* head;
	size_t* clients;

	//the variables that hold the viewmapfile
	char* cBuf, *sBuf;

	size_t padCalc(size_t msgSize, size_t chunkSize);

	struct Header
	{
		size_t id;
		size_t length;
		size_t nrClientsLeft;

		/*Just using three variables as the fourth (offset) will be
		  redundant, considering that the length will be a multiple
		  of 256
		  */
	};
	CircularBuffer();
	bool canRead();  
	size_t canWrite();
public:
	//constructor
	CircularBuffer(
		LPCWSTR buffName,          // unique name
		const size_t& buffSize,    // size of the whole filemap
		const bool& isProducer,    // is this buffer going to be used as producer
		const size_t& chunkSize);  // round up messages to multiple of this.

	//destructor
	~CircularBuffer();

	bool push(const void* msg, size_t length);
	bool pop(char* msg, size_t& length);
};