#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>

#define COMMENT_CHAR "#"
#define ACTION enum Action
#define SCOPE enum Scope

void removeNewLineCharacter(char*);
void parseExpression(char*, FILE*);
void declareCommandLineArgs(float*, FILE*, int);
void removeSpaces(char*);
void rtrim(char*);
bool isValidIdentifier_variable(char*);
bool isValidIdentifier_function(char*);
bool isFunction(char*);

// Action enumerator is used to handle a switch-case by identifying the type of an instruction
ACTION {
	ASSIGNMENT = 1,
	FUNCTION_DEFINITION = 2,
	RETURN = 3,
	PRINT = 4,
	NONE = 5
};

// Scope enumerator is used to track if an instruction is either local to a function or global
SCOPE {
    GLOBAL = 1,
    LOCAL = 2
};

struct identifier {
	char name[13];
	bool isFunction;	// Flag to identify the type of identifier - variable/function
}vars[50];			    // Allows the user to use at most 50 identifiers

int main(int argc, char* argv[]) {

	char* file_name;
    char c_file[12];
    float command_line_args[50]; // Allowing upto 50 command-line arguments
    int line_pointer = 0;
    int n_args; // To store the number of optional command-line arguments provided

    // Error handling

	if(argc < 2) {
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

    // Construct the filename for the C file to be generated in the form ml-<pid>.c
	sprintf(c_file,"ml-%d.c",getpid());

	FILE* source_file = fopen(file_name, "r");	// Open the input file
	FILE* c_builder = fopen(c_file, "w");		// Open the output C file to write translated C code
    FILE* header = fopen("global.h", "w");      // Create a header to declare functions and command-line arguments (if any)

    // Condition block executes when command-line arguments are provided. 
    if(argc > 2) {
        n_args = argc - 2;
        declareCommandLineArgs(command_line_args, header, n_args);
    }

    

	fprintf(c_builder,"#include <stdio.h>\n#include \"global.h\"\n");
	fprintf(c_builder,"void body() {\n");

    if(source_file == NULL) {
		fprintf(stderr,"! File not found / Unable to open file\n");
		exit(EXIT_FAILURE);
	}

    char *line_buffer = (char*) malloc(sizeof(char));

    //SCOPE instruction = GLOBAL; // The scope of an instruction is set to global by default

    // Read each line from ml file
    while((line_buffer = fgets(line_buffer, BUFSIZ, source_file)) != NULL) {

		char* current_line = line_buffer;

		// Condition to skip blank lines and comments
		if(current_line[0] == '\n' || current_line[0] == '#') continue;
        
        // Remove newline character as it is irrelevant for parsing the current line
        removeNewLineCharacter(current_line);

        // strtok() replaces the first occurrence a '#' with '\0'. We do this to ignore comments in the ML file
        strtok(current_line, COMMENT_CHAR); 

		// Increase the line pointer for every iteration to keep track of line number. 
		line_pointer++;

        ACTION statement_type;

        if(strstr(current_line,"function")) statement_type = FUNCTION_DEFINITION;
        else if(strstr(current_line,"return")) statement_type = RETURN;
        else if(strstr(current_line,"<-")) statement_type = ASSIGNMENT;
        else if(strstr(current_line,"print")) statement_type = PRINT;
        else {
            fprintf(stderr, "! Invalid line");
            exit(EXIT_FAILURE);
        }

        switch(statement_type) {
            case ASSIGNMENT:
                parseExpression(current_line, c_builder);
                break;
            default: 
                break;
        }
        printf("%d %s",line_pointer, current_line);
        printf("\n");
    }
    
    // Cleaning up
    free(line_buffer);
    fclose(header);
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

void parseExpression(char* expr, FILE* to_write) {

    char variable[13]; // Used to store the identifier used in the LHS of the expression (result variable)
    strcpy(variable, strtok(expr, "<-"));
    printf("\n%s\n",expr);
    if(!isValidIdentifier_variable(variable) && !isFunction(variable)) {
        fprintf(stderr, "! Illegal naming/use of identifier: %s\n", variable);
        exit(EXIT_FAILURE);
    }


}

void declareCommandLineArgs(float* arr, FILE* header, int n) { }

// This function removes all whitespace characters in a text and transforms into a continuous stream of characters
void removeSpaces(char* text) {
	char TextWithoutSpace[BUFSIZ];
	char* temp = text;
	int i = 0;
	while(*temp != '\0') {
	    if(*temp == ' ') {
	        temp++;
	        continue;
	    }
	    TextWithoutSpace[i++] = *temp;
	    temp++;
	}
	TextWithoutSpace[i] = '\0';
	temp = text;
	i = 0;
	while(TextWithoutSpace[i] != '\0') {
	    *temp = TextWithoutSpace[i++];
	    temp++;
	}
	*temp = '\0';
}

bool isValidIdentifier_variable(char* identifier) {
    int i = 1;
    rtrim(identifier);
    if(isdigit(identifier[0]) && !isalpha(identifier[0])) return false;
    while(identifier[i] != '\0') {
        if(isspace(identifier[i]) || !isalnum(identifier[i])) return false;
        i++;
    }
    return true;
}

// This function removes trailing whitespace character(s) in a text
void rtrim(char* text) {
    int index = -1, i = 0;
    while(text[i] != '\0') {
        if(isdigit(text[i]) || isalpha(text[i])) index = i;
        i++;
    }
    text[++index] = '\0';
}

bool isFunction(char* token) {}