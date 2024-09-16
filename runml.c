#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>

#define COMMENT_CHAR "#"
#define LINESIZE 1024

void removeNewLineCharacter(char*);

enum LineAction {
	ASSIGNMENT = 1,
	FUNCTION_DEFINITION = 2,
	RETURN = 3,
	PRINT = 4,
	NONE = 5
};

struct identifier {
	char name[13];
	bool isFunction;	// Flag to identify the type of identifier - variable/function
}vars[50];			    // Allows the user to use at most 50 identifiers

int main(int argc, char* argv[]) {

	char* file_name;
	int pid = getpid();
	char pid_str[7];
	int line_pointer = 0;

	sprintf(pid_str,"%d", pid);


// Error handling

	if(argc != 2) {
		fprintf(stderr,"usage: runml <filename>\".ml\"\n");
		exit(EXIT_FAILURE);
	}

	int filename_length = strlen(argv[1]);

	if(filename_length < 3) {
		printf("Invalid filename\n");
		exit(EXIT_FAILURE);
	}

//Explicit condition check for a filename with a .ml extension
	if((argv[1][filename_length - 1] != 'l' || argv[1][filename_length - 2] != 'm' || argv[1][filename_length -3] != '.')) {
		printf("File name should end with an extension <filename>.ml\n");
		exit(EXIT_FAILURE);
	}

	file_name = argv[1];
	char c_file[12];

// Construct the filename for the C file to be generated in the form ml-<pid>.c
	sprintf(c_file,"ml-%s.c",pid_str);

	FILE* source_file = fopen(file_name,"r");	// Open the input file
	FILE* c_builder = fopen(c_file, "w");		// Open the output C file to write translated C code

	fprintf(c_builder,"#include <stdio.h>\n");
	fprintf(c_builder,"void body() {\n");

    if(source_file == NULL) {
		fprintf(stderr,"Unable to open file\n");
		exit(EXIT_FAILURE);
	}

    char *line_buffer = (char*) malloc(sizeof(char));

    // Read each line from ml file
    while((line_buffer = fgets(line_buffer, LINESIZE, source_file)) != NULL) {

		char* current_line = line_buffer;

		// Condition to skip blank lines and comments
		if(current_line[0] == '\n' || current_line[0] == '#') continue;

        
        // Remove newline character as it is irrelevant for parsing the line
        removeNewLineCharacter(current_line);

        // strtok() replaces the first occurrence a '#' with '\0'. We do this to ignore comments in the ML file
        strtok(current_line, COMMENT_CHAR); 

		// Increment the line pointer for every iteration to keep track of line number. 
		line_pointer++;

        printf("%d %s",line_pointer, current_line);
        printf("\n");
    }

    free(line_buffer);
    fclose(source_file);
    fclose(c_builder);
}

void removeNewLineCharacter(char* token) {
    char* temp = token;
    while(*temp != '\0') {
        if(*temp == '\n') {
            *temp = '\0';
            break;
        }
        temp++;
    }
}
