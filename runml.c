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
void parseExpression(const char*, FILE*, FILE*);
void declareCommandLineArgs(float*, FILE*, int);
void removeSpaces(char*);
void rtrim(char*);
void createNewVariable(char*, FILE*);
void createNewFunction(char*);
void translateExpression(char*, char*);
void declareVariables(char*, FILE*, FILE*);
void defineFunction(char*, FILE*);
bool isValidIdentifier_variable(char*);
bool isValidIdentifier_function(char*);
bool variableExists(char*);
bool functionExists(char*);
bool isRealNumber(char*);
int varptr = -1;

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

	FILE* source_file = fopen(file_name, "r");	   // Open the input file
	FILE* global = fopen(c_file, "w");		       // Open the output C file to write translated C code
    FILE* header = fopen("global.h", "w");         // Create a header to declare functions and command-line arguments (if any)
    FILE* function = fopen("functions.c", "w");    // Handling functions in a separate file to isolate local and global variables

    // Condition block executes when command-line arguments are provided. 
    if(argc > 2) {
        n_args = argc - 2;
        declareCommandLineArgs(command_line_args, header, n_args);
    }

	fprintf(global,"#include <stdio.h>\n#include \"global.h\"\n#include \"functions.c\"");
	fprintf(global,"void body() {\n");

    fprintf(function, "#include <stdio.h>\n");

    if(source_file == NULL) {
		fprintf(stderr,"! File not found / Unable to open file\n");
		exit(EXIT_FAILURE);
	}

    char *line_buffer = (char*) malloc(sizeof(char));

    SCOPE instruction = GLOBAL; // The scope of an instruction is set to global by default

    // Read each line from ml file
    while((line_buffer = fgets(line_buffer, BUFSIZ, source_file)) != NULL) {

		char* current_line = line_buffer;

		// Increase the line pointer for every iteration to keep track of line number. 
		line_pointer++;

		// Condition to skip blank lines and comments
		if(current_line[0] == '\n' || current_line[0] == '#') continue;
        
        // Remove newline character as it is irrelevant for parsing the current line
        removeNewLineCharacter(current_line);

        // strtok() replaces the first occurrence a '#' with '\0'. We do this to ignore comments in the ML file
        strtok(current_line, COMMENT_CHAR); 
        
        void *filePtr;

        if(!isspace(current_line[0])) {
            if(instruction == GLOBAL) {
                filePtr = global;
            }
            else {
                /* Functionality to close brace in functions.c, revert instruction type to global
                   and assign filePtr to global
                */
               fprintf(function, "}\n");
               instruction = GLOBAL;
               filePtr = global;
            }
        }
        else if(current_line[0] == '\t') {
            if(instruction == LOCAL) {
                filePtr = function;                
            }
            else {
                fprintf(stderr, "! Unexpected indent at Line: %d\n", line_pointer);
                exit(EXIT_FAILURE);
            }
        }

        ACTION statement_type;

        if(strstr(current_line,"function")) statement_type = FUNCTION_DEFINITION;
        else if(strstr(current_line,"<-")) statement_type = ASSIGNMENT;
        else if(strstr(current_line,"return")) statement_type = RETURN;
        else if(strstr(current_line,"print")) statement_type = PRINT;
        else {
            fprintf(stderr, "! Invalid statement");
            exit(EXIT_FAILURE);
        }

        switch(statement_type) {

            case ASSIGNMENT:
                parseExpression(current_line, filePtr, header);
                break;
            
            case FUNCTION_DEFINITION:
                if(instruction != GLOBAL) {
                    fprintf(stderr, "! Unsupported function definition inside a function");
                    exit(EXIT_FAILURE);
                }

                instruction = LOCAL;
                defineFunction(current_line, function);
                break;

            default: 
                break;
        }
        printf("%d %s |%d",line_pointer, current_line, instruction);
        printf("\n");
    }
    
    // Cleaning up
    free(line_buffer);
    fclose(header);
    fclose(source_file);
    fclose(global);
    fclose(function);
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

void parseExpression(const char* expr, FILE* c_file, FILE* header) {
    char variable[13]; // Used to store the identifier used in the LHS of the expression (result variable)
    char expression[BUFSIZ];
    char translatedExpression[BUFSIZ]; 
    char BUFFER[BUFSIZ];
    char delim[] = "+*-/";  // Valid arithmetic operators used as delimiters in an expression

    strcpy(expression, expr);   // Copying the contents of the entire line to a temporary buffer for parsing

    // Move the RHS pointer from the beginning of the line to the right-hand side of the assignment operator
    char* rhs = strstr(expression,"-"); 
    rhs++;

    strcpy(variable, strtok(expression, "<-")); // Extract the l-value for validation

    printf("\n? %s\n",variable); // ? DEBUG

    // Validating the use of an identifier in accordance to the naming convention
    if(!isValidIdentifier_variable(variable) && !functionExists(variable)) {
        fprintf(stderr, "! Illegal naming/use of identifier: %s\n", variable);
        exit(EXIT_FAILURE);
    }

    if(!variableExists(variable)) { 
        createNewVariable(variable, header);
        sprintf(BUFFER, "%s_=", variable);
    }
    else sprintf(BUFFER, "%s_=",variable); // Else block executes when l-value has been declared

    removeSpaces(rhs);  // Remove any white-space in the expression as white-spaces are insignificant
    
    char* handler = strdup(rhs); // Copy the translated expression to a temporary char pointer. The memory is dynamically allocated to handler

    char* token = strtok(handler, delim);
    
    if(token != NULL) {
        while(token) {
            declareVariables(token, c_file, header);
            token = strtok(NULL, delim);
        }
        translateExpression(rhs, translatedExpression); // Appends and '_' character to valid identifiers
        strcat(BUFFER, translatedExpression);
    }
    else {
        // This block executes for direct assignment statement. (expression has no operators)
        // Check if RHS has an l-value or real number
        if(isdigit(rhs[0])) strcat(BUFFER, rhs);
        else {
            char output[512];
            translateExpression(rhs, output);
            strcat(BUFFER, output);
        }
    }

    fprintf(c_file,"%s;\n", BUFFER);


    free(handler);
}

void translateExpression(char* input, char* output) {

    int ptr = 0;
    output[ptr++] = *input;

    input++;
	if(*input == '\0') {	
		isalpha(*(input - 1)) ? output[ptr++] = '_': 0;
		output[ptr] = '\0';
		return;
	}
	else {
    	while(*input != '\0') {
        	if(!isalpha(*input) && isalpha(*(input -1))) {
            	output[ptr++] = '_';
            	output[ptr++] = *input;
            	input++;
            	continue;
        	}
        	output[ptr++] = *input;
        	input++;
    	}
	}

	if(isalpha(*(input-1)))
		output[ptr++] = '_';

	output[ptr] = '\0';
	removeSpaces(output);
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
    
    int n_keywords = 3;
    char keywords[3][10] = {"function", "return", "print"};
    int i = 1;
    rtrim(identifier);

    // Checking if the identifier comprises of valid characters
    if(isdigit(identifier[0]) && !isalpha(identifier[0])) return false;
    while(identifier[i] != '\0') {
        if(isspace(identifier[i]) || !isalnum(identifier[i])) return false;
        i++;
    }

    // Checking if any keywords of ML is used as an identifier
    for(int i = 0; i < n_keywords; i++) {
        if(!strcmp(keywords[i], identifier)) return false;
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

void createNewVariable(char* id, FILE* c_file) {
    char variable[13];
    strcpy(variable,id);
    strcat(variable, "_");
    strcpy(vars[++varptr].name, variable);
    vars[varptr].isFunction = false;
    fprintf(c_file, "float %s = 0;\n", variable);
}

bool variableExists(char* id) {
    char variable[13];
    strcpy(variable,id);
    strcat(variable, "_");
	for(int i = 0; i <= varptr; i++) {
		if(strcmp(vars[i].name, variable) == 0 && !vars[i].isFunction)
			return true;
    }
	return false;	
}

void declareVariables(char* token, FILE* c_file, FILE* header) {
	int char_ptr = 0;
	char term_buffer[50];

	while(*token!='\0') {
		if(*token == '(' || *token == ')' || *token == ';') {
			token++;
			continue;
		}
		term_buffer[char_ptr++] = *token;
		token++;
	}

	term_buffer[char_ptr] = '\0';

	// Exit the function if the term is a real number as it is not needed to declare real numbers. Duh!
    if(isRealNumber(term_buffer))
		return;

	if(isalpha(term_buffer[0])) {
		if(!variableExists(term_buffer)) {
			createNewVariable(term_buffer, header);
		}
	}
}

bool isRealNumber(char* token) {
    char* temp = token;
	while(*temp != '\0' && *temp != '\n') {
		if(*temp == '.') {
			temp++;
			continue;
		}
		else if(isdigit(*temp)) {
			temp++;
			continue;
		}
		else return false;
	}
	return true;
}

void defineFunction(char* statement, FILE* c_file) {
    char current_line[BUFSIZ];
    strcpy(current_line, statement);

    char* token = strtok(current_line, " "); // Assuming that a function definition is space separated

    token = strtok(NULL, " "); // Extract the name of the function

    if(!isValidIdentifier_function(token)) {
        fprintf(stderr, "! Illegal naming/use of identifier: %s\n", token);
        exit(EXIT_FAILURE);
    }

    if(functionExists(token)) {
        fprintf(stderr, "! Error: Redefinition of function %s", token);
        exit(EXIT_FAILURE);
    }


    fprintf(c_file, "float %s_(", token); // Define the function with the function name in the file

    token = strtok(NULL, " "); // Move to the next token (params)

    fprintf(c_file, "float %s_", token);

    token = strtok(NULL, " ");

    while(token) {

        if(!isValidIdentifier_variable(token)) {
            fprintf(stderr, "! Illegal naming/use of identifier: %s\n", token);
            exit(EXIT_FAILURE);
        }

        printf("@@%s@@\n", token);
        fprintf(c_file, ",float %s_", token);
        token = strtok(NULL, " "); // Move to the next parameter (if any)
    }
    fprintf(c_file, "){\n");
}

void createNewFunction(char* id) {
    char variable[13];
    strcpy(variable,id);
    strcat(variable, "_");
    strcpy(vars[++varptr].name, variable);
    vars[varptr].isFunction = true;
}

bool isValidIdentifier_function(char* identifier) {

    int i = 1;

    // Checking if the function name comprises of valid characters
    if(isdigit(identifier[0]) || (!isalpha(identifier[0]) || identifier[0] != '_')) 
        return false;
    while(identifier[i] != '\0') {
        if(!isalnum(identifier[i])) return false;
        i++;
    }
    return true;
}

bool functionExists(char* token) {
    char variable[13];
    strcpy(variable,token);
    strcat(variable, "_");
	for(int i = 0; i <= varptr; i++) {
		if(strcmp(vars[i].name, variable) == 0 && vars[i].isFunction) 
            return true;
    }
	return false;
}