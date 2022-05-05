#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

int NextCommand(int *cur, int res, struct dirent **namelist, int argc, char* argv[])
{
	int next = *cur;
	int find = 0, step = 1;
	while((next < argc) && (find < 2))
	{
		for(int i = 0; (i < res) && (find < step); i++)
		{
			if (strcmp(argv[next],namelist[i]->d_name) == 0)  find++;
		}
		if (find == 0)  *cur++;	  
		if (find == 1)  step = 2;
		next++;		
	}
	switch(find)
	{
	case 2: return (next - *cur - 1); break;
	case 1: return (next - *cur); break;
	default: return -1;
	}
}

int Changer(char* name, int src, int flags)
{
	int file = open(name, O_CREAT | flags, 0644); //wr-r-r
	if (file == -1)
	{
		fprintf(stderr, "Could not open file %s \n", name);
		return -1;
	}
	if (dup2(file, src) == -1)
	{
		fprintf(stderr, "Could not override handle %d \n", src);
		while (close(file) == -1);
		return -1;
	}
	while (close(file) == -1);
	return src;
}

int Redirect(char* command[], int last)
{
	int res = 3;
	int offset = 0;
	if (strstr(command[last], "<") || strstr(command[last - 1], "<"))
	{
		if (command[last][0] == '<')  offset = 1;
		res = Changer(&command[last][offset], STDIN_FILENO, O_RDONLY);
	}
	if (strstr(command[last], ">>") || strstr(command[last - 1], ">>"))
	{
		if (command[last][0] == '>')  offset = 2;
		res = Changer(&command[last][offset], STDOUT_FILENO, O_WRONLY | O_APPEND);
	} 
	if (strstr(command[last], ">") || strstr(command[last - 1], ">"))
	{
		if (command[last][0] == '>')  offset = 1;
		res = Changer(&command[last][offset], STDOUT_FILENO, O_WRONLY | O_TRUNC);
	}
	if ((res != -1) && (res != 3))
		if (offset > 0) command[last] = NULL;
		else command[last - 1] = NULL;
	return res;
}

int Execution(char* path, char* command[], int len)
{
	int saveIn = dup(STDIN_FILENO);
	int saveOut = dup(STDOUT_FILENO);
	int direction = Redirect(command, len - 1);
	pid_t child = fork();
	switch(child)
	{
	case -1:
		perror("Fork creation error\n");
		return -1;
	case 0: 
		if (execv(path, command) == -1)
		{
			fprintf(stderr, "Could not execute command %s\n", path);
			return -1;
		}
		break;
	default:
		if (waitpid(child, NULL, 0) == -1)
			perror("Waiting child fail\n");
		if (direction == 0)  direction = dup2(saveIn, STDIN_FILENO);
		else if (direction == 1)  direction = dup2(saveOut, STDIN_FILENO);
		if (direction == -1)  perror("Could not return handle\n");
		close(saveIn);
      		close(saveOut);
	}
      	return 0;
}

int main(int argc, char* argv[])
{
	if (argc < 2) 
	{
		perror("Not enough params. You should have at least 1 param \n");
		return -1;
	}
	
	struct dirent **namelist;
	int res = scandir("/bin", &namelist, 0, NULL);
	if (res == -1) 
	{
		perror("Commands load error\n");
		return -1;
	}
	
	char* path = calloc(_SC_TRACE_NAME_MAX, sizeof(char));
	if (path == NULL)
	{
		perror("Could not allocate memory\n");
		return -1;
	}
	
	int cur = 1;
	while(cur < argc) 
	{
		int len = NextCommand(&cur, res, namelist, argc, argv);
		if (len == -1)
		{
			perror("Error of reading command\n");
			return -1;
		}
		char* command[len + 1];
		for (int i = 0; i < len; i++)
			command[i] = argv[cur + i];
		command[len] = NULL;
		
		strcat(path, "/bin/");
		strcat(path, command[0]);
		Execution(path, command, len);
		cur += len;	
	}
	
	free(path);
	for(int i = 0; i < res; i++)
		free(namelist[i]);
	free(namelist);
	
	return 0;
}
