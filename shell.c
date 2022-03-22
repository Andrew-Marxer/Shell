#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <linux/limits.h>

int parse(char* str, char** strParsed, char** pipeParsed);
//prints current dir
void printPrompt(){
	char prompt[PATH_MAX];
	//current directory
	getcwd(prompt, sizeof(prompt));
	printf("%s", prompt);
}

//read input from command line
int getInput(char * str){
	char * buf;

	buf = readline("$ ");
	if (strlen(buf) != 0){
		strcpy(str, buf);
		if(!strchr(str,'!'))
		{	
		add_history(str);
		}
		free(buf);
		return 0;
	}
	else{
	return 1;
	}	
}
//find pipes, seperate each section into its own array index,
//return number of pipes
int findPipe(char* str, char ** parsed){
	int numPipes = 0;
	int i = 0;
	while(1){	
		
		parsed[i] = strsep(&str, "|");
		//printf("finding pipe, parsed[i]: %s\n", parsed[i]);
		if(parsed[i] == NULL)
			break;
	//	printf("parsed not null\n");
		//if(strcmp(parsed[i],str) == 0)
		//	break;
		numPipes++;
		i++;
	//	printf("loop no. %d\n" , i);
	}
	if (numPipes > 0)
		numPipes--;
	return numPipes;
}

int findSpaces(char * str, char ** parsed){
	int i =0; 
	int numSpaces = 0;
	while(1){
	parsed[i] = strsep(&str, " ");
//	printf("finding spaces, parsed[%d]: %s\n",i,parsed[i]);
	numSpaces++;

	if(parsed[i] == NULL)
		break;
	i++;
	}
	
	return numSpaces;
}

int doPipeExec(char** pipeParsed, int sCounter){
	//printf("pipeParsed[0]: %s\n" , pipeParsed[0]);
	//printf("pipeParsed[1]: %s\n" , pipeParsed[1]);
	//printf("pipeParsed[2]: %s\n" , pipeParsed[2]);
	//printf("entered pipeExec, stage counter = %d\n",sCounter);
	int** pipes;
	pipes = (int **) malloc((sCounter-1) * sizeof(int *));
	for(int i = 0; i < sCounter-1; i++)
	{	//printf("malloc on each pipe pair\n");
		pipes[i] = (int *)malloc(2*sizeof(int));
		//printf("malloc done\n");
		pipe(pipes[i]);
		//printf("Created pipe: %d\n", i);
	}

	pid_t pid;
	int status = 0;
	//	printf("created pipes\n");
	
//	printf("sCounter: %d\n",sCounter);
	for(int i = 0; i < sCounter; i++)
	{
		/*if (i == 1){
			close(pipes[i][0]);
		}
		if(i ==2){
			close(pipes[i][1]);
		}*/
			//printf("i: %d\n", i);

		pid = fork();
		if(pid < 0 ){
			perror("fork failed\n");
		}
		if(pid == 0){

			//printf("forked\n");
			if (i == 0)
			{
				//close read of stage 0
			//	printf("stage %d close reader%d\n", i, i);
				if(close(pipes[i][0]) < 0)
				{
					perror("close");
					exit(1);
				}
				
			}
			else
			{
				if(dup2(pipes[i-1][0],STDIN_FILENO)<0)
				{
					perror("dup2");
					exit(1);
				}
			//	printf("stage %d reads %d\n", i, i-1);
			
			}
			//close all read ends except i-1
			for(int j = 1; j < sCounter-1; j++)
			{
				if (j != (i-1))
				{
					if(close(pipes[j][0])< 0)
					{
						perror("close");
						exit(1);
					}
			//		printf("stage %d closed reader%d\n",i,j);
				}
			}

			
			//close all write ends except i
			for(int j = 0; j < sCounter-1; j++)
			{
				if(j != i)
				{
					if(close(pipes[j][1])< 0)
					{
						perror("close");
						//printf("this one\n");
						exit(1);
					}
			//		printf("stage %d closed writer%d\n",i,j);
				}
			}
		/*	if(i == sCounter - 1)
			{	printf("stage %d close writer %d\n",i,i-1);
				if(close(pipes[i-1][1]) < 0)
				{
					perror("close");
					exit(1);
				}
			}*/
			if(i != sCounter-1)
			{
				if (dup2(pipes[i][1], STDOUT_FILENO) < 0)
				{
					printf("dup2 error\n");
					exit(EXIT_FAILURE);
				}
			//	printf("stage %d writes to %d\n",i,i);
			}
			//printf("start tokenizing\n");
			int x = 0;
			char * token;
			char * args[10];
		//	printf("first token: %s\n",pipeParsed[0]);
		//	printf("i: %d\n", i);
			token = strtok(pipeParsed[i]," ");
		///	printf("token: %s\n", token);
			while (token){
				args[x] = token;
		//		printf("args[x]: %s\n" , args[x]);
				x++;
				token = strtok(NULL," ");
		//		printf("token: %s\n", token);
			}
			args[x] = NULL;
			if(execvp(args[0],args) < 0){
				printf("exec error\n");
				exit(EXIT_FAILURE);
			}
			
		
		}
	}
			wait(NULL);
		//while((wpid = waitpid(&status)) > 0);	
		for(int i = 0; i < sCounter-1; i++)
		{
			close(pipes[i][0]);
			close(pipes[i][1]);
			free(pipes[i]);
		}

		free(pipes);
	return 0;
}
int search(char * str)
{	char * substr;
	if(strcmp(str,"!") == 0)
	{
		int i = 1;
		while(str[i] != '\0')
		{
			substr[i-1] = str[i];
			i++;
		}
		int num = atoi(substr);
		return num;
	}
	else
	return -1;
}

int doExec(char ** strParsed)
{
	pid_t pid;
 	const char * num;
	HIST_ENTRY ** the_list;
	HIST_ENTRY * element;
	the_list = history_list();
	//printf("inside doExec\n");
	int n;
	int loc = 0;
	int redirect = -1;
	while(strParsed[loc] != NULL)
	{
		if((strcmp(strParsed[loc],"<") == 0) || (strcmp(strParsed[loc],">") == 0))
		{//	printf("found redirect\n");
			redirect = 1;
			break;
		}
		loc++;
	}
	num = strchr(strParsed[0], '!');
	if(num)
	{	char * temp;
	//	printf("before conversion: %s\n", num+1);
		num += 1;
		n = atoi(num);
	//	printf("!history\n");
	//	printf("search history: %d\n", n);
		element = history_get(n);
		replace_history_entry(n-1,element->line,NULL);
	//	temp = the_list[n-1]->line;
	//	printf("from history: %s\n",element->line);
		add_history(element->line);
		parse(element->line, strParsed, strParsed);	
	
	}	
	else if(strcmp(strParsed[0], "hist") == 0)
	{	//printf("History:\n");
		fflush(stdout);
		HIST_ENTRY ** the_list;
		int i;
		the_list = history_list();

		if(the_list)
		{
			for(i = 0; the_list[i]; i++)
			{
				printf("%d: %s\n", i+ history_base, the_list[i]->line);
			}
		}
	}
	else if(redirect == 1)
	{
		if(strcmp(strParsed[loc],"<") == 0)
		{
			char * args[10];
			int fd;
			if(fd = open(strParsed[loc + 1],0) < 0)
			{
				perror("failed to open file, maybe it doesn't exist\n");
				exit(1);
			}
			pid = fork();
			if (pid == 0)
			{
				if(dup(fd) < 0)
				{
					perror("dup failed\n");
					exit(1);
				}
				close(fd);
				printf("duped\n");
				//build argv
				for(int i = 0; i < loc; i++)
				{
					args[i] = strParsed[i];
				}
				args[loc] = '\0';
				if(execvp(args[0],args) < 0)
				{
					perror("exec failed\n");
					exit(1);
				}
				
				exit(0);
			}
			
		}	
		else
		{
			int fd;
			char * args[100];
			fd = open(strParsed[loc+1], O_RDWR|O_CREAT);
			if(fd < 0)
			{
				perror("outfile");
				exit(1);
			}
			printf("opened file\n");
			pid = fork();
			if(pid == 0)
			{	printf("forked\n");
				if(dup2(fd,STDOUT_FILENO) < 0)
				{
					perror("dup");
					exit(1);
				}
				printf("duped\n");
				close(fd);
				for(int i = 0; i < loc; i++)
				{
					args[i] = strParsed[i];
					printf("%s\n",args[i]);
				}
				args[100] = '\0';
				if(execvp(args[0],args) < 0)
				{
					perror("exec");
					exit(1);	
				}

			}
		}
	}
	else if(strcmp(strParsed[0], "cd") == 0){
		if(chdir(strParsed[1]) != 0)
			perror("cd failed\n");
	}
	else if(strcmp(strParsed[0] , "quit") == 0)
	{	printf("THANK YOU FOR USING MY SHELL!\n");
		printf("	GOODBYE \n");
		exit(0);
	}
	else
	{
	pid = fork();
	if(pid < 0)
	{
		perror("fork");
		exit(1);
	}
	if(pid == 0)
	{
		if(execvp(strParsed[0],strParsed) < 0)
		{
			perror("exec failed\n");
			exit(1);
		}
	
	}
	}
	wait(NULL);


}//parse command line, if contains pipe, execPipe. else exec line
int parse(char* str, char** strParsed, char** pipeParsed){
	char* pipeReturn[10];
	int pipes = findPipe(str, pipeParsed);
//	printf("pipeParsed[0]: %s\n" , pipeParsed[0]);
//	printf("pipeParsed[1]: %s\n" , pipeParsed[1]);
//	printf("pipeParsed[2]: %s\n" , pipeParsed[2]);
//	printf("pipes %d\n", pipes);
	if(pipes > 0 ){
		doPipeExec(pipeParsed, pipes+1);
	}
	else
	{
	findSpaces(str, strParsed);
	//printf("strPArsed[0]: %s\n",strParsed[0]);
	//printf("strPArsed[1]: %s\n",strParsed[1]);
	doExec(strParsed);
	}
//	printf("returns here\n");
	return 1 + pipes;
}




//main loop
int main(){
	char input[1000];
	char* strParse[1000];
	char* pipParse[1000];
	char varTable[50][2];
	int flag = 0;
	using_history();
	printf("******************************\n");
	printf("                              \n");
	printf("    WELCOME TO MY SHELL       \n");
	printf("  BUILT BY: ANDREW MARXER     \n");
	printf("        CSCE 3600             \n");
	printf("     	COMMANDS:             \n");
	printf("                              \n");
	printf("   \"hist\" list history      \n");
       	printf(" run a previous command: \"!#\"\n");
	printf(" \"cd\" to change directory   \n");
	printf(" supports piping and exec     \n");
	printf("   \"quit\" when finished     \n");
	printf("                              \n");
	printf("******************************\n");	
	while(1){
	//print director followed by $
	printPrompt();
	
	if (getInput(input))
	{
		continue;
	}
	
	flag = parse(input, strParse, pipParse);
	wait(NULL);
	
	}
	return 0;
}
