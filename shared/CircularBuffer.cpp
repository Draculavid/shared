#include "CircularBuffer.h"



size_t CircularBuffer::padCalc(size_t msgSize, size_t chunkSize)
{
	size_t ret = ((msgSize / chunkSize) + 1)*chunkSize - msgSize;
	if (ret == chunkSize)
		ret = 0;
	return ret;
}

CircularBuffer::CircularBuffer()
{
}


CircularBuffer::CircularBuffer(LPCWSTR buffName, const size_t & buffSize, const bool & isProducer, const size_t & chunkSize)
{
#pragma region declaring the main memory
	hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		(buffSize * 1 << 10),
		buffName);

	if (hMapFile == NULL)
	{
		_tprintf(TEXT("Could not create file mapping object (%d).\n"),
			GetLastError());
	}

	cBuf = (char*)MapViewOfFile(
		hMapFile,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		(buffSize * 1 << 10));

	if (cBuf == NULL)
	{
		_tprintf(TEXT("Could not map view of file (%d).\n"),
			GetLastError());

		CloseHandle(hMapFile);
	}

#pragma endregion
#pragma region the second memory
	//declaring a second memory to be used for control.
	sMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		12,
		(LPCWSTR)"secondMemory");

	if (sMapFile == NULL)
	{
		_tprintf(TEXT("Could not create file mapping object (%d).\n"),
			GetLastError());
	}

	sBuf = (char*)MapViewOfFile(
		sMapFile,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		12);

	if (sBuf == NULL)
	{
		_tprintf(TEXT("Could not map view of file (%d).\n"),
			GetLastError());

		CloseHandle(sMapFile);
	}
	
	tail = (size_t*)sBuf;
	*tail = 0;

	head = tail + 1;
	*head = 0;

	clients = head + 1;



#pragma endregion
#pragma region mutex declaration
	mutex = CreateMutex(nullptr, false, L"TheSuperMutalisk");
#pragma endregion
	this->idOrOffset = 0;
	this->buffSize = buffSize * 1<<10;
	this->chunkSize = chunkSize;

	if (!isProducer)
	{
		/*Increasing the clients counter*/
		while (true)
		{
			try
			{
				WaitForSingleObject(mutex, INFINITE);
				*clients += 1;
				ReleaseMutex(mutex);
				break;
			}
			catch(...)
			{
				Sleep(1);
			}
		}
	}
	else
	{
		/*a loop that puts the producer to sleep
		while there are no clients or clients are still
		connecting.
		----------------------------------------
		The producer wil first check how many clients there are,
		then sleep for 10 milliseconds and check if any new clients
		have connected.
		----------------------------------------
		If a new client has connected during this time, the loop
		will run again.
		----------------------------------------
		If no new clients have connected, the producer will assume
		that all clients are connected and will start sending messages.*/
		while (true)
		{
			try
			{
				int clientCount = 0;
				memcpy(&clientCount, (void*)clients, sizeof(int));
				if (clientCount > 0)
				{
					int oldCCount = clientCount;
					Sleep(10);
					try
					{
						memcpy(&clientCount, (void*)clients, sizeof(int));
						if (clientCount == oldCCount)
						{
							break;
						}
					}
					catch (...)
					{
						/*do nothing here, because if we can't access the
						clients it means that a consumer is currently writing to it
						which means we still have to re-loop this procedure*/
					}
				}
			}
			catch (...)
			{
				Sleep(10);
			}
		}
	}
}

CircularBuffer::~CircularBuffer()
{
	//unmapping the views
	UnmapViewOfFile(cBuf);
	UnmapViewOfFile(sBuf);

	//Closing the handles
	CloseHandle(hMapFile);
	CloseHandle(sMapFile);
	CloseHandle(mutex);
}

bool CircularBuffer::canRead() 
{
	if (idOrOffset != *head)
	{
		/*if the head and tail are at different locations, we can always read*/
		return true;
	}
	else
	{
		/*If the head and the tail are at the same location we can never read*/
		return false;
	}
	return false;
}

size_t CircularBuffer::canWrite()
{
	/*This function will return the size of the current memory
	avaliable for writing.*/
	if (*tail != *head)
	{
		if (*tail > *head)
		{
			return (*tail - *head);
		}
		else if (*tail < *head)
		{
			return ((buffSize - *head) + *tail);
		}
	}
	else 
	{
		/* If the head and the last tail are at the same position, it means that 
		 the whole buffer is free*/
		return buffSize;
	}
	return 0;
}

bool CircularBuffer::push(const void * msg, size_t length)
{
	/*If the amount of free memory is greater than the total length of the message*/
	if (canWrite() > (length + sizeof(Header) + padCalc(sizeof(Header) + length, chunkSize)))
	{
		Header mHeader{ this->idOrOffset++, length, *clients };
		if ((*head + length + sizeof(Header)) > buffSize) /*The message is bigger than the buffersize*/
		{
			if ((*head + sizeof(Header)) <= buffSize) /*at least the header will fit at the end of the buffer*/
			{
				/*writing the header*/
				memcpy((void*)(cBuf + *head), &mHeader, sizeof(Header));

				/*calculating how many characters will fit into the rest of the memory*/
				size_t fitLength = buffSize - (*head + sizeof(Header));

				/*writing the first part of the message*/
				memcpy((void*)(cBuf + *head + sizeof(Header)), msg, fitLength);

				/*writing the rest of the message*/
				memcpy((void*)cBuf, (char*)msg + fitLength, length - fitLength);

				/*updating the head position*/
				*head = ((length - fitLength + padCalc(length - fitLength, chunkSize)) % buffSize);
			}
			else /*If nothing fits in the end of the buffer*/
			{
				/*writing the header*/
				memcpy((void*)cBuf, &mHeader, sizeof(Header));

				/*writing the message*/
				memcpy((void*)(cBuf + sizeof(Header)), msg, length);

				/*updating the head position*/
				*head = ((length + sizeof(Header) + padCalc(length + sizeof(Header), chunkSize)) % buffSize);
			}
		}
		else /*if the message length isn't longer than the memory end*/
		{
			/*writing the header*/
			memcpy((void*)(cBuf + *head), &mHeader, sizeof(Header));

			/*writing the message*/
			memcpy((void*)(cBuf + *head + sizeof(Header)), msg, length);

			/*updating the head position*/
			*head = ((*head + length + sizeof(Header) + padCalc(length + sizeof(Header), chunkSize)) % buffSize);
		}
		return true;
	}
	return false;
}

bool CircularBuffer::pop(char * msg, size_t & length)
{
	if (canRead())
	{
		Header *mHeader = nullptr;
		if (this->idOrOffset + sizeof(Header) > buffSize) /*if the header didn't fit at the end of the buffer*/
		{
			mHeader = (Header*)cBuf;
			length = mHeader->length;

			/*getting the message*/
			memcpy(msg, (void*)(cBuf + sizeof(Header)), mHeader->length);

			/*updating the internal tail*/
			this->idOrOffset = (size_t)((sizeof(Header) + mHeader->length + padCalc(mHeader->length + sizeof(Header), chunkSize)) % buffSize);
		}
		else /*if there's at least a header at the chosen memory location*/
		{
			/*getting the header*/
			mHeader = (Header*)(cBuf + idOrOffset);
			length = mHeader->length;

			if (this->idOrOffset + sizeof(Header) + mHeader->length > buffSize)  /*Part of the message is at the end of the buffer*/
			{
				/*calculating how many characters are at the rest of the memory*/
				size_t fitLength = buffSize - (idOrOffset + sizeof(Header));

				/*getting the first part of the message*/
				memcpy(msg, (void*)(cBuf + idOrOffset + sizeof(Header)), fitLength);

				/*getting the second part*/
				memcpy((msg + fitLength), (void*)cBuf, mHeader->length - fitLength);

				/*updating the internal tail*/
				this->idOrOffset = (size_t)((length - fitLength + padCalc(length - fitLength, chunkSize)) % buffSize);
			}
			else /*If all of the message fit in the buffer*/
			{
				/*getting the message*/
				memcpy(msg, (void*)(cBuf + sizeof(Header) + idOrOffset), mHeader->length);

				/*updating the internal tail*/
				this->idOrOffset = (size_t)(this->idOrOffset + sizeof(Header) + mHeader->length + padCalc(mHeader->length + sizeof(Header), chunkSize)) % buffSize;
			}
		}
		/*changing contents, applying mutex*/
		while (true)
		{
			try
			{
				WaitForSingleObject(mutex, INFINITE);
				size_t tempClient = mHeader->nrClientsLeft;
				tempClient--;
				if (tempClient == 0)
				{
					*tail = this->idOrOffset;
				}
				mHeader->nrClientsLeft = tempClient;
				ReleaseMutex(mutex);
				break;
			}
			catch (...)
			{
				Sleep(1);
			}
		}
		return true;
	}
	return false;
}

