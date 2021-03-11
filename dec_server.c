#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define TRUE 1
#define FALSE 0

//Error function used for reporting issues
void error(const char *msg) {
	perror(msg);
	exit(1);
} 

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address, 
	int portNumber){

	// Clear out the address struct
	memset((char*) address, '\0', sizeof(*address)); 

	// The address should be network capable
	address->sin_family = AF_INET;
	// Store the port number
	address->sin_port = htons(portNumber);
	// Allow a client at any address to connect to this server
	address->sin_addr.s_addr = INADDR_ANY;
}

/* Input: length of message to read, socket to read from
 * Output: the message read from socket
 */
char *read_message(int length, int socket_fd) {
	char *message;
	char *temp;
	int message_read;
	int chars_read;
	int completed = 0;
	int mistake = FALSE;
	

	//initialize data
	message = malloc(sizeof(char)*(length+1));
	temp = malloc(sizeof(char)*(length+1));
	memset(message, '\0', length+1);
	memset(temp, '\0', length+1);

	//read data from the socket, leaving the \0 at end
	do {
		chars_read = recv(socket_fd, message, (length - completed), 0);
		if (chars_read < 0) {
			error("SERVER: ERROR reading from socket");
		}
		message_read = TRUE;
		if (chars_read < (length - completed)) {
			completed = completed + chars_read;
			strcat(temp,message);		
			mistake = TRUE;
			message_read = FALSE;
			memset(message, '\0', length+1);
		}
	} while(!message_read);

	//last step in case entire message not read triggers
	if (mistake) {
		strcat(temp, message);
		free(message);
		return temp;	
	}
	
	free(temp);
	return message;
}

/* Input: message to be sent, length of that message, and socket to send message to
 * Output: none
 */
void send_message(char *message, int length, int socket_fd) {
	int message_sent;
	int chars_written;
	int completed = 0;

	//send message loop
	do {
		chars_written = send(socket_fd, (message+completed), (length-completed), 0);
		message_sent = TRUE;
		//if message not full sent, begin to loop
		if (chars_written < (length - completed)) {
			message_sent = FALSE;
			completed = completed + chars_written;
		}
	} while(!message_sent);

	return;
}

/*child process actually handles the encoding of the message
 * Input: socket that the child is connected to
 * Output: none, coded message sent back to client
 */
void encode_message(int connection_socket)  {
	int i;
	int key_length;
	int code_length;
	int chars_read;
	char *key;
	char *code;
	char *plaintext;
	int temp1;
	int temp2;
	int temp3;

	//grab lengths of the messages
	chars_read = recv(connection_socket, &key_length, sizeof(int), 0);
	if (chars_read < 0) {
		error("\nSERVER: ERROR reading keylength\n");
	}
	chars_read = recv(connection_socket, &code_length, sizeof(int), 0);
	if (chars_read < 0) {
		error("\nSERVER: ERROR reading code length\n");
	}

	//read messages from socket
	key = read_message(key_length, connection_socket); 
	code = read_message(code_length, connection_socket);

	//encrypt the message
	plaintext = malloc(sizeof(char)*code_length+1);
	memset(plaintext, '\0', code_length+1);
	for (i = 0; i < code_length; i++) {
		temp1 = (((int)(code[i]))-65);
		temp2 = (((int)(key[i]))-65);
		if (temp2 == -33) {
			temp2 = 26;
		}
		if (temp1 == -33) {
			temp1 = 26;
		}
		temp3 = (temp1 - temp2);
		temp3 = (temp3%27+27)%27;
		temp3 = temp3 + 65;
		if (temp3 == 91) {
			temp3 = 32;
		}
		plaintext[i] = (char)(temp3);
	}
	plaintext[code_length] = '\0';

	//send the coded message back to the client
	send_message(plaintext, code_length, connection_socket);

	//free memory
	free(plaintext);	
}


// function to handle forking into client
void handle_forking(int connection_socket) {
	pid_t spawnpid = -10;
	int child_status;

	//fork function, as seen from proccess API exploration
	spawnpid = fork();
	switch(spawnpid) {
		case -1:
			perror("fork() failed!");
			exit(1);		
			break;
		case 0:
			//kid does stuff	
			encode_message(connection_socket);
			exit(0);
			break;
		default:
			//parent does other stuff
			//wait(&child_status);
			break;
	}
}

//main handles setting up the server and the general loop of connecting each client to a fork
/* Input: just a port numger
 * Note: never ends the loop
 */
int main(int argc, char *argv[]){
	int connectionSocket;
	int message;
	int check;
	struct sockaddr_in serverAddress, clientAddress;
	socklen_t sizeOfClientInfo = sizeof(clientAddress);

	// Check usage & args
	if (argc < 2) { 
		fprintf(stderr,"USAGE: %s port\n", argv[0]); 
		exit(1);
	} 

	// Create the socket that will listen for connections
	int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket < 0) {
		error("ERROR opening socket");
	}

	// Set up the address struct for the server socket
	setupAddressStruct(&serverAddress, atoi(argv[1]));

	// Associate the socket to the port
	if (bind(listenSocket,(struct sockaddr *)&serverAddress,sizeof(serverAddress)) < 0){
		error("ERROR on binding");
	}
	
	while(1) {
		// Start listening for connetions. Allow up to 5 connections to queue up
		listen(listenSocket, 5); 

		// Accept a connection, blocking if one is not available until one connects	
		// Accept the connection request which creates a connection socket
		connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); 
		if (connectionSocket < 0){
			error("ERROR on accept");
		}

		//check if correct server
		message = 0;
		check = send(connectionSocket, &message, sizeof(int), 0);	
		if (check < 0) {
			error("ERROR, failed to send identification int");
		}
		check = recv(connectionSocket, &message, sizeof(int), 0);
		if (check < 0) {
			error("ERROR, failed to recieve identification int");
		}
		//if the client was correct, move to forking. Otherwise close socket.
		if (message == 1) {
			//fork connection
			handle_forking(connectionSocket);	
	
			close(connectionSocket); 
		}
		else {
			close(connectionSocket);
		}
	}
	// Close the listening socket
	close(listenSocket); 
	return 0;
}	

