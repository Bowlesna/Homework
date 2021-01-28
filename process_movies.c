/* Author: Nathanael Bowles
 * Input: no commandline input. The prompts will let you choose a file to be processed
 * Output: just the integer value from main. When running, the code will create a directory in the current cirectory then add files for each year depending on the processed movies
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

#define TRUE 1
#define FALSE 0

//struct for use when processing the movies, only title and year used. copy pasted from assignment 1.
struct movie {
	char *title;
	int *year;
	char *languages;
	float *rating_value;
	struct movie *next;
	
};


/* Input: cstring of a line from the movie file from process_movie() function belo
 * output: A movie struct with all that information found from the input string
 * Note: Language is a single cstring, it is not split into an array of strings. I was a little lazy and just used strstr when this matters.
 * copy pasted from assignment 1.
 * */

struct movie *create_movie(char *curr_line) {
	struct movie *curr_movie = malloc(sizeof(struct movie));
	
	//for use with strok_r
	char *saveptr = NULL;
	char *token;
	char *filler;
	//int int_temp;
	//float flt_temp;

	//the first token is title
	token = strtok_r(curr_line, ",", &saveptr);	
	curr_movie->title = calloc(strlen(token)+1, sizeof(char));
	strcpy(curr_movie->title, token);

	//the second token is the year
	token = strtok_r(NULL, ",", &saveptr);
	curr_movie->year = malloc(sizeof(int));
	filler = calloc(strlen(token)+1, sizeof(char));
	strcpy(filler, token);
	*(curr_movie->year) = atoi(filler);
	//free filler	

	//the third token is the languages, we'll deal with parcing the languages individually later
	token = strtok_r(NULL, ",", &saveptr);
	curr_movie->languages = calloc(strlen(token)+1,sizeof(char));
	strcpy(curr_movie->languages, token);

	//the fourth token is the rating values
	token = strtok_r(NULL, "\n", &saveptr);
	curr_movie->rating_value = calloc(strlen(token)+1,sizeof(char));
	strcpy(filler, token);
	*(curr_movie->rating_value) = atof(filler);
	
	//printf("%f \n", *(curr_movie->rating_value));

	curr_movie->next = NULL;

	return curr_movie;
}


/* input: a cstring containing the file path that contains the movie information, and a pointer to an int.
 * Output: the head of a linked list of movies as processed from the input file. Additonall, how many movies processed is a inlcuded in count
 * copy from assignment 1
 */
struct movie *process_movie(char *file_path) {
	//open the specified fil
	FILE *movie_file = fopen(file_path, "r");	
	char *curr_line = NULL;
	size_t len = 0;
	ssize_t nread;
	int count = 1;

	//head of linked list
	struct movie *head = NULL;
	struct movie *tail = NULL;

	//clear the header line
	getline(&curr_line, &len, movie_file);

	while ((nread = getline(&curr_line, &len, movie_file) != -1)) {
		count++;
		struct movie *new_node = create_movie(curr_line);		
		if (head == NULL) {
			head = new_node;
			tail = new_node;
		}
		else{
			tail->next = new_node;
			tail = new_node;
		}
	}

	free(curr_line);
	fclose(movie_file);

	printf("Processed file %s and parsed data for %i movies.\n\n", file_path, count);
	return head;
}

/* Input: a c string containg the file path to the file that contains the movie information
 * Output: void function. Will create a new directory and files for each movie inside that directory.
 *
 */

void process_data(char *file_path) {
	//int check;
	int random;
	char dir_name[50] = "bowlesna.movies.";
	char temp[50];
	char new_dir_path[60];
	int file_check;

	//process data using what we did for HW1 to make it easy to work with
	printf("\nNow processing the complete file name: %s .\n", file_path);
	struct movie *list = process_movie(file_path);
		
	//make a new directory with a random number in the name
	random = rand() % 100000;
	sprintf(temp, "%i", random);
	strcat(dir_name, temp );
	mkdir(dir_name, 0750);
	printf("Created a directory with name %s\n", dir_name);

	//if the folder is empy, warn the user
	if(list == NULL) {
		printf("No movies to put in files\n");
	}

	//create a file for each year and add information to it
	while (list != NULL) {

		//deal with naming the file based on the year and labeling it as a text file
		//there's gotta be a better way to do this
		strcpy(new_dir_path, dir_name);
		strcat(new_dir_path, "/");
		sprintf(temp, "%i", *(list->year));
		strcat(new_dir_path, temp);
		strcat(new_dir_path, ".txt");
		file_check = open(new_dir_path, O_RDWR | O_CREAT | O_APPEND, 0640);	
		//if the program eer fails to open a file, exit (as used in the file exploraiton chapters)
		if (file_check == -1) {
			printf("open() failed on \"%s\" \n", new_dir_path);
			exit(1);
		}

		//print info to file and add a new line
		write(file_check, list->title , strlen(list->title));
		write(file_check, "\n", strlen("\n"));
		close(file_check);

		list = list->next;
	}	
}

/* Input: a char pointer that will carry the file path we tested here
 * Ouput: an int 1 or 0 that notifies whether a file was succesfully chosen or not
 * Note: 1 for failure to load file, 0 for success
 */
int user_chooses_file(char *file_path) {
	char user_input[250];
	int file_check;


	//get user input
	printf("\nEnter the complete file name: ");
	scanf("%s", user_input);
	
	//check if file opened correctly. If it was, return false. if it failed, teturn true.
	file_check = open(user_input, O_RDWR, 0600);
	if (file_check == -1) {
		printf("\nThe file %s was not found. Try again.\n", user_input);
		return TRUE;
	}
	else {
		strcpy(file_path, user_input);
		close(file_check);
		return FALSE;
	}
}

/* Input: a char pointer that will hold the file path found in the function
 * Output: Void function. finds the smallest file that starts with movies_ and has .csv
 * Note: I wanted the file checks to be consistent in how they nabbed the file, and user choice was an int function. 
 * So I succesfully kept them consistent in the most inconsistent way possible
 * Note2: This function is mostly copied from the explority page
 * Note3: My attempt to get it to only look at the last 3 elements for .csv ended in failure, so I used strstr if I need to be graded down for that.
 */
void smallest_file(char *file_path) {
	DIR * curr_dir = opendir("./");
	struct dirent *a_dir;
	struct stat dir_stat;
	off_t smallest_filesize ;
	char entry_name[256];
	int i = 0;

	//Loop to go through each file in the directory
	while ((a_dir = readdir(curr_dir)) != NULL) {
	
		//if the file starts with movies_ and ends with .csv
	 	if((strncmp("movies_", a_dir->d_name, strlen("movies_")) == 0) && (strstr(a_dir->d_name, ".csv") != NULL )){
			//.grab data
			stat(a_dir->d_name, &dir_stat);

			//find the largest file
			if (i == 0 || smallest_filesize > dir_stat.st_size ){
				smallest_filesize = dir_stat.st_size;
				memset(entry_name, '\0', sizeof(entry_name));
				strcpy(entry_name, a_dir->d_name);
			}
			i++;
		}	
	}				

	closedir(curr_dir);
	//printf("Largest file is: %s\n", entry_name);
	strcpy(file_path, entry_name);
	return;
}

/* Input: a char pointer that will hold the file path found in the function
 * Output: Void function. finds the largest file that starts with movies_ and has .csv
 * Note: I wanted the file checks to be consistent in how they nabbed the file, and user choice was an int function. 
 * So I succesfully kept them consistent in the most inconsistent way possible
 * Note2: This function is mostly copied from the explority page
 * Note3: My attempt to get it to only look at the last 3 elements for .csv ended in failure, so I used strstr if I need to be graded down for that.
 */

void largest_file(char *file_path) {
	DIR * curr_dir = opendir("./");
	struct dirent *a_dir;
	struct stat dir_stat;
	off_t largest_filesize;
	char entry_name[256];
	int i = 0;

	//Loop to go through each file in the directory
	while ((a_dir = readdir(curr_dir)) != NULL) {
	        //if the file starts with movies_ and contains .csv then grab it
		if ((strncmp("movies_", a_dir->d_name, strlen("movies_")) == 0) && (strstr(a_dir->d_name, ".csv") != NULL)){
			//.grab data
			stat(a_dir->d_name, &dir_stat);

			//find the largest file
			if (i == 0 || largest_filesize < dir_stat.st_size ){
				largest_filesize = dir_stat.st_size;
				memset(entry_name, '\0', sizeof(entry_name));
				strcpy(entry_name, a_dir->d_name);
			}
			i++;
		}	
	}				
	strcpy(file_path, entry_name);
	closedir(curr_dir);
	//printf("Largest file is: %s\n", entry_name);
	return;
}

/* Input: none
 * Output: none
 * Note: This file is just a menu, it grabs the users choice of how they want to select the file. then calls the process_data function using that info
 */
void file_select_menu() {
	int input;
	int loop_check = TRUE;
	char file_path[256];	
	

	//process input
	while(loop_check) {
		loop_check = FALSE;

		//prompt user
		printf("\nWhile file you want to process?\n");
		printf("Enter 1 to pick the largest file.\n");
		printf("Enter 2 to pick the smallest file.\n");
		printf("Enter 3 to specify the name of a file.\n");
		printf("Enter a choice from 1 to 3:");

		//grab user input;
		scanf("%i", &input);
		
		//based on the above printf messages, go to the appropriate method of choosing a file
		if (input == 1) {
			largest_file(file_path);
			loop_check = FALSE;
		}
		else if (input == 2) {
			smallest_file(file_path);
			loop_check = FALSE;
		}
		else if (input == 3) {
			loop_check = user_chooses_file(file_path);			
		}
		else {
			printf("\nInvalid input, please input 1, 2, or 3.\n");
			loop_check = TRUE;
		}
       	}

	//process the chosen file)
	process_data(file_path);
}

/* Input: None
 * Output: NOne
 * Note: Menu to choose if user wants to select a file or exit function
 */
void menu(){
	int input;	
	int loop_check = TRUE;
		
	//process input
	while(loop_check){
		//prompt user
		printf("1. Select file to process.\n");
		printf("2. Exit the program.\n");
		printf("Enter a choice 1 or 2: ");

		//user input based on the above prompts
		scanf("%i", &input);
		if (input == 1) {
			file_select_menu();
		}
		else if (input == 2) {
			return;
		}
		else {
	   		printf("\nInvalid input, please input a 1 or a 2.\n");
		}
	}	
	return;
}

/* Input: None
 * Output: integer value of 0.
 * Note: Just here to seed the random number and call the menu
 */

int main(){
	//seed random gen.
	srand(time(NULL));

	//start program from menu
	menu();

	return 0;
}

