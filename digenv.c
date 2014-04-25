/*
* NAME
*   digenv - prints sorts and filters envoriment variables
* 
* SYNOPSIS
*   digenv [grep arguments]
* 
* DESCRIPTION
*   Digenv does the same thing as executing printenv | grep [args] | sort | less
*   Grep is omitted if no arguments are specified
* 
* Author
*   Paul Griffin, pgriffin@kth.se
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int executePipeline(int fin, char** programs[]);
/*Loads data from command line and prepares and starts the execution of the other programs*/
/** main
 *
 * main returns 0 if ok error code if not
 *
 *
 * Def:
 *   if (arguments)
 *       getenc | grep arguments | sort | pager
 *   else
 *       getenv | sort | pager
 *
 */

int 
main(
	int argc,	/* Number of arguments passed in argv*/
	char *argv[])	/* Command line arguments*/
{
	char * pager=getenv("PAGER");
	if (pager==NULL) pager="less";

	char * printenv[] = {"printenv",NULL};
	char * sort[] = {"sort",NULL};
	char * less[] = {pager,NULL};

	int result;
	if (argc>1)
	{
		argv[0] = "grep";
		char ** programs[] = {printenv,argv,sort,less,NULL};
		result = executePipeline(STDIN_FILENO, programs);
	}else{
		char ** programs[] = {printenv,sort,less,NULL};
		result = executePipeline(STDIN_FILENO, programs);
	}
	return result;
}
/** executePipeline
 *
 * executePipeline returns 0 if ok error code if not
 *
 * Def:
 *     Execute all programs with parameters and connects their stdout to the next programs stdin.
 *     The first programs STDIN is fin and thlast programs stdout is stdout.
 *
 */
int
executePipeline(
	int fin, 		/* Filedescriptor number to redirect stdin to */
	char** programs[])	/* list of list of strings. The inner list contains a program and it's arguments */
{
	char * program = *programs[0];
	char ** args = programs[0];

	/*  Check if this is the last program. This is inportant for the last pipe  */
	int lastProgram = 0;
	if (programs[1]==NULL) lastProgram=1;

	int fd[2];
	int ret=pipe(fd);
	if (ret) { fprintf(stderr, "Failed to create pipe\n"); return 1;}


	int childPid=fork();
	if (childPid==-1){ fprintf(stderr, "Could not fork\n"); return 1;}
	if (!childPid)
	{
		/*  Child process - Close and duplicate pipes for reading and writing then execute program  */
		ret = close(fd[0]);
		if (ret == -1) { fprintf(stderr, "Failed to close pipe(non critical)\n");}
		ret = dup2(fin,STDIN_FILENO);
		if (ret == -1) { fprintf(stderr, "Failed to duplicate pipe(STDIN)\n"); return 1;}

		if (!lastProgram)
		{
			ret = dup2(fd[1],STDOUT_FILENO);
			if (ret == -1) { fprintf(stderr, "failed to duplicate pipe(STDOUT)\n"); return 1;}
		}
		ret = close(fd[1]);
		if (ret == -1) { fprintf(stderr, "Failed to close pipe(non critical");}
		execvp(program,args); /* Finds program path(in $PATH) and executes it */
	}
	else
	{
		/* Main process - Wait for return value then recurse  */
		int statval;

		ret = wait(&statval); 	/* wait for child to exit */
		//if (!ret) {printf("Wait failed"); return 1;}
		if (statval!=0) { fprintf(stderr, "Error with code %d from %s\n",statval,program);close(fd[0]); return 1;}
		ret = close(fd[1]);
		if (ret == -1) { fprintf(stderr, "failed to close pipe(non critical)\n");}

		if (!lastProgram)
			return executePipeline(fd[0],programs+1);
	}
	return 0;
}


