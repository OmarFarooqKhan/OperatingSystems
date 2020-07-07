

# Client

The client is a program that takes 2 arguments:

* A hostname of a logging server 
* A port number of the logging server

After it started, it will connect to the server and read messages from
STDIN. Every message is a string without 0 bytes. Messages are
terminated by a newline character \n. When there is nothing to read
anymore, the client will close the
connection and terminate with return code 0.

When there is an internal problem, such as the connection to the server breaks, an input cannot be parsed or a similar problem, the return code should be 1.

# Server

The server is a program that takes 2 arguments:

* The port number to bind to, for example 5555
* A filename of a file to write the logs to, for example: /var/log/messages.log

After the server is started, it will bind to the port and listen for
incoming connections. For every connection, the server will read
messages and write them to the logfile. The format of the logfile is
always a line number in decimal followed by a space followed by the
message itself with a \n newline character at the end. When the server
cannot bind to the port, it will return with return code 1. When a
certain file cannot be opened or writing to a file fails, you should
write an error message to STDERR but continue to operate. When the
server starts it creates a new logfile if it does not exist, and
appends the data from the client to the logfile if it already exists.

The server should be able to handle inputs that are not properly formatted and should never crash. The client should also be able to handle any kind of unexpected responses from the server or invalid lines in the input or command line argument.


* The critical section in serverThreaded should be as short as possible.
* The server must not leak memory.
* Multiple parallel clients must be handled at the same time. 


