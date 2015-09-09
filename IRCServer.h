
#ifndef IRC_SERVER
#define IRC_SERVER

#define PASSWORD_FILE "password.txt"

class IRCServer {
	// Add any variables you need
	struct Room {
                const char * roomName;
                int messCount;
                Room * next;
        };
	typedef struct Room Room;

	struct LLRooms {
                Room * head;          
        };
        typedef struct LLRooms LLRooms;

	struct Person {
		const char * password;
		const char * username;
		const char * args;
		Room * roomIn;
		struct Person * next;
	};
	typedef struct Person Person;

	struct LLUsers {
		Person * head;
	};
	typedef struct LLUsers LLUsers;

	struct Message {
		const char * message;
		const char * room;
		int messNum;
		Person * messFrom;
		Message * next;
	};
	typedef struct Message Message;
	
	struct LLMessage {
		Message * head;
	};
	typedef struct LLMessage LLMessage;

private:
	int open_server_socket(int port);
	LLUsers userList;
	LLMessage messList;
	LLRooms roomList;
	int messCount;

public:
	void initialize();
	bool checkPassword(int fd, const char * user, const char * password);
	void processRequest( int socket );
	void addUser(int fd, const char * user, const char * password, const char * args);
	void enterRoom(int fd, const char * user, const char * password, const char * args);
	void leaveRoom(int fd, const char * user, const char * password, const char * args);
	void sendMessage(int fd, const char * user, const char * password, const char * args);
	void getMessages(int fd, const char * user, const char * password, const char * args);
	void getUsersInRoom(int fd, const char * user, const char * password, const char * args);
	void getAllUsers(int fd, const char * user, const char * password, const char * args);
	void createRoom(int fd, const char * user, const char * password, const char * args);
	void listRooms(int fd, const char * user, const char * password, const char * args);
	bool checkRoom(int fd, const char * user, const char * password, const char * roomName);
	bool checkUserInRoom(int fd, const char * user, const char * password, const char * args);
	void runServer(int port);
};

#endif
