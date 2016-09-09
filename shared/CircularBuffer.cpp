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
	/*
	if the bool isProducer is true, then don't increase the client value, keep that at zero.
	If it's false, then  increase it with a mutex and when the client > 0, the program will run,
	i think...*/

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
		buffSize);

	if (cBuf == NULL)
	{
		_tprintf(TEXT("Could not map view of file (%d).\n"),
			GetLastError());

		CloseHandle(hMapFile);
	}

	/* setting the current position */
	//currentPosition = cBuf;

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
		buffSize);

	if (sBuf == NULL)
	{
		_tprintf(TEXT("Could not map view of file (%d).\n"),
			GetLastError());

		CloseHandle(sMapFile);
	}

	/*Now setting the adresses to the control variables*/ 
	//int t1 = 0;
	//int h1 = 0;
	//int c1 = 0;
	
	tail = (size_t*)sBuf;
	*tail = 0;
	//memcpy((void*)tail, &t1, sizeof(int));

	head = tail + 1;
	*head = 0;
	//memcpy((void*)head, &h1, sizeof(int));

	clients = head + 1;
	//memcpy((void*)clients, &c1, sizeof(int));



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
						clients it means that a cunsomer is currently writing to it
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
	/*connect this function to the shared head and
	compare it to the last tail, or if it just returns
	the current position of the head??*/
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
				//memcpy(mHeader, (Header*)cBuf + idOrOffset, sizeof(Header));
				mHeader = (Header*)cBuf + idOrOffset;
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
	/*Write a supercool function here that compares the
	values of the head and last tail of the shared buffer,
	then returns how many bytes that are free*/
	/*getting the position of the head and the last tail*/
	//size_t tailPos = 0, headPos = 0;
	//memcpy(&tailPos, (void*)tail, sizeof(int));
	//memcpy(&headPos, (void*)head, sizeof(int));

	if (*tail != *head)
	{
		if (*tail > *head)
		{
			return (buffSize - *tail + *head);
		}
		else if (*tail < *head)
		{
			return (buffSize - (*head - *tail));
		}
	}
	else /*<--------------------------------------------------------------look at this one later*/
	{
		Header* mHeader = nullptr;

		/*inserting a loop here just for precausion.
		Because if the memory cannot be read it means
		that a consumer has locked it and is currently
		changing it's contents*/
		while (true)
		{
			try
			{
				//memcpy(mHeader, (Header*)cBuf + *head, sizeof(Header));
				mHeader = (Header*)cBuf + *head;
				break;
			}
			catch (...)
			{
				Sleep(5);
			}
		}

		/*if nrClientsLeft are 0, it means that all of the consumers have read the messages*/
		if (mHeader->nrClientsLeft == 0)
		{
			return buffSize;
		}
		else
			return 0;
	}
	return 0;
}

bool CircularBuffer::push(const void * msg, size_t length)
{
	if (canWrite() > length)
	{
		Header mHeader{ this->idOrOffset++, length, *clients };
		if ((*head + length + sizeof(Header)) > buffSize)
		{
			if ((*head + sizeof(Header)) <= buffSize)
			{
				/*writing the header*/
				memcpy((void*)(cBuf + *head), &mHeader, sizeof(Header));

				/*calculating how many characters will fit into the rest of the memory*/
				size_t fitLength = buffSize - (length + sizeof(Header)) - sizeof(Header);

				/*writing the first part of the message*/
				memcpy((void*)(cBuf + *head + sizeof(Header)), msg, fitLength);

				/*writing the rest of the message*/
				memcpy((void*)cBuf, (char*)msg + fitLength, length - fitLength); //check this one <------------------------------

				/*updating the head position*/
				*head = ((length - fitLength) % buffSize) + padCalc(length - fitLength, chunkSize); //check this one as well<--------------------
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
		bool isLast = false;
		if (this->idOrOffset + sizeof(Header) > buffSize) /*if the header didn't fit at the end of the buffer*/
		{
			Header *mHeader = (Header*)cBuf;
			length = mHeader->length;

			/*changing contents, so using a mutex*/
			while (true)
			{
				try
				{
					WaitForSingleObject(mutex, INFINITE);
					mHeader->nrClientsLeft--;
					if (mHeader->nrClientsLeft == 0)
						isLast = true;
					ReleaseMutex(mutex);
					break;
				}
				catch (...)
				{
					Sleep(5);
				}
			}

			/*getting the message*/
			memcpy(msg, (void*)(cBuf + sizeof(Header)), mHeader->length);

			/*updating the internal tail*/
			this->idOrOffset = (size_t)((sizeof(Header) + mHeader->length) % buffSize) + padCalc(mHeader->length + sizeof(Header), chunkSize);
		}
		else /*if there's at least a header at the chosen memory location*/
		{
			/*getting the header*/
			Header *mHeader = (Header*)(cBuf + idOrOffset);
			length = mHeader->length;

			/*changing contents, so using a mutex*/
			while (true)
			{
				try
				{
					WaitForSingleObject(mutex, INFINITE);
					mHeader->nrClientsLeft--;
					if (mHeader->nrClientsLeft == 0)
						isLast = true;
					ReleaseMutex(mutex);
					break;
				}
				catch (...)
				{
					Sleep(5);
				}
			}

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
		if (isLast)
		{
			/*changing contents, so using a mutex*/
			while (true)
			{
				try
				{
					WaitForSingleObject(mutex, INFINITE);
					*tail = this->idOrOffset;
					ReleaseMutex(mutex);
					break;
				}
				catch (...)
				{
					Sleep(5);
				}
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
