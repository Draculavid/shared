# shared
A circular buffer project.
The main part is he circular buffer class, which can be used for various applications as a communicator.

To test this project, create a .exe file of the project with the command line arguments:
1. (string)producer/consumer | 2. (int)delay | 3. (int)memory size | 4. (int) nr of messages | 5. (string/int) chunksize
--------------------
1. just write "producer" or "consumer", this will decide if the program will recieve or send messages.
--------------------
2. delay in milliseconds between messages.
--------------------
3. How much memory the application will allocate for the messages.
--------------------
4. How many messages the producer will send and for the consumer, how many it will recieve.
--------------------
5. This can be either a string or an int, if it's a string, write "random" as it will then send
random sized messages between the producer and consumers. If an int, create a set size of how big
the messages will be, if the chunksize is bigger than the memorysize, no message will be sent.
--------------------

For application use:
include the .h and .cpp file of the circular buffer class in the current project ou wish to have a circular buffer in.
Then just create a circularbuffer object and push if you want to send a message, pop if you wish to read a message.

The corresponding functions require following parametres for Push:
1. Const void * msg | 2. size_t length

1. the message wished to send, prefferably an array of chars that will be
interpretted when recieved.
--------------------
2. the length of the message. to be used by the consumer when reading the message,
so that it doesn't read out of bounds of the memory.
--------------------

Parametres for Pop:
1. char * msg | 2. size_t & length

1. a char array pointer to read the sent message.
--------------------
2. the length of the message sent.
