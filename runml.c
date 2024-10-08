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
void declareVariables(char*, FILE*);
void defineFunction(char*, FILE*);
void returnExpression(char*, FILE*);
void printExpression(char*, FILE*);
bool isValidIdentifier_variable(char*);
bool isValidIdentifier_function(char*);
bool variableExists(char*);
bool functionExists(char*);
bool isRealNumber(char*);

int varptr = -1;
bool returnsValue = false;  // This variable tracks the occurrence of a return statement in a function

// Action enumerator is used to handle a switch-case by identifying the type of an instruction
ACTION {
	ASSIGNMENT = 1,
	FUNCTION_DEFINITION = 2,
	RETURN = 3,
	PRINT = 4,
	FUNCTION_CALL = 5
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
    FILE* header = fopen("global.h", "w");         // Create a header to declare functions and command-line arguments (if any) and global variables
    FILE* function = fopen("functions.c", "w");    // Handling functions in a separate file to isolate local and global statements

    // Condition block executes when command-line arguments are provided. 
    if(argc > 2) {
        n_args = argc - 2;
        for(int i = 2; i < n_args + 2; i++) 
            command_line_args[i-2] = atof(argv[i]);
        declareCommandLineArgs(command_line_args, header, n_args);
    }

    // Writing boilerplate code to the files that are created
	fprintf(global,"#include <stdio.h>\n#include \"global.h\"\n#include \"functions.c\"\n");
	fprintf(global,"void body() {\n");

    fprintf(function, "#include <stdio.h>\n");

    /* 'result' is a variable that is used to store the value returned by an expression that is 
       used in the print statement. Storing the value in 'result' will effectively help determine
       if the resultant value has a floating point numbers
    */
    fprintf(header, "float result = 0;\n"); 

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
        
        /* filePtr corresponds to either functions.c or ml<pid>.c depending on the scope
           of the instruction - LOCAL or GLOBAL respectively 
        */
        void *filePtr; 

        /* This condition is used to check if the current line starts in the beginning(global scope) 
           or is indented(local)
        */
        if(!isspace(current_line[0])) {
            if(instruction == GLOBAL) {
                filePtr = global; // Assigning 'filePtr' to the C file handling global statements
            }
            else {
                /* Control flow to else block indicates that there is a transition of scope from
                   local to global space
                */

                /* This condition tracks if there is a return statement in a function as the scope 
                   changes. 'returnsValue' gets updated to true if there is a return statement in
                   the function. In the absence, we explicitly write 'return 0.0f' into the file
                */
                if(returnsValue) {
                    fprintf(function, "}\n");
                    returnsValue = false;
                }
                else {
                    fprintf(function, "return 0.0f;\n}");
                }
               instruction = GLOBAL; // Updating the scope
               filePtr = global;
            }
        }

        else if(current_line[0] == '\t') {
            // Control flow comes to this block when the line is indented
            if(instruction == LOCAL) {
                filePtr = function;                
            }
            else {
                fprintf(stderr, "! Unexpected indent at Line: %d\n", line_pointer);
                exit(EXIT_FAILURE);
            }
        }

        ACTION statement_type;

        // Identify the type of the statement based on the standard keywords of ml
        if(strstr(current_line,"function")) statement_type = FUNCTION_DEFINITION;
        else if(strstr(current_line,"<-")) statement_type = ASSIGNMENT;
        else if(strstr(current_line,"return ")) statement_type = RETURN;
        else if(strstr(current_line,"print ")) statement_type = PRINT;
        else if(strstr(current_line,"(")) statement_type = FUNCTION_CALL;
        else {
            fprintf(stderr, "! Invalid statement");
            exit(EXIT_FAILURE);
        }

        switch(statement_type) 
        {
            case ASSIGNMENT:
            {
                parseExpression(current_line, filePtr, header);
                break;
            }
        
            case FUNCTION_DEFINITION:
            {
                if(instruction != GLOBAL) {
                    fprintf(stderr, "! Unsupported function definition inside a function");
                    exit(EXIT_FAILURE);
                }
                instruction = LOCAL;
                defineFunction(current_line, function);
                break;
            }

            case PRINT:
            {
                printExpression(current_line, filePtr);
                break;
            }

            case FUNCTION_CALL:
            {
                char func_call[512];
                translateExpression(current_line, func_call);
                fprintf(filePtr, "%s;\n", func_call);
                break;
            }

            case RETURN:
            {
                if(instruction != LOCAL) {
                    fprintf(stderr, "! Error: Use of 'return' outside function at Line: %d", line_pointer);
                    exit(EXIT_FAILURE);
                }
                returnsValue = true;
                returnExpression(current_line, filePtr);
            }
            default: 
            {
                break;
            }
        }
    }

    fprintf(global, "}\nint main(void)\n{\nbody();\n}");
    
    // Cleaning up
    free(line_buffer);
    fclose(header);
    fclose(source_file);
    fclose(global);
    fclose(function);

    char compile_command[50];
    //char remove_files[50];

	sprintf(compile_command,"gcc -o ml %s && ./ml",c_file);
    //sprintf(remove_files, "rm ml* f* g*");
	
    system(compile_command);
	//system(remove_files);
    exit(EXIT_SUCCESS);
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

/* This function handles all the assignment operations by splitting the LHS and RHS of the equation,
    tokenizing every operand, validates identifiers and declares all the undefined variables before
    usage.
*/
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

    // Validating the use of an identifier in accordance to the naming convention
    if(!isValidIdentifier_variable(variable) && !functionExists(variable)) {
        fprintf(stderr, "! Illegal naming/use of identifier: %s\n", variable);
        exit(EXIT_FAILURE);
    }

    if(!variableExists(variable)) { 
        expr[0] != '\t' ? createNewVariable(variable, header) : createNewVariable(variable, c_file);
        sprintf(BUFFER, "%s_=", variable);
    }
    else sprintf(BUFFER, "%s_=",variable); // Else block executes when l-value has been declared

    removeSpaces(rhs);  // Remove any white-space in the expression as white-spaces are insignificant
    
    char* handler = strdup(rhs); // Copy the translated expression to a temporary char pointer. The memory is dynamically allocated to handler

    char* token = strtok(handler, delim);
    
    if(token != NULL) {
        while(token) {
            declareVariables(token, c_file);
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

/* This function looks for all the identifiers used in a given expression and appends them with 
   an '_' character. This is a part of the standard naming convention of all the identifiers used
   in the program. It is done to handle the use of any C standard keywords as identifiers in the ml
   file which will eventually impact the translation.
*/
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

void declareCommandLineArgs(float* arr, FILE* header, int n) { 
    for(int i = 0; i < n; i++) {
        char arg[6];
        sprintf(arg, "arg_%d", i);
        fprintf(header, "float %s = %f;\n", arg, arr[i]);
        strcpy(vars[++varptr].name, arg);
        vars[varptr].isFunction = false;
    }
} 

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
        if(isspace(identifier[i]) || !isalpha(identifier[i])) return false;
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

void declareVariables(char* token, FILE* c_file) {
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
    if(strstr(term_buffer, "arg"))
        return;
	if(isalpha(term_buffer[0])) {
        char* id = strpbrk(term_buffer, "1234567890");
        if(id) {
            char params[40];
            strcpy(params, id);
            *id = '\0';
            if(!functionExists(term_buffer)) {
                fprintf(stderr, "! %s() is not defined\n", term_buffer);
                exit(EXIT_FAILURE);
            }
            else return;
        }

        if(functionExists(term_buffer)) return;

		if(!variableExists(term_buffer)) {
			createNewVariable(term_buffer, c_file);
		}
	}
    else {

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

    createNewFunction(token);

    fprintf(c_file, "float %s_(", token); // Define the function with the function name in the file

    token = strtok(NULL, " "); // Move to the next token (params)

    if(token != NULL) {
        fprintf(c_file, "float %s_", token);
        char variable[13];
        strcpy(variable,token);
        strcat(variable, "_");
        strcpy(vars[++varptr].name, variable);
        vars[varptr].isFunction = false;
    }

    token = strtok(NULL, " ");

    while(token) {

        if(!isValidIdentifier_variable(token)) {
            fprintf(stderr, "! Illegal naming/use of identifier: %s\n", token);
            exit(EXIT_FAILURE);
        }
        char variable[13];
        strcpy(variable,token);
        strcat(variable, "_");
        strcpy(vars[++varptr].name, variable);
        vars[varptr].isFunction = false;
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

    int i = 0;

    while(identifier[i] != '\0') {
        if(!isalpha(identifier[i])) return false;
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

void printExpression(char* exp, FILE* c_file) {

    char current_line[100];
    
    if(*exp == '\t')
        exp++;

    strcpy(current_line, exp);
    
    
    char cleanedExpression[512];
    char print_statement[100];

    // Move the pointer to skip the 'print' keyword 
    char* expression  = strstr(current_line, " ");
    expression++;

    translateExpression(expression, cleanedExpression);
    removeSpaces(cleanedExpression);

    fprintf(c_file, "result = %s;\n", cleanedExpression);
    strcpy(print_statement, "result - (int) result != 0 ? printf(\"%.6f\\n\", result) : printf(\"%d\\n\", (int) result);\n");
    fprintf(c_file, "%s", print_statement);
}

void returnExpression(char* statement, FILE* c_file) {
    
    char current_line[BUFSIZ];
    
    if(*statement == '\t') {
        statement++;
    }
    strcpy(current_line, statement);
    char return_statement[512];

    // Move the pointer to skip the 'return' keyword 
    char* expression = strstr(current_line, " ");
    expression++;

    translateExpression(expression, return_statement);
    fprintf(c_file, "return %s;\n", return_statement);
}