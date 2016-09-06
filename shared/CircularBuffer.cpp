#include "CircularBuffer.h"



CircularBuffer::CircularBuffer()
{
}


CircularBuffer::CircularBuffer(LPCWSTR buffName, const size_t & buffSize, const bool & isProducer, const size_t & chunkSize)
{
	/*I guess, you will filemap here and try to filemap the second one as well
	(the shared with the tail and the bipidibop), and store those adresses in 
	the private variables*/
}

CircularBuffer::~CircularBuffer()
{
}

size_t CircularBuffer::canRead()
{
	/*connect this function to the shared head and
	compare it to the last tail, or if it just returns
	the current position of the head??*/
	return size_t();
}

size_t CircularBuffer::canWrite()
{
	/*Write a supercool function here that compares the
	values of the head and last tail of the shared buffer,
	then returns how many bytes that are free*/
	return size_t();
}

bool CircularBuffer::push(const void * msg, size_t length)
{
	/*try to send a message, if it fails it will be false
	then wait in the "main" function and try to send later*/
	return false;
}

bool CircularBuffer::pop(char * msg, size_t & length)
{
	/*Perhaps try to implement the mutex here which will
	control the clients, and if there are no clients move
	the last tail?
	
	Also try to read the message, if the message cannot be read
	return false and don't tamper with the clients in the shared buffer*/
	return false;
}
