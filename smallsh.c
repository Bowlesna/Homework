/*
 *
 *
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


struct inputs{
	char *full_input;	
	char **arg;
	int background;
	int ignore;
	int count;
	char *output;
	char *input;
};

struct children_list{
	int *child_pid;
	int child_count;
};


void handle_SIGINT(int signo) {
	
}

void handle_SIGTSTP(int signo) {
	char *message = "Entering foreground only mode\n :";
	write(STDOUT_FILENO,message, 32);
		
}


void exit_handling() {
	//kill other processess
	

	return;
}

void cd_handling(struct inputs *commands) {
	if (commands->count == 1) {
		chdir(getenv("PWD"));		
	}
	else {
		//absolute

			
		//relative	
		chdir((commands->arg)[1]);
	}

	return;
}

void status_handling() {
	
	
	return;
}

void run_process_background(struct inputs *commands, struct children_list *children) {
	int output_fd;
	int input_fd;
	int result;

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
		
		//redirection
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

			//redirect the 
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

void run_process_foreground(struct inputs *commands) {
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
		


		//redirection
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
		perror("execve");
		exit(2);
		break;
	default:
		spawn_pid = waitpid(spawn_pid, &child_status, 0);
		if (WIFSIGNALED(child_status)){
			printf("child %i killed by signal %i\n",spawn_pid,WTERMSIG(child_status));
		}
		
	}	

	return;
}

void run_process(struct inputs *commands, struct children_list *children) {

	//split between foreground and background
	if (commands->background) {
		run_process_background(commands, children);
	}
	else {
		run_process_foreground(commands);
	}
		
}

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
	if (!strcmp(((command->arg)[0]),"#")) {
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

	//check for inputs and outputs
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
	temp = strcmp(((command->arg)[(command->count)-1]),"$");	
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
	//else we don't need to alter command

	/*
	printf("output: %s\n",command->output);
	printf("input: %s\n", command->input);
	*/
	//printf("background: %i\n", command->background);
	return command;
}

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
	

	//checker that inputs were read correctly
	/*
	int i;
	for(i = 0; i < count; i++) {
		printf("%s\n",(command->arg)[i]);
	}
	*/

	return command;
}

 

void menu() {
	int loop;
	int i;
	int j;
	char *user_input;
	size_t max_size = 2049;
	struct inputs *commands;
	struct children_list *children;
	int child_status;

	children = malloc(sizeof(struct children_list));
	children->child_pid = malloc(sizeof(int)*500);
	children->child_count = 0;
	

	//menu loop
	do {
		//reset important variables for the loop
		loop = TRUE;
		child_status = -1;

		//check for children to kill
		for (i = 0; i < children->child_count; i++) {
			waitpid((children->child_pid)[i], &child_status, WNOHANG);
			//if a child was terminated, print what happened
			if (WIFEXITED(child_status)) {
				printf("\nbackground pid %i has been killed with status %d\n", (children->child_pid)[i], WEXITSTATUS(child_status));
				//shift around the child_pid array. First in a safe way, then in a panic way.
				if (children->child_count == 1) {
					(children->child_pid)[0] = 0;
					(children->child_count) = 0; 
					printf("\ndone\n)");
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

		if ((!(strcmp((commands->arg)[0],"exit")))){
			exit_handling();
			loop = FALSE;
		}
		else if ((!(strcmp((commands->arg)[0],"cd\n")) || !strcmp((commands->arg)[0],"cd"))) {
			cd_handling(commands);
		}
		else if (((!strcmp((commands->arg)[0],"status\n")) || !strcmp((commands->arg)[0],"status"))) {
			status_handling();
		}
		else if (commands->ignore) {
			
		}
		else {
			run_process(commands,children);
			//int i;
			//for (i = 0; i < commands->count; i++) {
			//	printf("%s\n",(commands->arg)[i]);
			//}	
		}

		free(user_input);
	} while(loop);

	//free the children
	free(children->child_pid);
	free(children);
	return;
}

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

