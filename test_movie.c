/* Author: Nathanael Bowles
 * Filename: movie.c
 * Description: processes movie data into a struct and lets the user learn interact with that data via rating, year of release, and language.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FALSE 0
#define TRUE 1

/*struct for movie information */
struct movie
{
	char *title;
	int *year;
	char *languages;
	float *rating_value;
	struct movie *next;
	
};

/* Input: cstring of a line from the movie file from process_movie() function below
 * Output: A movie struct with all that information found from the input string
 * Note: Language is a single cstring, it is not split into an array of strings. I was a little lazy and just used strstr when this matters.
 */
struct movie *create_movie(char *curr_line) {
	struct movie *curr_movie = malloc(sizeof(struct movie));
	
	//for use with strok_r
	char *saveptr = NULL;
	char *token;
	char *filler;
	int int_temp;
	float flt_temp;

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

/* Input: a cstring containing the file path to file that contains the movie information, and a pointer to an int.
 * Output: the head of a linked list of movies as processed from the input file. Additionally, how many movies were processed is included in count (used later when processing movies by rating per year)
 */
struct movie *process_movie(char *file_path, int *count) {
	//open the specified file
	FILE *movie_file = fopen(file_path, "r");	

	char *curr_line = NULL;
	size_t len = 0;
	ssize_t nread;
	char *token;
	*count = 0;

	//head of linked list
	struct movie *head = NULL;
	struct movie *tail = NULL;

	//clear the header line
	getline(&curr_line, &len, movie_file);

	//for each line, fill out a new movie node and add it to the linked list
	while ((nread = getline(&curr_line, &len, movie_file) != -1)) {
		(*count)++;
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

	//free dynamic allocations
	free(curr_line);
	fclose(movie_file);

	printf("Processed file %s and parsed data for %i movies.\n\n", file_path, *count);
	return head;
}

/* Input: the first movie struct that acts as the head of the linked list. 
 * Output: void. However, it will print to screen movies released in the year as specified by the user input
 * User Input: An integer.
 */

void movies_by_year(struct movie *list){
	struct movie *next = list;
	int count = 0;
	int user_input;		

	//second user input
	printf("Enter the year for which you want to see movies: ");
	scanf("%d",&user_input);



	//check each movie, if it from that year then print to screen
	while (next != NULL) {
		if (*(next->year) == user_input){
			printf("%s\n", next->title);	
			count++;
		}	
		next = next->next;
	}

	if (count == 0) {
		printf("No movies were released in the year %i \n", user_input);
	}

	return;	
}


/* Input: the first movie struct that acts as the head of the linked list
 * Output: void. However, it will print to screen the best rated movie for each year (out of years included)
 * Note: Searching for best rated movie is done each time this is called in O(n^2) time. I'm already late on this assignment, sorting a linked list would be a hassle.
 */
void movies_by_rating(struct movie *list, int *count) {
	//variables to iterate through the movies for inner and outer loop respectfully
	struct movie *next = list;
	struct movie *year_outer = list;
	//markes the best rated movie for the iteration currently being compared
	struct movie *leading_movie;
	//redudent declerations so I don't have to deal with pointers here
	int length = *count;
	int years[length];	
	//variables for storing temp values/ iterations through loops
	int current_year = 0;
	int current_rating = 0;
	int i;
	int j = 0;	
	int skip;

	//initializeyears
	for (i = 0; i < length; i++) {
		years[i] = 0;
	}

	//since I don't want to sort the file, we'll just O(N^2) style go through each year
	//outer loop, goes through each year
	while (year_outer != NULL){
		skip = TRUE;

		//if the year has already been checked, skip it
		for (i = 0; i < length; i++) {
			if (years[i] == *(year_outer->year)) {
				skip = FALSE;
			}
		}
	
		if (skip) {
			//inner loop, for each year we iterate to find the largest rating value and print it to stdout
			current_year = *(year_outer->year);
			years[j] = current_year;
			leading_movie = year_outer;
			j++;		
	
			while (next != NULL) {
				if ((*(next->year) == current_year) & (current_rating < *(next->rating_value))) {
					current_rating = *(next->rating_value);
					leading_movie = next;
				}
				//iterate to next movie for inner loop
				next = next->next;
			}
			printf("%i %.1f %s \n", *(leading_movie->year), *(leading_movie->rating_value),leading_movie->title);	
		}
		//move to next movie, outer loop
		year_outer = year_outer->next;
	}				
}

/* Input: the first movie struct that acts as the ehad of the linked list.
 * Output: Void. However, prints the titles of movies released in certain languages
 */

void movies_by_language(struct movie *list) {
	struct movie *next = list; 
	char user_input[20];	
	int movie_found = FALSE;

	//take user input
	printf("Enter the language for which you want to see movies: ");
	scanf("%s", &user_input);

	//search for what movies have that language
	//Since I don't split the languages, this is done by finding a substring. 
	while (next != NULL) {
		if (strstr(next->languages, user_input)) {
			printf("%i %s\n",*(next->year), next->title);
			movie_found = TRUE;
		}
		next = next->next;
	}
	
	//if no movies movies found for that language, print this
	if (!movie_found) {
		printf("No data about movies released in %s\n", user_input);
	}
}

/* Input: First movie that acts as the head of the linked list. the count of movies processed beforehand/ how many movies in the linked list
 * Output: No output.
 * Note: Acts as the menu that lets the user interact with the movie information.
 */

void menu(struct movie *list, int *count) {
	int input;

	do{
		//menu options listed at the start of each loop
		printf("\nShow movies released in the specified year\n");
		printf("Show highest rated movie for each year\n");
		printf("Show the title and year of release of all movies in a specific language\n");
		printf("Exit from the program\n");

		//take user input
		scanf("%d", &input);	

		//split between each of the following options (1-4), if other number input is given an error message is given
		if (input == 1) {
			movies_by_year(list);			
		}
		else if (input == 2) {
			movies_by_rating(list, count);
		}
		else if (input == 3) {
			movies_by_language(list);
		}
		else if (input == 4) {
			input = FALSE;
		}
		else {
			printf("Invalid choice, please try again\n");
			input = -1;
		} 

	} while(input);
	
	return;
}


/* Input: Input file path to the file with all the movie names
 * Output: 0 because it's the main function and I'm not hooking it up to anything else.
 */

int main(int argc, char *argv[]){
	int *count = malloc(sizeof(int));	

	//process the file to give us our linked list
	struct movie *list = process_movie(argv[1], count);

	//call menu function to let users look at processed data
	menu(list, count);		
	
	//free count data
	free(count);
	return 0;
} 
