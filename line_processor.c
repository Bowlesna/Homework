#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

#define MAX_input_length 1000
#define buffer_size 80

//mutex global variable to make my life easier
pthread_mutex_t m_input_seperator = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m_seperator_plus = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t m_plus_output = PTHREAD_MUTEX_INITIALIZER; 

//condition global variables to make my life easier
pthread_cond_t ready_input_seperator = PTHREAD_COND_INITIALIZER ;
pthread_cond_t ready_seperator_plus = PTHREAD_COND_INITIALIZER;
pthread_cond_t ready_plus_output = PTHREAD_COND_INITIALIZER;

/* Struct to act as the buffer for the following threads
 * holds the list of string inputs along with how many strings there are, how many have been consumed, 
 * and if STOP has been found and each thread should exit
 */
struct buffer{
	char **string;
	int string_count;
	int string_processed;
	int stop_found;
};

/* Input: User input to place into the buffer in the second argument
 * Output: None
 * Note: Specifically used by the input thread
 */
void temp_to_buffer(char *input, struct buffer *buffer) {
	//lock mutex
	pthread_mutex_lock(&m_input_seperator);
	//copy info to buffer, increment count
	strcpy(buffer->string[buffer->string_count], input);			
	buffer->string_count++;
	//signal that input_seperator buffer has values to be seperat
	pthread_cond_signal(&ready_input_seperator);
	//unlock mutex
	pthread_mutex_unlock(&m_input_seperator);
	return;
}

/* Input: the plus_output buffer
 * Output: none
 * Note: breaks the inputs into buffer_size pieces and prints them to string once enough are gathered
 */
void *output_thread(void *args) {
	int count;
	int stop_found;
	char *temp;	
	char *waitroom;
	struct buffer *plus_output = ((struct buffer *)args);

	//allocate memory for the temp string
	waitroom = malloc(sizeof(char)*MAX_input_length*50);
	temp = malloc(sizeof(char)*MAX_input_length);
	char *temp2 = malloc(sizeof(char)*MAX_input_length);

	strcpy(waitroom,"");	
	count = 0;
	stop_found = FALSE;
	//outer loop until STOP is found
	do {
		//inner loo for waiting until data needs to output
		//lack of producer mutex
		pthread_mutex_lock(&m_plus_output);
		while((plus_output->string_count) <= (plus_output->string_processed)) {
			if(plus_output->stop_found) {
				free(temp);
				free(waitroom);
				free(temp2);
				return NULL;
			}
			pthread_cond_wait(&ready_plus_output,&m_plus_output);
		}	

		//copy over string being processed for ease of access
		strcpy(temp, (plus_output->string)[plus_output->string_processed]);
	
		//increment which line has been processed
		((plus_output)->string_processed)++;	
	
		//unlock producer mutex
		pthread_mutex_unlock(&m_plus_output);	

		//process output
		//first add to waitroom
		strcat(waitroom,temp);
		
		//loop to empty waitroom in chunks specified by buffer_size
		while(strlen(waitroom+(count*buffer_size)) >= buffer_size) {
			strncpy(temp2, waitroom+(count*buffer_size), buffer_size);
			printf("%s\n", temp2);
			//fflush(stdout);
			count++;
		}		
		
	} while(!stop_found);

	return args;
}

/* Input: args contains the buffers between the seperator/plus threads and the plus/output threads
 * Output: none
 * Note: replaces ++ with ^ in the input strings
 */
void *plus_sign_thread(void *args) {
	int stop_found;
	int inner_loop;
	char *temp;
	char *string;
	char *saveptr;
	char *token;
	char *location;
	struct buffer **arguments = ((struct buffer**)args);
	struct buffer *seperator_plus = arguments[0];
	struct buffer *plus_output = arguments[1];

	//allocate memory for the temporary strings
	string = malloc(sizeof(char)*MAX_input_length);
	temp = malloc(sizeof(char)*MAX_input_length);
	stop_found = FALSE;
	location = NULL;

	//outerloop until STOP is found
	do {
		//inner loop for waiting until data needs to be processed
		//lock producer mutex
		pthread_mutex_lock(&m_seperator_plus);
		while((seperator_plus->string_count) <= (seperator_plus->string_processed)) {
			if(seperator_plus->stop_found) {
				free(string);
				free(temp);
				//let consumer know it's good to go
				pthread_mutex_lock(&m_plus_output);
				plus_output->stop_found = TRUE;
				pthread_cond_signal(&ready_plus_output);
				pthread_mutex_unlock(&m_plus_output);
				return NULL;
			}
			pthread_cond_wait(&ready_seperator_plus,&m_seperator_plus);	
		}	

		//copy over string being processed for ease of access
		strcpy(string, (seperator_plus->string)[seperator_plus->string_processed]);
	
		//increment which line has been processed
		((seperator_plus)->string_processed)++;	
	
		//unlock producer mutex
		pthread_mutex_unlock(&m_seperator_plus);	

		//now process temp
		//inner loop to find all cases of ++ in the string
		inner_loop = TRUE;
		do {
			saveptr = NULL;
			location = strstr(string, "++");
			if (location == NULL) {
				inner_loop = FALSE;
			}
			//backup case since strtok_r crashes if it is empty for ++ being the first part of an input
			else if (strncmp(string,"++",2) == 0) {
				strcpy(temp, "");
				strcat(temp, "^");
				token = strtok_r(string, "\0", &saveptr);
				strcat(temp,token+2);
				strcpy(string,temp);
			}
			else {
				token = strtok_r(string, "++", &saveptr);
				strcpy(temp, token);
				strcat(temp, "^");
				token = strtok_r(NULL, "\0", &saveptr);
				strcat(temp, token+1);
				strcpy(string, temp);	
			}
		} while(inner_loop);	

		//place seperated line in next buffer
		//lock next mutex
		pthread_mutex_lock(&m_plus_output);
		//copy string, increment buffer
		strcpy((plus_output->string)[plus_output->string_count], string);
		(plus_output->string_count)++;
		//signal and unlock mutex for consumer
		pthread_cond_signal(&ready_plus_output);
		pthread_mutex_unlock(&m_plus_output);			
	} while(!stop_found);

	//free data
	free(string);

	return args;
}

/* Input: argus carries the buffers between the input/seperator and seperator/plus sign threads
 * Output: None
 * Note: removes any new line characters and replaces them with spaces
 */
void *line_seperator_thread(void *args) {
	int stop_found;
	int i;
	char *temp;
	struct buffer **arguments = ((struct buffer**)args);
	struct buffer *input_seperator = arguments[0];
	struct buffer *seperator_plus = arguments[1];

	//allocate temp
	temp = malloc(sizeof(char)*MAX_input_length);

	//initialize the loop that doesn't really get used	
	stop_found = FALSE;

	//outerloop until STOP is found
	do {
		//inner loop for when data needs to be seperated, start by locking producer mutex
		pthread_mutex_lock(&m_input_seperator);
		while((input_seperator->string_count) <= (input_seperator->string_processed)) {
			if (input_seperator->stop_found) {
				free(temp);				
				//let consumer know it's good to go
				pthread_mutex_lock(&m_seperator_plus);
				seperator_plus->stop_found = TRUE;
				pthread_cond_signal(&ready_seperator_plus);
				pthread_mutex_unlock(&m_seperator_plus);
				return NULL;
			}
			pthread_cond_wait(&ready_input_seperator,&m_input_seperator);	
		}
		
		//copy over string being processed for ease of access
		strcpy(temp, (input_seperator->string)[input_seperator->string_processed]);
	
		//increment which line has been processed
		((input_seperator)->string_processed)++;	
	
		//unlock producer mutex
		pthread_mutex_unlock(&m_input_seperator);

		//remove new lines and replace with a space
		for (i = 0; i < strlen(temp); i++) {
			if (temp[i] == '\n') {
				//replace it
				temp[i] = ' ';
			}	
		}	

		//place seperated line in next buffer
		//lock next mutex
		pthread_mutex_lock(&m_seperator_plus);
		//copy string, increment buffer
		strcpy((seperator_plus->string)[seperator_plus->string_count], temp);
		(seperator_plus->string_count)++;
		//signal and unlock mutex for consumer
		pthread_cond_signal(&ready_seperator_plus);
		pthread_mutex_unlock(&m_seperator_plus);			
	} while(!stop_found);

	//
	return args;
}

/* Input: the buffer between input and seperator threads
 * Output: None
 * Note: reads from stdin and adds them to the buffer
 */
void *input_thread(void *args) {
	int stop_found;
	size_t max_size;
	char *input;
	struct buffer *input_seperator = ((struct buffer *)args);

	//allocate memory
	input = malloc(sizeof(char)*MAX_input_length);

	//initializ the needed values
	max_size = MAX_input_length;
	//outer loop until STOP processing line	
	do {
		stop_found = FALSE;
	
		//get user input
		getline(&input, &max_size, stdin);
		printf("");
		if (!strcmp(input, "STOP\n")) {
			stop_found = TRUE;
		}
		else {
			temp_to_buffer(input, input_seperator);	
		}
	} while(!stop_found);
	
	//let consumer know it's good to go
	pthread_mutex_lock(&m_input_seperator);
	input_seperator->stop_found = TRUE;
	pthread_cond_signal(&ready_input_seperator);
	pthread_mutex_unlock(&m_input_seperator);

	//free memory
	free(input);

	return args;

}

/* Input: none
 * Output: none
 * Note: sets up the buffers and calls the thread functions
 */
void setup() {
	int i;
	
	//decleare threads and buffers
	pthread_t input_t, seperator_t, plus_t, output_t;
	struct buffer *input_seperator, *seperator_plus, *plus_output;
	struct buffer **arg_2, **arg_3;

	//allocate memory to the above pointers
	input_seperator = malloc(sizeof(struct buffer));
	seperator_plus = malloc(sizeof(struct buffer));
	plus_output = malloc(sizeof(struct buffer));

	input_seperator->string = malloc(sizeof(char *)*50);
	seperator_plus->string = malloc(sizeof(char *)*50);
	plus_output->string = malloc(sizeof(char *)*50);
	
	for (i = 0; i < 50; i++) {
		(input_seperator->string)[i] = malloc(sizeof(char)*MAX_input_length);
		(seperator_plus->string)[i] = malloc(sizeof(char)*MAX_input_length);
		(plus_output->string)[i] = malloc(sizeof(char)*MAX_input_length);
	}

	arg_2 = malloc(sizeof(struct buffer *)*2);
	arg_3 = malloc(sizeof(struct buffer *)*2);

	//initialize some values in structs
	input_seperator->string_count = 0;
	input_seperator->string_processed = 0;
	input_seperator->stop_found = FALSE;
	seperator_plus->string_count = 0;
	seperator_plus->string_count = 0;
	seperator_plus->stop_found = FALSE;
	plus_output->string_count = 0;
	plus_output->string_count = 0;
	plus_output->stop_found = FALSE;

	//fill out arguments to pass propper buffers in	
	arg_2[0] = input_seperator;
	arg_2[1] = seperator_plus;
	arg_3[0] = seperator_plus;
	arg_3[1] = plus_output;

	//sets up and calls threads
	pthread_create(&input_t, NULL, input_thread, input_seperator);
	pthread_create(&seperator_t, NULL, line_seperator_thread, arg_2);
	pthread_create(&plus_t, NULL, plus_sign_thread, arg_3);
	pthread_create(&output_t, NULL, output_thread, plus_output);

	//terminate threads as needed
	pthread_join(input_t, NULL);
	pthread_join(seperator_t, NULL);
	pthread_join(plus_t, NULL);
	pthread_join(output_t, NULL);

	//free the imprisoned memory
	for (i = 0; i < 50; i++) {
		free((input_seperator->string)[i]);	
		free((seperator_plus->string)[i]);
		free((plus_output->string)[i]);
	}	
	free(input_seperator->string);
	free(seperator_plus->string);
	free(plus_output->string);

	free(input_seperator);
	free(plus_output);
	free(seperator_plus);	

	free(arg_3);
	free(arg_2);
	return;
}


/* Input: none
 * Output: none
 * Note: just calls the setup function
 */
int main() {
	setup();
	
	return 0;
}
