// process management specific
#include <sys/wait.h>
#include <sys/types.h>
// POSIX CONSTANT
#include <unistd.h>
// standard programming
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// share memory specific
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
// user defined library
#include "intel_hex.h"
#include "error_description.h"
// constants
#define TOK_DELIM " \t\r\n\a"
#define SHM_BOUND 128
#define ARG_BOUND 64
/* 
* ================================================== function prototype ==================================================
*/

/* 
* ================================================== core functions ==================================================
*/
void shell_loop(void);
char *shell_readline(void);
int shell_execute(void);
int shell_launch(void);

/* 
* ================================================== arguement processing functions ==================================================
*/
int arguement_processor(const char *command, char **target_function, char **parameters);
char **command_splitter(char *command);
char *target_getter(char *command);
char *parameters_getter(const char *command);

/* 
* ================================================== share memory IPC functions ==================================================
*/
int write_to_share_memory(const char *target_function, const char *parameters);
char *read_from_share_memory(const char *target_function);

/* 
* ================================================== SHELL built-in functions ==================================================
*/
int shell_cd(void);
int shell_help(void);
int shell_exit(void);
int shell_BinToIntelHexFormat(void);

/* 
* ================================================== GLOBAL VARIABLE SEGMENT ==================================================
*/
char *builtin_str[] = {
	"cd",
	"help",
	"exit",
	"binary_to_intel_hex_format"
};

int (*builtin_func[]) () = {
	&shell_cd,
	&shell_help,
	&shell_exit,
	&shell_BinToIntelHexFormat
};

/* 
* ================================================== MAIN PROGRAM SEGMENT ==================================================
*/
int main(void)
{
	shell_loop();
	return EXIT_SUCCESS;
}

/* 
* ================================================== FUNCTION DEFINITION SEGMENT ==================================================
*/
void shell_loop(void)
{
	char *command;
	char *target_function;
	char *parameters;
  	int status;

  	do {
    	printf("$ ");
		command = shell_readline();
		status = arguement_processor(command, &target_function, &parameters);

		if (status)
		{
			write_to_share_memory("function_name", target_function);
			write_to_share_memory("command", command);
			write_to_share_memory(target_function, parameters);
			/*
			*	sleep not necessary because:
			*	we store to shared memory using sprintf()
			*	we load from shared memory using strcpy()
			*	no race condition expected
			*/
			// sleep(0.5); 
			status = shell_execute();

			// ensure all acquired shared memory are released
			switch(status){
			   	case 1:
			   		shm_unlink(target_function); // clear parameters
			   	case 3:
			   		shm_unlink("command"); // clear complete command
			   		break;
			   	case 2:
			   		shm_unlink(target_function);
			   		break;
			   	default:
			   		break;
			}
		}
		else
			status = 1;

		free(command);
		free(parameters);
	} while (status);
}

char *shell_readline(void)
{
	int BUFLEN = ARG_BOUND;
	int byte, index = 0;
	char *line = malloc(sizeof(char)*BUFLEN);

	if (!line){
		fprintf(stderr, "Exception : %s", get_error_description(1));
		exit(EXIT_FAILURE);
	}

	while (1){
		byte = getchar();

		if (byte==EOF)	exit(EXIT_SUCCESS);
		else if (byte=='\n'){
			line[index] = '\0';
			return line;
		} else	line[index++] = byte;

		if (index >= BUFLEN){
			BUFLEN += ARG_BOUND;
			line = realloc(line, BUFLEN);
			if (!line){
				fprintf(stderr, "Exception : %s", get_error_description(2));
				exit(EXIT_FAILURE);
			}
		}
	}
	return line;
}

int arguement_processor(const char *command, char **target_function, char **parameters)
{
	if (!command){
		fprintf(stderr, "Exception : %s\n", get_error_description(3));
		return 1;
	}

	char *copy_command = malloc(sizeof(char) * strlen(command));
	strcpy(copy_command, command);
	*target_function = target_getter(copy_command);
	*parameters = parameters_getter(command);
	return 1;
}

int shell_execute(void)
{
	int i;
	char *function_name = read_from_share_memory("function_name");

	if (!function_name){
		fprintf(stderr, "Exception : %s\n", get_error_description(3));
		return 1;
	}

	for (i=0; i<4; i++){
		if(strcmp(builtin_str[i], function_name)==0){
			free(function_name);
			return (*builtin_func[i])();
		}
	}

	free(function_name);

	// not built-in function
	shell_launch();
	return 2;
}

int shell_launch(void)
{
  	pid_t pid;
  	int status;
  	char *command = read_from_share_memory("command");
  	char **args = command_splitter(command);

  	pid = fork();
  	if (pid == 0) {
    	// Child process
    	if (execvp(args[0], args) == -1) { 	// return -1 when system call failed
    		perror("Jonah\'s Shell"); 		// lookup error message from strerror then output to stderr
    										// arguement[0] is the name of failed function
    	}
    	exit(EXIT_FAILURE);
  	} else if (pid < 0) {
    	// Error forking
    	perror("Jonah\'s Shell");
  	} else {
    	// Parent process
    	do {
      		waitpid(pid, &status, WUNTRACED);
      	//	WIFEXITED -> WAIT_IF_EXITED -> specified child terminated normally
      	//	WIFSIGNALED -> WAIT_IF_SIGNALED -> specified child terminated by signal
    	} while (!WIFEXITED(status) && !WIFSIGNALED(status)); // escape if either one not satisfied
  	}

  	free(command);
  	free(args);
  	return 1;
}

char *target_getter(char *command)
{
	if (!command){
		fprintf(stderr, "Exception : %s\n", get_error_description(3));
		exit(EXIT_FAILURE);
	}
	char *target = strtok(command, TOK_DELIM);
	if (!target){
		fprintf(stderr, "target getter\n");
		fprintf(stderr, "Exception : %s\n", get_error_description(4));
		// exit(EXIT_FAILURE);
	}
	return target;
}

char *parameters_getter(const char *command)
{
	char *parameters;
	int i, p;
	int size = strlen(command);
	for (i=0; i<strlen(command); i++){
		if (command[i]==' ') break;
		size--;
	}
	parameters = malloc(sizeof(char) * size);
	if (!parameters){
		fprintf(stderr, "Exception : %s", get_error_description(1));
		exit(EXIT_FAILURE);
	}
	i++; // stepover space
	p = 0;
	while (i<strlen(command)){
		parameters[p] = command[i];
		p++;
		i++;
	}
	return parameters;
}

char **command_splitter(char *command)
{
	int BUFLEN = ARG_BOUND/2, index = 0;
	char *token;
	char **tokens = malloc(BUFLEN * sizeof(char *));

	if (!tokens){
		fprintf(stderr, "Exception : %s", get_error_description(1));
		exit(EXIT_FAILURE);
	}

	token = strtok(command, TOK_DELIM);
	while (token != NULL){
		tokens[index++] = token;

		if (index >= BUFLEN){
			BUFLEN += ARG_BOUND/2;
			tokens = realloc(tokens, BUFLEN * sizeof(char *));
			if (!tokens){
				fprintf(stderr, "Exception : %s", get_error_description(1));
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL, TOK_DELIM);
	}
	return tokens;
}

int write_to_share_memory(const char *target_function, const char *parameters)
{

  	int shm_fd;
  	void *ptr;

  	shm_fd = shm_open(target_function, O_CREAT | O_RDWR, 0666);
  	ftruncate(shm_fd, SHM_BOUND);
  	ptr = mmap(NULL, SHM_BOUND, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
  	
  	if(!ptr){
  		fprintf(stderr, "Exception : %s", get_error_description(5));
		exit(EXIT_FAILURE);
  	}

  	sprintf(ptr, "%s", parameters);

	return 1;
}

char *read_from_share_memory(const char *target_function)
{
	int shm_fd;
	void *ptr;
	char *line;

	shm_fd = shm_open(target_function, O_RDONLY, 0666);
	ptr = mmap(NULL, SHM_BOUND, PROT_READ, MAP_SHARED, shm_fd, 0);
	line = malloc(sizeof(char)*strlen((char*)ptr));

	strcpy(line, (char*)ptr);

	shm_unlink(target_function);
	return line;
}

int shell_cd(void)
{
	char *message = read_from_share_memory("cd");
	
	if (chdir(message)!=0)	perror("change directory error");

	free(message);
	return 3;
}

int shell_help(void)
{
	int i;
  	printf("Jonah's Shell\n");
  	printf("Type program names and arguments, and hit enter.\n");
  	printf("The following are built in:\n");

  	for (i = 0; i < 4; i++)	printf("  %s\n", builtin_str[i]);

  	printf("Use the man command for information on other programs.\n");
  	return 1;
}

int shell_exit(void)
{
	printf("Exiting shell...\n");
	return 0;
}

int shell_BinToIntelHexFormat(void)
{
	char *command = read_from_share_memory("command");
  	char **args = command_splitter(command);
  	
  	if (strlen(*args)>=3)
	  	convert(args[1], args[2]);
	else
  		fprintf(stderr, "Exception : %s", get_error_description(5));

	free(command);
  	free(args);
	return 1;
}


