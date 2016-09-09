#include "CircularBuffer.h"



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
	currentPosition = cBuf;

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
	int t1 = 0;
	int h1 = 0;
	//int c1 = 0;
	
	tail = sBuf;
	memcpy((void*)tail, &t1, sizeof(int));

	head = tail + sizeof(int);
	memcpy((void*)head, &h1, sizeof(int));

	clients = head + sizeof(int);
	//memcpy((void*)clients, &c1, sizeof(int));

#pragma endregion

	this->buffSize = buffSize * 1<<10;
	this->chunkSize = chunkSize;

	if (!isProducer)
	{
		/*Increasing the clients counter*/
		while (true)
		{
			try
			{
				int newClient = 0;
				//mutex here <-------------------------------------------
				memcpy(&newClient, (void*)clients, sizeof(int));
				newClient++;
				memcpy((void*)clients, &newClient, sizeof(int));
				//unlock mutex here <------------------------------------
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
		this->id = 0;
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
}

size_t CircularBuffer::canRead()
{
	/*connect this function to the shared head and
	compare it to the last tail, or if it just returns
	the current position of the head??*/
	size_t headPos = 0;
	memcpy(&headPos, (void*)head, sizeof(int));
	if ((currentPosition - cBuf) != headPos)
	{
		if ((currentPosition - cBuf) > headPos)
		{
			return 1;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		Header mHeader;

		while (true)
		{
			try
			{
				memcpy(&mHeader, (void*)currentPosition, sizeof(Header));
				break;
			}
			catch (...)
			{
				Sleep(5);
			}
		}
		if (mHeader.nrClientsLeft > 0)
		{
			return 1;
		}
	}
	return 0;
}

size_t CircularBuffer::canWrite()
{
	/*Write a supercool function here that compares the
	values of the head and last tail of the shared buffer,
	then returns how many bytes that are free*/
	/*getting the position of the head and the last tail*/
	size_t tailPos = 0, headPos = 0;
	memcpy(&tailPos, (void*)tail, sizeof(int));
	memcpy(&headPos, (void*)head, sizeof(int));

	if (tailPos != headPos)
	{
		if (tailPos > headPos)
		{
			return (buffSize - tailPos + headPos);
		}
		else if (tailPos < headPos)
		{
			return (buffSize - (headPos - tailPos));
		}
	}
	else
	{
		Header mHeader;

		/*inserting a loop here just for precausion.
		Because if the memory cannot be read it means
		that a consumer has locked it and is currently
		changing it's contents*/
		while (true)
		{
			try
			{
				memcpy(&mHeader, (void*)currentPosition, sizeof(Header));
				break;
			}
			catch (...)
			{
				Sleep(5);
			}
		}

		/*if nrClientsLeft are 0, it means that all of the consumers have read the messages*/
		if (mHeader.nrClientsLeft == 0)
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
	/*try to send a message, if it fails it will be false
	then wait in the "main" function and try to send later*/
	//size_t freeMem = canWrite();
	if (canWrite() > length)
	{
		int headPos = 0;
		memcpy(&headPos, (void*)head, sizeof(int));
		if ((headPos + length + sizeof(Header)) > buffSize)
		{
			Header mHeader;
			mHeader.id = this->id;
			mHeader.length = length;
			memcpy(&mHeader.nrClientsLeft, (void*)clients, sizeof(size_t));
			id++;

			/*calculating how much of the message we can fit at the end of the buffer*/
			int fitLength = buffSize - (headPos + sizeof(Header)); 

			/*calculating the diff in the if statement because they will behave differently
			based on which of the roads the application will take*/
			int diff = 0;
			if (fitLength >= 0)
			{
				/*writing the header to the virtual memory*/
				memcpy((void*)currentPosition, &mHeader, sizeof(Header));
				currentPosition += sizeof(Header);

				/*writing part of the message to the virtual memory*/
				memcpy((void*)currentPosition, msg, fitLength);

				/*resetting the position to the start of the circular buffer*/
				currentPosition = cBuf;
				headPos = 0;
				diff = (length - fitLength) % chunkSize;
			}
			else
			{
				/*resetting the position to the start of the circular buffer*/
				currentPosition = cBuf;
				headPos = 0;
				fitLength = 0;

				memcpy((void*)currentPosition, &mHeader, sizeof(Header));

				currentPosition += sizeof(Header);
				headPos += sizeof(Header);
				diff = (length + sizeof(Header)) % chunkSize;
			}
			char* sPartOfMsg = (char*)msg + fitLength; //<----------------------------see if this works
			/*calculating and writing the rest of the message*/
			memcpy((void*)currentPosition, sPartOfMsg, (length - fitLength));
			currentPosition += length - fitLength;

			/*calculating the diff/padding*/
			headPos += length - fitLength;
			int padding = chunkSize - diff;
			//temporary, come up with something smarter
			if (diff == 0)
				padding = 0;

			/*updating the positions*/
			headPos += padding;
			currentPosition += padding;

			/*updating the position of the header in the control message*/
			if (headPos == buffSize)
			{
				headPos = 0;
				currentPosition = cBuf;
			}
			memcpy((void*)head, &headPos, sizeof(int));

			/*Print message*/
			printf("Producer\nId: %d\nMessage: %s\n", mHeader.id, msg);

			/*returning true*/
			return true;
		}
		else
		{
			/* Creating the header */
			Header mHeader;
			mHeader.id = this->id;
			mHeader.length = length;
			memcpy(&mHeader.nrClientsLeft, (void*)clients, sizeof(size_t));
			id++;

			/*writing the header to the virtual memory*/
			memcpy((void*)currentPosition, &mHeader, sizeof(Header));
			currentPosition += sizeof(Header);

			/*writing the message to the virtual memory*/
			memcpy((void*)currentPosition, msg, length);
			currentPosition += length;

			/*calculating the head position so that it will be a multiple of the buffSize*/
			headPos += length + sizeof(Header);

			int diff = (length + sizeof(Header)) % chunkSize;
			int padding = chunkSize - diff;
			//temporary, come up with something smarter
			if (diff == 0)
				padding = 0;

			/*updating the positions*/
			headPos += padding;
			currentPosition += padding;

			/*updating the position of the header in the control message*/
			if (headPos == buffSize)
			{
				headPos = 0;
				currentPosition = cBuf;
			}
			memcpy((void*)head, &headPos, sizeof(int));

			/*Print the message*/
			printf("Producer\nId: %d\nMessage: %s\n", mHeader.id, msg);

			/*returning true*/
			return true;
		}
	}
	return false;
}

bool CircularBuffer::pop(char * msg, size_t & length)
{
	/*Perhaps try to implement the mutex here which will
	control the clients, and if there are no clients move
	the last tail?
	
	Also try to read the message, if the message cannot be read
	return false and don't tamper with the clients in the shared buffer*/

	//size_t readable = canRead();
	if (canRead() > 0)
	{
		Header mHeader;
		bool isLast = false;
		if ((currentPosition - cBuf) + sizeof(Header) > buffSize)
		{
			currentPosition = cBuf;
			memcpy(&mHeader, (void*)currentPosition, sizeof(Header));
			length = mHeader.length;
			this->id = mHeader.id;
			mHeader.nrClientsLeft--;

			while (true)
			{
				try
				{
					//mutex here<-----------------------------
					memcpy((void*)currentPosition, &mHeader, sizeof(size_t));
					//unlock mutex here
					break;
				}
				catch (...)
				{
					Sleep(5);
					memcpy(&mHeader, (void*)currentPosition, sizeof(Header));
					mHeader.nrClientsLeft--;
				}
			}

			/*moving the currentposition/checking if it's the last tail*/
			currentPosition += sizeof(Header);
			if (mHeader.nrClientsLeft == 0)
				isLast = true;

			/*allocating memory for the message*/
			//msg = new char[mHeader.length];
			memcpy(msg, (void*)currentPosition, mHeader.length);
			currentPosition += mHeader.length;

			int diff = (length + sizeof(Header)) % chunkSize;
			int padding = chunkSize - diff;
			currentPosition += padding;
		}
		else
		{
			memcpy(&mHeader, (void*)currentPosition, sizeof(Header));
			length = mHeader.length;
			this->id = mHeader.id;
			mHeader.nrClientsLeft--;

			while (true)
			{
				try
				{
					//mutex here<-----------------------------
					memcpy((void*)currentPosition, &mHeader, sizeof(size_t));
					//unlock mutex here
					break;
				}
				catch (...)
				{
					Sleep(5);
					memcpy(&mHeader, (void*)currentPosition, sizeof(Header));
					mHeader.nrClientsLeft--;
				}
			}
			//msg = new char[mHeader.length];
			/*moving the currentposition/checking if it's the last tail*/
			currentPosition += sizeof(Header);
			if (mHeader.nrClientsLeft == 0)
				isLast = true;

			/* if the length is bigger than the buffer, the producer will have written part of the message
			in the end of the buffer and the rest in the front. */
			if (((currentPosition - cBuf) + mHeader.length) > buffSize)
			{
				/*calculating and reading part of the message*/
				int fitLength = buffSize - ((currentPosition-cBuf) + sizeof(Header)); //<--------------------------look at this function, to see if it works during runtime
				memcpy(msg, (void*)currentPosition, fitLength);

				/*reading the rest of the message*/
				currentPosition = cBuf;
				memcpy(msg + fitLength, (void*)currentPosition, (length - fitLength)); //<----------------------------look to see if it works, does not work
				currentPosition += (length - fitLength);

				/*calculating the rest of the padding*/
				int diff = (length - fitLength) % chunkSize;
				int padding = chunkSize - diff;
				currentPosition += padding;
			}
			else
			{
				/*copying the message in the buffer*/
				memcpy(msg, (void*)currentPosition, mHeader.length);
				currentPosition += mHeader.length;

				/*calculating the offset*/
				int diff = (length + sizeof(Header)) % chunkSize;
				int padding = chunkSize - diff;
				currentPosition += padding;
			}
		}
		if (isLast)
		{
			int newTail = currentPosition - cBuf;
			if (newTail == buffSize)
			{
				newTail = 0;
				currentPosition = cBuf;
			}
			memcpy((void*)tail, &newTail, sizeof(int));
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
}
