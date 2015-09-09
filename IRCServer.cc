const char * usage =
"                                                               \n"
"IRCServer:                                                   \n"
"                                                               \n"
"Simple server program used to communicate multiple users       \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   IRCServer <port>                                          \n"
"                                                               \n"
"Where 1024 < port < 65536.                                     \n"
"                                                               \n"
"In another window type:                                        \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where talk-server      \n"
"is running. <port> is the port number you used when you run    \n"
"daytime-server.                                                \n"
"                                                               \n";

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <string>

#include "IRCServer.h"

int QueueLength = 5;

int
IRCServer::open_server_socket(int port) {

	// Set the IP address and port for this server
	struct sockaddr_in serverIPAddress; 
	memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
	serverIPAddress.sin_family = AF_INET;
	serverIPAddress.sin_addr.s_addr = INADDR_ANY;
	serverIPAddress.sin_port = htons((u_short) port);
  
	// Allocate a socket
	int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
	if ( masterSocket < 0) {
		perror("socket");
		exit( -1 );
	}

	// Set socket options to reuse port. Otherwise we will
	// have to wait about 2 minutes before reusing the sae port number
	int optval = 1; 
	int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
			     (char *) &optval, sizeof( int ) );
	
	// Bind the socket to the IP address and port
	int error = bind( masterSocket,
			  (struct sockaddr *)&serverIPAddress,
			  sizeof(serverIPAddress) );
	if ( error ) {
		perror("bind");
		exit( -1 );
	}
	
	// Put socket in listening mode and set the 
	// size of the queue of unprocessed connections
	error = listen( masterSocket, QueueLength);
	if ( error ) {
		perror("listen");
		exit( -1 );
	}

	return masterSocket;
}

void
IRCServer::runServer(int port)
{
	int masterSocket = open_server_socket(port);

	initialize();
	
	while ( 1 ) {
		
		// Accept incoming connections
		struct sockaddr_in clientIPAddress;
		int alen = sizeof( clientIPAddress );
		int slaveSocket = accept( masterSocket,
					  (struct sockaddr *)&clientIPAddress,
					  (socklen_t*)&alen);
		
		if ( slaveSocket < 0 ) {
			perror( "accept" );
			exit( -1 );
		}
		
		// Process request.
		processRequest( slaveSocket );		
	}
}

int
main( int argc, char ** argv )
{
	// Print usage if not enough arguments
	if ( argc < 2 ) {
		fprintf( stderr, "%s", usage );
		exit( -1 );
	}
	
	// Get the port from the arguments
	int port = atoi( argv[1] );

	IRCServer ircServer;

	// It will never return
	ircServer.runServer(port);
	
}

//
// Commands:
//   Commands are started y the client.
//
//   Request: ADD-USER <USER> <PASSWD>\r\n
//   Answer: OK\r\n or DENIED\r\n
//
//   REQUEST: GET-ALL-USERS <USER> <PASSWD>\r\n
//   Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//
//   REQUEST: CREATE-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LIST-ROOMS <USER> <PASSWD>\r\n
//   Answer: room1\r\n
//           room2\r\n
//           ...
//           \r\n
//
//   Request: ENTER-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LEAVE-ROOM <USER> <PASSWD>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: SEND-MESSAGE <USER> <PASSWD> <MESSAGE> <ROOM>\n
//   Answer: OK\n or DENIED\n
//
//   Request: GET-MESSAGES <USER> <PASSWD> <LAST-MESSAGE-NUM> <ROOM>\r\n
//   Answer: MSGNUM1 USER1 MESSAGE1\r\n
//           MSGNUM2 USER2 MESSAGE2\r\n
//           MSGNUM3 USER2 MESSAGE2\r\n
//           ...\r\n
//           \r\n
//
//    REQUEST: GET-USERS-IN-ROOM <USER> <PASSWD> <ROOM>\r\n
//    Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//

void
IRCServer::processRequest( int fd )
{
	// Buffer used to store the comand received from the client
	const int MaxCommandLine = 1024;
	char commandLine[ MaxCommandLine + 1 ];
	int commandLineLength = 0;
	int n;
	
	// Currently character read
	unsigned char prevChar = 0;
	unsigned char newChar = 0;
	
	//
	// The client should send COMMAND-LINE\n
	// Read the name of the client character by character until a
	// \n is found.
	//

	// Read character by character until a \n is found or the command string is full.
	while ( commandLineLength < MaxCommandLine &&
		read( fd, &newChar, 1) > 0 ) {

		if (newChar == '\n' && prevChar == '\r') {
			break;
		}
		
		commandLine[ commandLineLength ] = newChar;
		commandLineLength++;

		prevChar = newChar;
	}
	
	// Add null character at the end of the string
	// Eliminate last \r
	commandLineLength--;
        commandLine[ commandLineLength ] = 0;

	printf("RECEIVED: %s\n", commandLine);

	//printf("The commandLine has the following format:\n");
	//printf("COMMAND <user> <password> <arguments>. See below.\n");
	//printf("You need to separate the commandLine into those components\n");
	//printf("For now, command, user, and password are hardwired.\n");

	char * tempCommand = (char*)malloc(MaxCommandLine*sizeof(char));
	char * tempUser = (char*)malloc(MaxCommandLine*sizeof(char));
	char * tempPassword = (char*)malloc(MaxCommandLine*sizeof(char));
	char * tempArgs = (char*)malloc(MaxCommandLine*sizeof(char));
	char * tempInput = (char*)malloc(MaxCommandLine*sizeof(char));
	strcpy(tempInput, commandLine);

	int position = 0;
	while(*tempInput != '\0' && *tempInput != ' ')
	{
		tempCommand[position] = *tempInput;
		tempInput ++;
		position ++;
	}
	position = 0;
	tempInput ++;
	while(*tempInput != '\0' && *tempInput != ' ')
	{
		tempUser[position] = *tempInput;
		tempInput ++;
		position ++;
	}
	position = 0;
	tempInput ++;
	while(*tempInput != '\0' && *tempInput != ' ')
	{
		tempPassword[position] = *tempInput;
		tempInput ++;
		position ++;
	}
	position = 0;
	tempInput ++;
	tempArgs = tempInput;

	const char * command = tempCommand;
	const char * user = tempUser;
	const char * password = tempPassword;
	const char * args = tempArgs;

	printf("command=%s\n", command);
	printf("user=%s\n", user);
	printf( "password=%s\n", password );
	printf("args=%s\n", args);

	if (!strcmp(command, "ADD-USER")) {
		addUser(fd, user, password, args);
	}
	else if (!strcmp(command, "ENTER-ROOM")) {
		enterRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "LEAVE-ROOM")) {
		leaveRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "SEND-MESSAGE")) {
		sendMessage(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-MESSAGES")) {
		getMessages(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-USERS-IN-ROOM")) {
		getUsersInRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-ALL-USERS")) {
		getAllUsers(fd, user, password, args);
	}
	else if (!strcmp(command, "CREATE-ROOM")) {
		createRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "LIST-ROOMS")) {
		listRooms(fd, user, password, args);
	}
	else {
		const char * msg =  "UNKNOWN COMMAND\r\n";
		write(fd, msg, strlen(msg));
	}

	// Send OK answer
	//const char * msg =  "OK\n";
	//write(fd, msg, strlen(msg));
	close(fd);	
}

void
IRCServer::initialize()
{
	// Open password file

	// Initialize users in room
	userList.head = NULL;
	roomList.head = NULL;
	// Initalize message list
	messList.head = NULL;
	messCount = 0;
}

bool
IRCServer::checkPassword(int fd, const char * user, const char * password) {
	// Here check the password
	Person * e = userList.head;
	while (e->next != NULL)
	{
		if (strcmp(e->username, user) == 0)
		{
			if (strcmp(e->password, password) == 0)
			{	
				return true;
			}
		}
		e = e->next;
	}
	if (strcmp(e->username, user) == 0)
	{
		if (strcmp(e->password, password) == 0)
		{
			return true;
		}
	}

	return false;
}

bool
IRCServer::checkRoom(int fd, const char * user, const char * password, const char * args) 
{
	Room * e;
	if (roomList.head == NULL)
	{
		return false;
	}
	e = roomList.head;
	while (e->next != NULL)
	{
		if (strcmp(args, e->roomName) == 0)
		{
			return true;
		}
		e = e->next;
	}
	if (strcmp(args, e->roomName) == 0)
	{
		return true;
	}
	return false;
}

bool
IRCServer::checkUserInRoom(int fd, const char * user, const char * password, const char * args)
{
	Person * e;
	Room * f;
	if (userList.head == NULL)
	{
		return false;
	}
	e = userList.head;
	while(e->next != NULL)
	{
		if (strcmp(e->username, user) == 0)
		{
			f = e->roomIn;
			if (f != NULL)
			{
				while (f->next != NULL)
				{
					if (strcmp(f->roomName, args) == 0)
					{
						return true;
					}
					f = f->next;
				}
				if (strcmp(f->roomName, args) == 0)
				{
					return true;
				}
			}
		}
		e = e->next;
	}
	if (strcmp(e->username, user) == 0)
        {
		f = e->roomIn;
		if (f != NULL)
		{
                	while (f->next != NULL)
                	{
	        	        if (strcmp(f->roomName, args) == 0)
                	        {
                	                return true;
                	        }
                	        f = f->next;
                	}
                	if (strcmp(f->roomName, args) == 0)
                	{
                	        return true;
                	}
		}
	}
}

void
IRCServer::addUser(int fd, const char * user, const char * password, const char * args)
{
	// ere add a new user. For now always return OK.
	Person * newUser = (Person*)malloc(sizeof(Person));
	newUser->password = password;
	newUser->username = user;
	newUser->args = args;
	newUser->roomIn = NULL;
	newUser->next = NULL;

	if (userList.head == NULL)
	{
		userList.head = newUser;
		const char * msg =  "OK\r\n";
		write(fd, msg, strlen(msg));
		return;		
	}
	if (strcmp(user, userList.head->username) < 0)
	{
		newUser->next = userList.head;
		userList.head = newUser;
		const char * msg =  "OK\r\n";
                write(fd, msg, strlen(msg));
                return;
	}
	Person * tempUser = userList.head;
	while (tempUser->next != NULL)
	{
		if (strcmp(user, tempUser->next->username) < 0)
		{
			newUser->next = tempUser->next;
			tempUser->next = newUser;
			const char * msg = "OK\r\n";
		        write(fd, msg, strlen(msg));
			return;
		}
		tempUser = tempUser->next;
	}
	tempUser->next = newUser;
	const char * msg = "OK\r\n";
	write(fd, msg, strlen(msg));
	return;
}

void
IRCServer::enterRoom(int fd, const char * user, const char * password, const char * args)
{
        Person * e;
        if (userList.head == NULL)
        {
                const char * msg = "DENIED (NO USERS).\r\n";
                write(fd, msg, strlen(msg));
                return;
        }
        if (!checkPassword(fd, user, password))
        {
                const char * msg = "ERROR (Wrong password)\r\n";
                write(fd, msg, strlen(msg));
                return;
        }

	char * roomName = (char*)args;
	if (!checkRoom(fd, user, password, args))
	{
		const char * msg = "ERROR (No room)\r\n";
                write(fd, msg, strlen(msg));
                return;
	}
	
	Room * f = (Room*)malloc(sizeof(Room));
	f->next = NULL;
	f->roomName = roomName;
	f->messCount = 0;

	e = userList.head;
	while (e->next != NULL)
	{
		if (strcmp(user, e->username) == 0)
		{
			if (e->roomIn == NULL)
			{
				e->roomIn = f;
			}
			else if (strcmp(e->roomIn->roomName, roomName) == 0)
			{
			}
			else
			{
				e->roomIn->next = f;
			}
			const char * msg = "OK\r\n";
        		write(fd, msg, strlen(msg));
        		return;
		}
		e = e->next;
	}
	if (strcmp(user, e->username) == 0)
	{
		if (e->roomIn == NULL)
		{
			e->roomIn = f;
		}
		else if (strcmp(e->roomIn->roomName, roomName) == 0)
                {
                }
                else
                {
                        e->roomIn->next = f;
                }
		const char * msg = "OK\r\n";
        	write(fd, msg, strlen(msg));
        	return;
	}
}

void
IRCServer::leaveRoom(int fd, const char * user, const char * password, const char * args)
{
	Person * e;
	Room * f;
	Room * r;
        if (userList.head == NULL)
        {
                const char * msg = "DENIED (NO USERS).\r\n";
                write(fd, msg, strlen(msg));
                return;
        }
        if (!checkPassword(fd, user, password))
        {
                const char * msg = "ERROR (Wrong password)\r\n";
                write(fd, msg, strlen(msg));
                return;
        }
	if (!checkRoom(fd, user, password, args))
	{
		const char * msg = "Error (Room DNE)\r\n";
                write(fd, msg, strlen(msg));
                return;
	}
	if (!checkUserInRoom(fd, user, password, args))
	{
		const char * msg = "ERROR (No user in room)\r\n";
                write(fd, msg, strlen(msg));
                return;
	}
	f = roomList.head;
	while(f->next != NULL)
	{
		if (strcmp(f->roomName, args) == 0)
		{
			break;
		}
		f = f->next;
	}

	e = userList.head;
	while (e->next != NULL)
	{
		if (strcmp(e->username, user) == 0)
		{
			while (e->roomIn->next != NULL)
			{
				if (strcmp(e->roomIn->roomName, args) == 0)
				{
					e->roomIn = NULL;
					const char * msg = "OK\r\n";
		        		write(fd, msg, strlen(msg));
		        		return;
				}
				e->roomIn = e->roomIn->next;
			}
			if (strcmp(e->roomIn->roomName, args) == 0)
                        {
                                e->roomIn = NULL;
                                const char * msg = "OK\r\n";
                                write(fd, msg, strlen(msg));
                                return;
                        }
		}
		e = e->next;
	}
	if (strcmp(e->username, user) == 0)
	{
                while (e->roomIn->next != NULL)
                {
                        if (strcmp(e->roomIn->roomName, args) == 0)
                        {
                                e->roomIn = NULL;
                                const char * msg = "OK\r\n";
                                write(fd, msg, strlen(msg));
                                return;
                        }
                        e->roomIn = e->roomIn->next;
                }
                if (strcmp(e->roomIn->roomName, args) == 0)
                {        
                        e->roomIn = NULL;
                        const char * msg = "OK\r\n";
                        write(fd, msg, strlen(msg));
                        return;
                }
	}
}

void
IRCServer::sendMessage(int fd, const char * user, const char * password, const char * args)
{
        Person * e;
        if (userList.head == NULL)
        {
                const char * msg = "DENIED (NO USERS).\r\n";
                write(fd, msg, strlen(msg));
                return;
        }
        if (!checkPassword(fd, user, password))
        {
                const char * msg = "ERROR (Wrong password)\r\n";
                write(fd, msg, strlen(msg));
                return;
        }
	e = userList.head;
	while (e->next != NULL)
	{
		if (strcmp(user, e->username) == 0)
		{
			break;
		}
		e = e->next;
	}

	char * tempRoom = (char*)malloc(sizeof(char)*100);
	char * roomName = tempRoom;
	char * tempMessage = (char*)malloc(sizeof(char)*1000);
	char * tempArgs = (char*)args;
	while (*tempArgs != ' ' && *tempArgs != '\0')
	{
		*tempRoom = *tempArgs;
		tempArgs++;
		tempRoom++;
	}
	tempArgs ++;
	tempMessage = tempArgs;

	Message * newMessage = (Message*)malloc(sizeof(Message));
	newMessage->message = tempMessage;
	newMessage->messFrom = e;
	newMessage->room = roomName;
	newMessage->next = NULL;

	Room * r;
	if (roomList.head == NULL)
	{
		const char * msg = "ERROR (NO ROOMS)\r\n";
                write(fd, msg, strlen(msg));
                return;
	}
	r = roomList.head;
	while (r->next != NULL)
	{
		if (strcmp(r->roomName, roomName) == 0)
		{
			newMessage->messNum = r->messCount;
			r->messCount ++;
		}
		r = r->next;
	}
	if (strcmp(r->roomName, roomName) == 0)
	{
		newMessage->messNum = r->messCount;
		r->messCount ++;
	}

	if (!checkUserInRoom(fd, user, password, newMessage->room))
	{
		const char * msg = "ERROR (user not in room)\r\n";
                write(fd, msg, strlen(msg));
                return;
	}
	Message * f;
	if (messList.head == NULL)
	{
		newMessage->next;
		messList.head = newMessage;
		const char * msg = "OK\r\n";
        	write(fd, msg, strlen(msg));
        	return;
	}
	f = messList.head;
	while (f->next != NULL)
	{
		f = f->next;
	}
	f->next = newMessage;

	const char * msg = "OK\r\n";
	write(fd, msg, strlen(msg));
	return;
}

void
IRCServer::getMessages(int fd, const char * user, const char * password, const char * args)
{
	Person * e;
        if (userList.head == NULL)
        {
                const char * msg = "ERROR (No users)\r\n";
                write(fd, msg, strlen(msg));
                return;
        }
        if (!checkPassword(fd, user, password))
        {
                const char * msg = "ERROR (Wrong password)\r\n";
                write(fd, msg, strlen(msg));
                return;
        }
        e = userList.head;
	while (e->next != NULL)
	{
		if (strcmp(e->username, user) == 0)
		{
			break;
		}
		e = e->next;
	}

	int tempMessCount = atoi(args);
	while (*args != ' ')
	{
		args++;
	}
	args ++;

	char * roomName = (char*)args;

	if (!checkUserInRoom(fd, user, password, roomName))
        {
                const char * msg = "ERROR (User not in room)\r\n";
                write(fd, msg, strlen(msg));
                return;
        }

	Message * m;
	Message * n;
	int lastMessNum = 0;
	if (messList.head == NULL)
	{
		const char * msg = "NO-NEW-MESSAGES\r\n";
                write(fd, msg, strlen(msg));
                return;
	}
	m = messList.head;
	n = messList.head;
	while (n->next != NULL)
	{
		if (strcmp(n->room, roomName) == 0)
		{
			lastMessNum = n->messNum;
		}
		n = n->next;
	}
	if (strcmp(n->room, roomName) == 0)
	{
		lastMessNum = n->messNum;
	}

	if (lastMessNum < tempMessCount)
	{
		const char * msg = "NO-NEW-MESSAGES\r\n";
                write(fd, msg, strlen(msg));
                return;
	}

	while (m->next != NULL)
	{
		if (strcmp(m->room, roomName) == 0 && tempMessCount <= m->messNum)
		{
			char * out = (char*)malloc(sizeof(char)*1000);

			char buffer[32];
			sprintf(buffer, "%d ", m->messNum);
			char * msg1 = (char*)buffer;
			char * msg2 = (char*)m->messFrom->username;
                        char * msg3 = (char*)m->message;
			char * msg4 = (char*)"\r\n";

			strcat(out, msg1);
                	strcat(out, msg2);
                	strcat(out, " "); 
        	        strcat(out, msg3);
	                strcat(out, msg4);
			
			write(fd, out, strlen(out));
		}
		m = m->next;
	}
	if (strcmp(m->room, roomName) == 0 && tempMessCount <= m->messNum)
        {
		char * out = (char*)malloc(sizeof(char)*1000);
		
		char buffer[32];
		sprintf(buffer, "%d ", m->messNum);
                char * msg1 = (char*)buffer;
                char * msg2 = (char*)m->messFrom->username;
                char * msg3 = (char*)m->message;
                char * msg4 = (char*)"\r\n";
		
		strcat(out, msg1);
		strcat(out, msg2);
		strcat(out, " ");
		strcat(out, msg3);
		strcat(out, msg4);

		write(fd, out, strlen(out));
	}
	char * end = (char*)"\r\n";
        write(fd, end, strlen(end));
}

void
IRCServer::getUsersInRoom(int fd, const char * user, const char * password, const char * args)
{
	Person * e;
	Room * f;
        if (userList.head == NULL)
        {
                const char * msg = "DENIED (NO USERS).\r\n";
                write(fd, msg, strlen(msg));
                return;
        }
        if (!checkPassword(fd, user, password))
        {
                const char * msg = "ERROR (Wrong password)\r\n";
                write(fd, msg, strlen(msg));
                return;
        }
	e = userList.head;
	char * roomName = (char*)args;
	while (e->next != NULL)
	{
		f = e->roomIn;
		if (f != NULL)
		{
			while (f->next != NULL)
			{
				if (strcmp(f->roomName, roomName) == 0)
				{
					char * msg1 = (char*)e->username;
                			char * msg2 = (char*)"\r\n";
					write(fd, msg1, strlen(msg1));
                			write(fd, msg2, strlen(msg2));
				}
				f = f->next;
			}
			if (strcmp(f->roomName, roomName) == 0)
                	{
                	        char * msg1 = (char*)e->username;
                	        char * msg2 = (char*)"\r\n";
                	        write(fd, msg1, strlen(msg1));
                	        write(fd, msg2, strlen(msg2));
                	}
		}
		e = e->next;
	}
	f = e->roomIn;
	if (f != NULL)
	{
        	while (f->next != NULL)
        	{
        	        if (strcmp(f->roomName, roomName) == 0)
        	        {
        	                char * msg1 = (char*)e->username;
        	                char * msg2 = (char*)"\r\n";
        	                write(fd, msg1, strlen(msg1));
        	                write(fd, msg2, strlen(msg2));
        	        }
        	        f = f->next;
        	}
        	if (strcmp(f->roomName, roomName) == 0)
        	{
        	        char * msg1 = (char*)e->username;
        	        char * msg2 = (char*)"\r\n";
        	        write(fd, msg1, strlen(msg1));
        	        write(fd, msg2, strlen(msg2));
        	}
	}
	char * msg1 = (char*)"\r\n";
	write(fd, msg1, strlen(msg1));
}

void
IRCServer::getAllUsers(int fd, const char * user, const char * password,const  char * args)
{
	Person * e;
	if (userList.head == NULL)
	{
		const char * msg = "DENIED (NO USERS).\r\n";
		write(fd, msg, strlen(msg));
		return;
	}
	if (!checkPassword(fd, user, password))
	{
		const char * msg = "ERROR (Wrong password)\r\n";
		write(fd, msg, strlen(msg));
		return;
	}
	e = userList.head;
	while (e->next != NULL)
	{
		char * msg1 = (char*)e->username;
		char * msg2 = (char*)"\r\n";
		write(fd, msg1, strlen(msg1));
		write(fd, msg2, strlen(msg2));
		e = e->next;
	}
	char * msg1 = (char*)e->username;
	char * msg2 = (char*)"\r\n\r\n";
	write(fd, msg1, strlen(msg1));
	write(fd, msg2, strlen(msg2));
	return;
}

void
IRCServer::createRoom(int fd, const char * user, const char * password,const  char * args)
{
	if (userList.head == NULL)
        {
                const char * msg = "DENIED (NO USERS).\r\n";
                write(fd, msg, strlen(msg));
                return;
        }
        if (!checkPassword(fd, user, password))
        {
                const char * msg = "ERROR (Wrong password)\r\n";
                write(fd, msg, strlen(msg));
                return;
        }
	Room * newRoom = (Room*)malloc(sizeof(Room));
	newRoom->roomName = args;
	newRoom->messCount = 0;
	newRoom->next = NULL;
	if (roomList.head == NULL)
	{
		roomList.head = newRoom;
		const char * msg = "OK\r\n";
        	write(fd, msg, strlen(msg));
		return;
	}
	newRoom->next = roomList.head;
	roomList.head = newRoom;
	
	const char * msg = "OK\r\n";
        write(fd, msg, strlen(msg));
        return;
}


void
IRCServer::listRooms(int fd, const char * user, const char * password,const  char * args)
{
	
}
