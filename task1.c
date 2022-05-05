#include<stdio.h>
#include<unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
int main()
{
	pid_t pid1, pid2;
	pid1 = fork();
	if (pid1 > 0)  pid2 = fork();
	if ((pid1 == -1) || (pid2 == -1)) perror("Proc creation error\n");
	else
	{
		struct timeval tv;
 		if (gettimeofday(&tv, NULL) != -1)
 		{
 			int mil = tv.tv_usec / 1000;
 			int sec = tv.tv_sec % 60;
 			int min = (tv.tv_sec / 60) % 60;
 			int hor = (tv.tv_sec / 3600 + 3) % 24;
 			printf("time:%02d:%02d:%02d:%03d pid:%d parants pid:%d\n", hor, min, sec, mil, getpid(), getppid());
 		}
 		else perror("Getting time error\n");
 		
		if ((pid1 > 0) && (pid2 > 0))
		{
			if (system("ps -x") == -1)  perror("System function error\n");
			if ((waitpid(pid1,NULL,0) == -1) || (waitpid(pid2,NULL,0) == -1)) perror("Waiting error\n");
		}
	}
 	return 0;
}
