#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

void foreground();
pid_t get_pid(char* name);
int main(void)
{
	char buf[BUFFER_SIZE];
	pid_t pid;

	if(access("check",F_OK) < 0)
		mkdir("check",0777);
	sprintf(buf,"gcc background.c -o background.exe");
	system(buf);
	sprintf(buf,"./background.exe &");
	system(buf);

	foreground();

	sprintf(buf,"background.exe");
	pid = get_pid(buf);
	kill(pid, SIGKILL); 

	printf("프로그램을 종료합니다.\n");
	
}

pid_t get_pid(char *name)
{
	pid_t pid;
	char command[64];
	char tmp[64];
	int fd;
	int saved;
	
	//background.txt파일 생성
	memset(tmp, 0, sizeof(tmp));
	fd = open("background.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);

	//ps | grep명령어 실행내용을 background.txt에 저장
	sprintf(command, "ps | grep %s", name);
	//redirection
	saved = dup(1);
	dup2(fd,1);
	system(command);
	dup2(saved,1);
	close(saved);

	lseek(fd, 0, SEEK_SET);
	//tmp에 파일내용 읽어옴
	read(fd, tmp, sizeof(tmp));

	//아무내용도 없으면 종료
	if(!strcmp(tmp, "")){
		unlink("background.txt");
		close(fd);
		return 0;
	}

	//토큰을 잘라내서 pid에 저장
	pid = atoi(strtok(tmp, " "));
	close(fd);

	unlink("background.txt");
	//pid번호 리턴
	return pid;
}
