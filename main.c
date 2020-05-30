#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024 

void ssu_crontab(void);
void start_service(void);
void make_service_file(void);

int main(void)
{
	pid_t pid;

	if((pid = fork()) < 0){
		fprintf(stderr,"main fork() error\n");
		exit(1);
	}
	else if(pid == 0){
        execlp("gcc","gcc","ssu_crond.c","-o","ssu_crond.exe",(char*)0);
        exit(0);
    }
    while(wait((int*)0) != -1);
    start_service();
    ssu_crontab();
}
void start_service(void)
{
    uid_t uid;
    int backup;

    uid = getuid();
    if(uid != 0){
        fprintf(stderr,"Error 0 : ssu_crond 데몬을 실행을 위해서는 root 권한이 필요합니다.\n");
        return;
    }
    //service파일 유무확인
    if(access("/etc/systemd/system/crond_2484.service",F_OK) != 0)
        make_service_file();
    backup = dup(2);
    close(2);
    system("systemctl enable crond_2484.service");
    system("systemctl start crond_2484.service");
    dup2(backup,2);
    close(backup);
}

void make_service_file(void)
{
    FILE* fp;
    char path[BUFFER_SIZE];

    getcwd(path,BUFFER_SIZE);

    if((fp = fopen("crond_2484.service","w+")) == NULL){
        fprintf(stderr,"make_serive_file create fail\n");
        exit(1);

    }
    fprintf(fp,"[Unit]\nDescription=crond_20162484\n\n");
    fprintf(fp,"[Service]\nExecStart=%s/ssu_crond.exe\nWorkingDirectory=%s\n",
            path,path);
    fprintf(fp,"\n[Install]\nWantedBy=multi-user.target\n");
    fclose(fp);
    system("mv crond_2484.service /etc/systemd/system/crond_2484.service");
    //system("systemctl daemon-reload");
}
