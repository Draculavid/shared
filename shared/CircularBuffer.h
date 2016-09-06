#pragma once
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <time.h>

class CircularBuffer
{
private:

	//the variables that hold the addresses for the shared values
	char* tail;
	char* head;
	char* clients;

	struct Header
	{
		size_t id;
		size_t length;
		/*Just using two variables as the third (offset) will be
		  redundant, considering that the length will be a multiple
		  of 256
		  */
	};
	CircularBuffer();
public:
	//constructor
	CircularBuffer(
		LPCWSTR buffName,          // unique name
		const size_t& buffSize,    // size of the whole filemap
		const bool& isProducer,    // is this buffer going to be used as producer
		const size_t& chunkSize);  // round up messages to multiple of this.

	//destructor
	~CircularBuffer();

	size_t canRead();  // returns how many bytes are available for reading. 
	size_t canWrite(); // returns how many bytes are free in the buffer.
					   // try to send a message through the buffer,
					   // if returns true then it succeeded, otherwise the message has not been sent.
					   // it should return false if the buffer does not have enough space.
	bool push(const void* msg, size_t length);
	// try to read a message from the buffer, and the implementation puts the content
	// in the memory. The memory is expected to be allocated by the program that calls
	// this function.
	bool pop(char* msg, size_t& length);
};

/*

class CircBufferFixed
{
private:
// your private stuff,
// implementation details, etc.
//
struct Header
{
size_t id;
size_t length;
size_t padding;
// maybe number of consumers here?
};

public:
// Constructor
CircBufferFixed(
LPCWSTR buffName,          // unique name
const size_t& buffSize,    // size of the whole filemap
const bool& isProducer,    // is this buffer going to be used as producer
const size_t& chunkSize);  // round up messages to multiple of this.

// Destructor
~CircBufferFixed();

size_t canRead();  // returns how many bytes are available for reading.
size_t canWrite(); // returns how many bytes are free in the buffer.
// try to send a message through the buffer,
// if returns true then it succeeded, otherwise the message has not been sent.
// it should return false if the buffer does not have enough space.
bool push(const void* msg, size_t length);
// try to read a message from the buffer, and the implementation puts the content
// in the memory. The memory is expected to be allocated by the program that calls
// this function.
bool pop(char* msg, size_t& length);
};
*/