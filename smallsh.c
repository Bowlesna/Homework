/* FILENAME: smallsh.c
 * Author: Nathnael Bowles
 * Description: running this code will run a basic shell. the three built in commands are ls, status, and exit. You can comment with #.
 * otherwise, the comment will be run through execv. 
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0


//I tried everything I could to avoid global variables, but I couldn't figure out how to handle SIGTSTP alternating functions
//so here we are.
int SIGTSTP_mode;


//Struct for holding user commands and related info
struct inputs{
	char *full_input;	
	char **arg;
	int background;
	int ignore;
	int count;
	char *output;
	char *input;
};

//Struct for carrying around the list of background children running. 
//Used in menu() for checking if children need to be killed
struct children_list{
	int *child_pid;
	int child_count;
};

//filler SIGINT handler function I am going to forget to delete
void handle_SIGINT(int signo) {
	//filler	
}


//Signal handler for SIGTSTP. USes the SIGTSTP_mode global variable to switch foreground only mode on and off
void handle_SIGTSTP(int signo) {
	if (!SIGTSTP_mode) {
		char *message = "Entering foreground only mode\n\n:";
		write(STDOUT_FILENO,message, 33);		
		SIGTSTP_mode = TRUE;
	}
	else {
		char *message = "Exiting foreground only mode\n\n:";
		write(STDOUT_FILENO,message, 32);
		SIGTSTP_mode = FALSE;		

	}	
}

/* Input: None
 * Output: when tis is called, finishes killing any children and exits the program
 * Note: only called from menu()
 */
void exit_handling(struct children_list *children) {
	int i;
	//kill other processess
	for (i = 0; i < children->child_count; i++) {
		kill((children->child_pid)[i], SIGKILL);
	}

	return;
}

/* Input: One inputs struct, contains all the information needed to change directory
 * Output: None, only changes the directory
 */
void cd_handling(struct inputs *commands) {
	if (commands->count == 1) {
		chdir(getenv("HOME"));		
	}
	else {	
		//relative	
		chdir((commands->arg)[1]);
	}

	return;
}

/* Input: A pointer to the status output of the most recent wait() command from run foreground process
 * Output: posts to STDOUT the status number of what terminated the most recent foreground process
 */
void status_handling(int* recent_foreground) {
	int status;


	if (recent_foreground == NULL) {
		//no foreground proccesses ran, default to 0
		
		printf("\nexit value: 0\n");
		fflush(stdout);
	}	
	else {	
		//check last foreground process
		if (WIFEXITED(*recent_foreground)) {
			status = WEXITSTATUS(*recent_foreground);	
		}
		else if (WIFSIGNALED(*recent_foreground)){
			status = WTERMSIG(*recent_foreground);
		}
		else {
			printf("\n Error, foreground child somehow not terminated");
			fflush(stdout);
		}

		printf("\nexit value: %i\n", status);
		fflush(stdout);	
	}
	
	return;
}

/* Input: inputs struct holding the processed user input, the list of to-be terminated background children
 * Output: Runs the command specificed in comamnds in the background, saves this new child's pid to the children list
 */

void run_process_background(struct inputs *commands, struct children_list *children) {
	int output_fd;
	int input_fd;
	int result;
	struct sigaction SIGTSTP_action = {{0}};

	//prep the command to work with execv
	char temp[500] = "/bin/\0";
	strcat(temp,(commands->arg)[0]);
	
	//fork and execute, copied from eploration: process API
	pid_t spawn_pid = fork();
	switch(spawn_pid) {
	case -1:
		perror("fork()\n");
		exit(1);
		break;
	case 0: 
		//reset SIGTSTP handling
		SIGTSTP_action.sa_handler = SIG_IGN;
		sigaction(SIGTSTP, &SIGTSTP_action, NULL);

		//giant if/else statments dealing with redirecting to chose file OR to /dev/null if not specified
		//This is for both input and output
		if ((commands->output) != NULL) {
			//open the output files	
			output_fd = open((commands->output), O_WRONLY | O_TRUNC | O_CREAT, 0640);
			if (output_fd == -1) {
				printf("cannot open %s for output\n",commands->output);
				//set exit status to
				exit(1);
			}	
	
			//redirect the output file
			result = dup2(output_fd,1);
			if (result == -1) {
				printf("cannot redirect %s for output\n", commands->output);
				exit(2);
			}
		}
		else {
			//open the /dev/null
			output_fd = open("/dev/null", O_WRONLY);
			if (output_fd == -1) {
				printf("cannot redirect /dev/null for output");			
				exit(1);
			}
			//redirect output to /dev/null
			result = dup2(output_fd,1);
			if (result == -1) {
				printf("cannot redirect /dev/null for output\n");
				exit(2);
			}

		}
		if (commands->input != NULL) {
			//open the input files
			input_fd = open((commands->input), O_RDONLY);
			if (input_fd == -1) {
				printf("cannot open %s for input\n",commands->input);
				exit(1);
			}		

			//redirect the input files
			result = dup2(input_fd,0);
			if (result == -1) {		
				printf("cannot redirect %s for input\n", commands->input);
				exit(2);
			}
		}
		else {
			//open /dev/mull
			input_fd = open("/dev/null",O_RDONLY);		
			if (input_fd == -1) {
				printf("cannot open /dev/null for input");
				exit(1);
			}

			//redirect the to /dev/null 
			result = dup2(input_fd,0);
			if (result == -1) {
				printf("cannot redirect /dev/null for input\n");		
				exit(2);
			}	
		}


		//printf("child(%d) running %s command\n",getpid(),temp);
		execv(temp, (commands->arg));		
		perror("execv background failed");
		exit(2);
		break;
	default:
		//print process id of background process
		printf("background pid is %i", spawn_pid);
		(children->child_pid)[children->child_count] = spawn_pid;
		children->child_count++;

		break;
	}	

	return;
}

/* input: inputs strctu containing the processed user input, and a pointer to the value of the most recent foreground termination status
 * output: forks and runs a process and waits for it to terminate
 * Note: the switch part structure is copied from the exploration segment on forking and exec()ing children
 */
void run_process_foreground(struct inputs *commands, int *recent_foreground) {
	int child_status;
	int output_fd;
	int input_fd;
	int result;
	struct sigaction SIGINT_action = {{0}};


	//prep the command
	char temp[500] = "/bin/\0";
	strcat(temp,(commands->arg)[0]);
	
	//fork and execute, copied from eploration: process API
	pid_t spawn_pid = fork();
	switch(spawn_pid) {
	case -1:
		perror("fork()\n");
		exit(1);
		break;
	case 0: 
		//set signal handlers for foreground child
		SIGINT_action.sa_handler = SIG_DFL;
		sigaction(SIGINT, &SIGINT_action, NULL);

		//redirection for both input and output
		if ((commands->output) != NULL) {
			//open the output files	
			output_fd = open((commands->output), O_WRONLY | O_TRUNC | O_CREAT, 0640);
			if (output_fd == -1) {
				printf("cannot open %s for output\n",commands->output);
				//set exit status to
				exit(1);
			}	
	
			//redirect the output file
			result = dup2(output_fd,1);
			if (result == -1) {
				printf("cannot redirect %s for output\n", commands->output);
				exit(2);
			}
		}
		if (commands->input != NULL) {
			//open the input files
			input_fd = open((commands->input), O_RDONLY);
			if (input_fd == -1) {
				printf("cannot open %s for input\n",commands->input);
				exit(1);
			}		

			//redirect the input files
			result = dup2(input_fd,0);
			if (result == -1) {		
				printf("cannot redirect %s for input\n", commands->input);
				exit(2);
			}
		}


		//printf("child(%d) running %s command\n",getpid(),temp);
		execv(temp, (commands->arg));		
		printf("\n%s: no such file or directory\n", (commands->arg)[0]);
		exit(1);
		break;
	default:
		spawn_pid = waitpid(spawn_pid, &child_status, 0);
		if (WIFSIGNALED(child_status)){
			printf("child %i killed by signal %i\n",spawn_pid,WTERMSIG(child_status));
		}
		(*recent_foreground) = child_status;
	}	

	return;
}

/* input: inputs struct contaiing the processed user input, a children list struct to keep track of background processes, and a pointer to the most recent foreground process termination
 * Output: none, simply calls between running the process in the background or foreground
 */

void run_process(struct inputs *commands, struct children_list *children, int *recent_foreground) {
	//split between foreground and background
	if ((commands->background) && (!SIGTSTP_mode)) {
		run_process_background(commands, children);
	}
	else {
		run_process_foreground(commands, recent_foreground);
	}
}

/* Input: input struct containing the processed and organized user input
 * Output: A inputs struct ready to be carried out
 * Note: takes care of checking if the command is a background/foreground rpocess, if it is a comment, and any redirection	
 */

struct inputs *interpret_input(struct inputs *command) {
	char **arguments;
	char *check;
	int max_size = 514;
	int temp;
	int i;

	//initialze the variables
	(command->input) = NULL;
	(command->output) = NULL;
	(command->background) = FALSE;	
	(command->ignore) = FALSE;

	//check if this command should be ignored (either a comment or empty)
	if (*((command->arg)[0]) == '#') {
		command->ignore = TRUE;
		return command;
	}
	if (!strcmp(((command->arg)[0]),"\0")) {
		command->ignore = TRUE;
		return command;
	}

	//grab the actual command
	arguments = malloc(sizeof(char)*max_size);
	arguments[0] = (command->arg)[0];

	//check for inputs and outputs to account for
	if ((command->count) > 2) {
		check = (command->arg)[1];	

		if (!strcmp(check,"<")) {
			(command->input) = (command->arg)[2]; 
		}
		else if (!strcmp(check,">")) {
			(command->output) = (command->arg)[2];
		}
	}
	if ((command->count) > 4) {
		check = (command->arg)[3];	

		if (!strcmp(check,"<")) {
			(command->input) = (command->arg)[4]; 
		}
		else if (!strcmp(check,">")) {
			(command->output) = (command->arg)[4];
		}

	}

	//check if it is a background process
	temp = strcmp(((command->arg)[(command->count)-1]),"&");	
	if (!temp) {
		command->background = TRUE;
		(command->arg)[(command->count)-1] = NULL;
		(command->count)--;
	} 
	

	//piece the argument back together
	if ((command->input != NULL) && (command->output != NULL)) {
		for(i = 5; i <= (command->count); i++) {
			arguments[i-4] = (command->arg)[i];
		}		
		command->count = command->count - 4;
		command->arg = arguments;
	}
	else if ((command->input != NULL) || (command->output != NULL)) {
		for (i = 3; i <= (command->count); i++) {
			arguments[i-2] = (command->arg)[i];
		} 
		command->count = command->count - 2;
		command->arg = arguments;
	}
	//else we don't need to alter commands
	
	return command;
}

/* Input: an inputs struct that has been organized
 * Output: an inputs struct that has been organized and processed
 * Note: This function takes care of the variable expansion part of the project 
 */
struct inputs *process_input(struct inputs *commands) {
	int i;
	char *str_location;
	char *temp_str;
	char *saveptr = NULL;
	char *token;

	//retrieve pid
	int pid = getpid();
	char pid_s[300];
	sprintf(pid_s, "%d", pid);

	//expansion of variables
	for(i = 0; i < commands->count; i++) {
		str_location = strstr((commands->arg)[i],"$$");	
		if (str_location != NULL) {
			saveptr = NULL;
			temp_str = calloc(600,sizeof(char));

			//before expansion
			token = strtok_r((commands->arg)[i],"$", &saveptr);
			if (token != NULL) {
				strcpy(temp_str,token);
			}

			//add pid
			strcat(temp_str,pid_s);
			saveptr++;		

			//add the rest				
			token = strtok_r(NULL,"\0", &saveptr);
			if (token != NULL) {
				strcat(temp_str,token);
			}

			//re-add the string
		        //free((commands->arg)[i]);
			strcpy((commands->arg)[i], temp_str);

			//decrement to check if there are any more expansions needed in the word
			i--;
		}
	} 

	return commands;
}

/* Input: A c string containing the user input
 * Output: a inputs struct with the args variable filled out
 * Note: breaks up the c string into parts, changes the newline from getline() to a null terminator, and adds a NULL to the end for when the process is executed
 */
struct inputs *organize_input(char *user_input) {
	char *token;
	char *saveptr = NULL;
	char *temp;
	int max_size = 514;
	int count = 0;
	temp = malloc(200*sizeof(char));

	//create struct
	struct inputs *command;
	command = malloc(sizeof(struct inputs));
	command->arg = malloc(max_size*sizeof(char));

	command->full_input = user_input;

	//process line, initial
	token = strtok_r(user_input," ", &saveptr);
	(command->arg)[count] = calloc(strlen(token)+1,sizeof(char));
	strcpy((command->arg)[count],token);

	//printf("%s\n",token);
	count++;		
	//the rest
	while((token = strtok_r(NULL," ", &saveptr)) != NULL) {
		(command->arg)[count] = calloc(strlen(token)+1,sizeof(char));
		strcpy((command->arg)[count],token);	
	  	//printf("%s\n",token);	
		count++;	
	}

	//remove the \n from the final command
	strcpy(temp, (command->arg)[count-1]);
	temp[strlen((command->arg)[count-1])-1] = '\0';
	strcpy((command->arg)[count-1],temp);

	//keep track of length
	command->count = count;

	//add random NULL to the end
	(command->arg)[count] = NULL;
	

	return command;
}

 /* Input: none
 *  Output: None
 * Note: Menu() takes care of all the looping of prompting and taking user inputs
 * Included is also also the loop to kill any zombie background children
 */
void menu() {
	int loop;
	int i;
	int j;
	char *user_input;
	size_t max_size = 2049;
	struct inputs *commands;
	struct children_list *children;
	int child_status;
	int *recent_foreground = malloc(sizeof(int));

	children = malloc(sizeof(struct children_list));
	children->child_pid = malloc(sizeof(int)*500);
	children->child_count = 0;
	

	//menu loop
	do {
		//reset important variables for the loop
		loop = TRUE;
		child_status = -1;

		//check for children to kill, no zombies allowed. note to self to put this in its own function in the future.
		for (i = 0; i < children->child_count; i++) {
			//reset important variables for this loop
			child_status = -1;

			waitpid((children->child_pid)[i], &child_status, WNOHANG);
			//if a child was terminated, print what happened. Do this for both normal and abnormal terminations
			if (WIFEXITED(child_status)) {
				printf("\nbackground pid %i has been killed with status %d\n", (children->child_pid)[i], WEXITSTATUS(child_status));
				//shift around the child_pid array. First in a safe way, then in a panic way.
				if (children->child_count == 1) {
					(children->child_pid)[0] = 0;
					(children->child_count) = 0; 
				}
				else {
					for (j = i; j < children->child_count; j++) {
						(children->child_pid)[j] = (children->child_pid)[j+1];
					}
					(children->child_count)--;
					i--;
				}
			}
			else if(WIFSIGNALED(child_status)) {
				printf("\n background pid %i has been killed with status %d\n", (children->child_pid)[i], WTERMSIG(child_status));
				//shift around the child_pid array. first in a safe way, then in a panic way
				if (children->child_count == 1) {
					(children->child_pid)[0] = 0;
					(children->child_count) = 0; 
				}
				else {
					for (j = i; j < children->child_count; j++) {
						(children->child_pid)[j] = (children->child_pid)[j+1];
					}
					(children->child_count)--;
					i--;
				}
			}
		}
		//prompt user
		printf("\n: ");
		fflush(stdout);

		//accept input
		user_input = malloc(max_size*sizeof(char));
		getline(&user_input,&max_size,stdin);		

		//process input
		commands = organize_input(user_input);		
		commands = process_input(commands);
		commands = interpret_input(commands);

		// choose between the prebuilt functions, ignoring the command, and passing it to be excuted
		if ((!(strcmp((commands->arg)[0],"exit")))){
			exit_handling(children);
			loop = FALSE;
		}
		else if ((!strcmp((commands->arg)[0],"cd"))) {
			cd_handling(commands);
		}
		else if ((!strcmp((commands->arg)[0],"status"))) {
			status_handling(recent_foreground);
		}
		else if (commands->ignore) {
			//ignore the command
		}
		else {
			run_process(commands,children,recent_foreground);
		}
		//free user_input before next time
		free(user_input);
	} while(loop);

	//free the children
	free(children->child_pid);
	free(children);
	free(recent_foreground);
	return;
}

// Main function, just used to ready the signal handlers for the general process
// After that calls menu()
int main(){

	//in general, ignore sig_int
	struct sigaction SIGINT_action = {{0}};
	SIGINT_action.sa_handler = SIG_IGN;
	sigaction(SIGINT, &SIGINT_action, NULL);

	//set up SIGTSTP struct
	struct sigaction SIGTSTP_action = {{0}};
	SIGTSTP_action.sa_handler = handle_SIGTSTP;
	SIGTSTP_action.sa_flags = SA_RESTART;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);	

	//start fuction
	menu();
	
	return 0;
}

