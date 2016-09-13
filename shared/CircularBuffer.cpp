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
		if (idOrOffset > *head)
		{
			return true;
		}
		else
		{
			return true;
		}
	}
	else
	{
		Header *mHeader = nullptr;

		while (true)
		{
			try
			{
				mHeader = (Header*)(cBuf + idOrOffset);
				break;
			}
			catch (...)
			{
				Sleep(5);
			}
		}
		if (mHeader->nrClientsLeft > 0)
		{
			return true;
		}
	}
	return false;
}

size_t CircularBuffer::canWrite()
{
	//if (*tail != *head)
	//{
		if (*tail > *head)
		{
			//return (buffSize - *tail + *head);
			return (*tail - *head);
		}
		else if (*tail <= *head)
		{
			//return (buffSize - (*head - *tail));
			return ((buffSize - *head) + *tail);
		}
	//}
	//else 
	//{
	//	Header* mHeader = nullptr;

	//	/*inserting a loop here just for precausion.
	//	Because if the memory cannot be read it means
	//	that a consumer has locked it and is currently
	//	changing it's contents*/
	//	while (true)
	//	{
	//		try
	//		{
	//			mHeader = (Header*)(cBuf + *head);
	//			break;
	//		}
	//		catch (...)
	//		{
	//			Sleep(5);
	//		}
	//	}

	//	/*if nrClientsLeft are 0, it means that all of the consumers have read the messages*/
	//	if (mHeader->nrClientsLeft == 0)
	//	{
	//		return buffSize;
	//	}
	//	else
	//		return 0;
	//}
	return 0;
}

bool CircularBuffer::push(const void * msg, size_t length)
{
	//if (canWrite() > length)
	//{
	//if (*tail > *head)
	//{
	//	//return (buffSize - *tail + *head);
	//	return (*tail - *head);
	//}
	//else if (*tail <= *head)
	//{
	//	//return (buffSize - (*head - *tail));
	//	return ((buffSize - *head) + *tail);
	//}
	if ((*tail - *head) > length || ((buffSize - *head) + *tail) > length)
	{
		Header mHeader{ this->idOrOffset++, length, *clients };
		if ((*head + length + sizeof(Header)) > buffSize) /*The message is bigger than the buffersize*/
		{
			if ((*head + sizeof(Header)) <= buffSize) /*at least the header will fit at the end of the buffer*/
			{
				/*writing the header*/
				memcpy((void*)(cBuf + *head), &mHeader, sizeof(Header));

				/*calculating how many characters will fit into the rest of the memory*/
				size_t fitLength = buffSize - (length + sizeof(Header)) - sizeof(Header);

				/*writing the first part of the message*/
				memcpy((void*)(cBuf + *head + sizeof(Header)), msg, fitLength);

				/*writing the rest of the message*/
				memcpy((void*)cBuf, (char*)msg + fitLength, length - fitLength);

				/*updating the head position*/
				*head = ((length - fitLength) % buffSize) + padCalc(length - fitLength, chunkSize);
			}
			else /*If nothing fits in the end of the buffer*/
			{
				/*writing the header*/
				memcpy((void*)cBuf, &mHeader, sizeof(Header));

				/*writing the message*/
				memcpy((void*)(cBuf + sizeof(Header)), msg, length);

				/*updating the head position*/
				*head = ((length + sizeof(Header)) % buffSize) + padCalc(length + sizeof(Header), chunkSize);
			}
		}
		else /*if the message length isn't longer than the memory end*/
		{
			/*writing the header*/
			memcpy((void*)(cBuf + *head), &mHeader, sizeof(Header));

			/*writing the message*/
			memcpy((void*)(cBuf + *head + sizeof(Header)), msg, length);

			/*updating the head position*/
			*head = ((*head + length + sizeof(Header)) % buffSize) + padCalc(length + sizeof(Header), chunkSize);
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
			this->idOrOffset = (size_t)((sizeof(Header) + mHeader->length) % buffSize) + padCalc(mHeader->length + sizeof(Header), chunkSize);
		}
		else /*if there's at least a header at the chosen memory location*/
		{
			/*getting the header*/
			mHeader = (Header*)(cBuf + idOrOffset);
			length = mHeader->length;

			if (this->idOrOffset + mHeader->length > buffSize)  /*Part of the message is at the end of the buffer*/
			{
				/*calculating how many characters are at the rest of the memory*/
				size_t fitLength = buffSize - (mHeader->length + sizeof(Header)) - sizeof(Header);

				/*getting the first part of the emssage*/
				memcpy(msg, (void*)(cBuf + idOrOffset + sizeof(Header)), fitLength);

				/*getting the second part*/
				memcpy((msg + fitLength), (void*)cBuf, mHeader->length - fitLength);

				/*updating the internal tail*/
				this->idOrOffset = (size_t)((length - fitLength) % buffSize) + padCalc(length - fitLength, chunkSize);
			}
			else
			{
				/*getting the message*/
				memcpy(msg, (void*)(cBuf + sizeof(Header) + idOrOffset), mHeader->length);

				/*updating the internal tail*/
				this->idOrOffset = (size_t)(this->idOrOffset + sizeof(Header) + mHeader->length) % buffSize + padCalc(mHeader->length + sizeof(Header), chunkSize);
			}
		}
		/*changing contents, applying mutex*/
		while (true)
		{
			try
			{
				WaitForSingleObject(mutex, INFINITE);
				mHeader->nrClientsLeft--;
				if (mHeader->nrClientsLeft == 0)
				{
					*tail = this->idOrOffset;
				}
				ReleaseMutex(mutex);
				break;
			}
			catch (...)
			{
				Sleep(5);
			}
		}
		return true;
	}
	return false;
}

void CircularBuffer::closeEverything()
{
	//unmapping the views
	UnmapViewOfFile(cBuf);
	UnmapViewOfFile(sBuf);

	//Closing the handles
	CloseHandle(hMapFile);
	CloseHandle(sMapFile);
	CloseHandle(mutex);
}
