#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
	if(fork() == 0)
		execlp("tar","tar","-xvf","src.tar","-C","./test",">tar_log.txt",(char*)0);
	while(wait((int*)0) != -1);
	exit(0);
}
