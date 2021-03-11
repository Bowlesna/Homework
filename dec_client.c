#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()

#define TRUE 1
#define FALSE 0

/**
 * * Client code
 * * 1. Create a socket and connect to the server specified in the command arugments.
 * * 2. Prompt the user for input and send that input as a message to the server.
 * * 3. Print the message received from the server and exit the program.
 * */

// Error function used for reporting issues
void error(const char *msg) { 
	perror(msg); 
	exit(0); 
} 

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, 
		int portNumber, 
		char* hostname){

	// Clear out the address struct
	memset((char*) address, '\0', sizeof(*address)); 

	// The address should be network capable
	address->sin_family = AF_INET;
	// Store the port number
	address->sin_port = htons(portNumber);

	// Get the DNS entry for this host name
	struct hostent* hostInfo = gethostbyname(hostname); 
	if (hostInfo == NULL) { 
		fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
		exit(0); 
	}
	// Copy the first IP address from the DNS entry to sin_addr.s_addr
	memcpy((char*) &address->sin_addr.s_addr, 
			hostInfo->h_addr_list[0],
			hostInfo->h_length);

}

/* input: file to read the line from
 * output: the message from the file
 */
char *read_file(char *file) {
	char *message;
	char *buffer = NULL;
	FILE *fp;
	size_t len = 0;
	int length;

	//open file
	fp = fopen(file, "r");
	if (fp == NULL) {
		error("invalid file");
	}

	//grab message
	length = getline(&buffer, &len, fp);
	message = malloc(sizeof(char)*length);
	strcpy(message,buffer);

	//strip off newline
	message[length-1] = '\0';

	//free data
	free(buffer);

	//close file
	fclose(fp);
	
	return message;
}

/* Input: length of message to be recieved  and socket
 * Output: None
 * Note: reads the message from the socket and sends it to stdout
 */

void read_message(int length, int socketFD) {
	char message[length+1];
	char temp[length+1];
	int chars_read;
	int completed = 0;
	int mistake = FALSE;
	int message_read;	

	//initialize data
	memset(message, '\0', length+1);
	memset(temp, '\0', length+1);	

	//Read data from the socket, leaving \0 at end
	do {
		chars_read = recv(socketFD, message, length - completed, 0); 
		if (chars_read < 0){                                       
			error("CLIENT: :) ERROR reading from socket");
		}
		message_read = TRUE;
		//if not entire message read, begin to loop 
		if (chars_read < (length - completed)) {
			completed = completed + chars_read;
			strcat(temp, message);
			mistake = TRUE;			
			message_read = FALSE;
			
		}
	} while(!message_read);

	//last step in case entire message read triggers
	if (mistake) {
		strcat(temp, message);
		printf("%s\n", temp);
		return;
	}
	

	printf("%s\n", message);

	return;
}


/* Input: message to be sent, length of the message, and the socket being written to
 * Output: none
 * Note: just does the loop to make sure every part of the message was sent
 */
void send_message(char *message, int length, int socketFD){
	int message_sent;
	int chars_written;
	int completed = 0;

	//send message loop
	do{
		chars_written = send(socketFD, message+completed, (length-completed), 0); 
		message_sent = TRUE;
		//if not full message is sent, begin to loop
		if (chars_written < (length - completed)){
			completed = completed + length;
			message_sent = FALSE;
		}
	} while (!message_sent);

	return;
}

/* Input: plaintext, key, port
 * Output: the coded plaintext according to the key (sent to stdout)
 * Note: General outline from the client.c
 */

int main(int argc, char *argv[]) {
	int socketFD, charsWritten;
	int i;
	char *code;
	char *key;
	int key_length;
	int code_length;
	int check;
	struct sockaddr_in serverAddress;

	// Check usage & args
	if (argc < 4) { 
		fprintf(stderr,"USAGE: %s code key port\n", argv[0]); 
		exit(0); 
	} 

	//set variables
	code = read_file(argv[1]);
	key = read_file(argv[2]);
	
	//error check the key and plaintext
	//is the key long enough	
	if (strlen(code) > strlen(key)) {
		error("Error: key is not long enough to encrypt this message.\n");
	}

	//does code contain the correct values
	for (i = 0; i < strlen(code); i++) {
		check = (int)code[i];
		if (!(((check > 64)  && (check < 91 )) || (check == 32))) {
			error("Error: input contains bad character");
		}	
	}

	// Create a socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); 
	if (socketFD < 0){
		error("CLIENT: ERROR opening socket");
	}

	// Set up the server address struct
	setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
		error("CLIENT: ERROR connecting");
	}

	//send message lengths
	key_length = strlen(key);
	code_length = strlen(code);
	charsWritten = send(socketFD, &key_length, sizeof(int), 0);
	if (charsWritten < 0) {
		error("CLIENT: ERROR sending key_length");
	}
	charsWritten = send(socketFD, &code_length, sizeof(int), 0);
	if (charsWritten < 0) {
		error("CLIENT: ERROR sending code length");
	}

	//send the key and plaintext to server
	send_message(key, key_length, socketFD);
	send_message(code, code_length, socketFD);
		
	// Get back the coded message
	// Clear out the buffer again for reuse
	read_message(code_length, socketFD);

	//free data
	free(code);
	free(key);

	// Close the socket
	close(socketFD); 
	return 0;
}
