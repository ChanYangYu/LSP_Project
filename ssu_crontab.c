#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>
#define BUFFER_SIZE 1024

int command_num = 0;

//함수 프로토타입
void execute_add(FILE* fp, char* cmd);
int check_run_cycle(char* cmd, int index);
void execute_remove(FILE* fp, char* cmd);
void runtime(struct timeval *begin, struct timeval *end);

//ssu_crontab
int main(void)
{
	struct timeval begin, end;
	FILE* fp;
	char buf[BUFFER_SIZE];
	char cmd_name[10];
	char* fname = "ssu_crontab_file";
	char* ptr;
	int i;

	//ssu_crontab_file이 존재하지않으면 생성
	if(access(fname,F_OK) != 0){
		if((fp = fopen(fname,"w+")) == NULL){
			fprintf(stderr,"%s file create fail\n",fname);
			exit(1);
		}
	}
	//존재하면 파일오픈
	else{
		if((fp = fopen(fname,"r+")) == NULL){
			fprintf(stderr,"%s file open fail\n",fname);
			exit(1);
		}
	}
	gettimeofday(&begin, NULL);
	while(!feof(fp)){
		//파일내용 출력
		memset(buf,0,BUFFER_SIZE);
		fgets(buf,BUFFER_SIZE,fp);
		printf("%s",buf);
		//buf가 비어있지 않을경우
		if(strlen(buf) != 0)
			command_num++;
	}
	printf("\n");
	while(1)
	{
		memset(buf,0,BUFFER_SIZE);
		printf("20162484> ");
		fgets(buf,BUFFER_SIZE,stdin);
		if(buf[0] == '\n')
			continue;
		buf[strlen(buf)-1] = '\0';
		//exit 문자검사
		if(!strcmp(buf,"exit"))
			break;
		//명령어이름 cmd_name에 저장
		i = 0;
		ptr = buf;
		//명령어 자름
		while(*ptr != ' ' && *ptr != '\0'){
			cmd_name[i] = *ptr;
			ptr++;
			i++;
		}
		cmd_name[i] = '\0';
		//공백문자 넘김
		ptr++;
		if(!strcmp(cmd_name, "add"))
			execute_add(fp,ptr);
		else if(!strcmp(cmd_name, "remove"))
			execute_remove(fp,ptr);
		else
			printf("Error 1 : 없는 명령어입니다.\n");
	}
	gettimeofday(&end, NULL);
	runtime(&begin, &end);
	exit(0);
}

void execute_add(FILE* fp, char* cmd)
{
	char buf[BUFFER_SIZE];
	char* token;
	time_t t;
	FILE* log;

	memset(buf,0,BUFFER_SIZE);
	//토큰으로 분리를 위해 buf에 cmd 복사
	strcpy(buf, cmd);
	//segment 오류를 막기위해 검사
	if((token = strtok(buf, " ")) == NULL){
		fprintf(stderr,"Error 2 : 실행 주기를 잘못 입력하셨습니다.\n");
		return;
	}
	//실행주기 분 check
	if(check_run_cycle(token, 1))
		return;
	//segment 오류를 막기위해 검사
	if((token = strtok(NULL, " ")) == NULL){
		fprintf(stderr,"Error 2 : 실행 주기를 잘못 입력하셨습니다.\n");
		return;
	}
	//실행주기 시 check
	if(check_run_cycle(token, 2))
		return;
	//segment 오류를 막기위해 검사
	if((token = strtok(NULL, " ")) == NULL){
		fprintf(stderr,"Error 2 : 실행 주기를 잘못 입력하셨습니다.\n");
		return;
	}
	//실행주기 일 check
	if(check_run_cycle(token, 3))
		return;
	//segment 오류를 막기위해 검사
	if((token = strtok(NULL, " ")) == NULL){
		fprintf(stderr,"Error 2 : 실행 주기를 잘못 입력하셨습니다.\n");
		return;
	}
	//실행주기 월 check
	if(check_run_cycle(token, 4))
		return;
	//segment 오류를 막기위해 검사
	if((token = strtok(NULL, " ")) == NULL){
		printf("%s",buf);
	}
	if(check_run_cycle(token, 5))
		return;
	printf("\n");
	time(&t);
	//로그 생성
	if(access("ssu_crontab_log", F_OK) != 0)
		log = fopen("ssu_crontab_log", "w+");
	else
		log = fopen("ssu_crontab_log", "a+");
	//현재시간 저장
	token = ctime(&t);
	token[strlen(token)-1] = '\0';
	fprintf(log, "[%s] add %s\n",token, cmd);
	fclose(log);
	fprintf(fp, "%d. %s\n",command_num, cmd);
	//명령어개수 증가
	command_num++;
	//로그추가
	fflush(fp);
	//파일처음으로 이동
	rewind(fp);
	while(!feof(fp)){
		//명령어 목록 출력
		memset(buf,0,BUFFER_SIZE);
		fgets(buf,BUFFER_SIZE,fp);
		printf("%s",buf);
		//buf가 비어있지 않을경우
	}
	printf("\n");
}
int check_run_cycle(char* cmd, int index)
{
	char number[10];
	int i,j;
	int start = 0;
	int prev_number = -1;
	int curr_number;
	int in_slash = 0;
	int in_minus = 0;

	i = 0;
	//널문자 전까지 반복
	while(cmd[i] != '\0'){
		// 문자열 처음이거나 ,문자가 이전에 나왔으면
		if(start == 0 || start == 2){
			//*문자가 아니고 숫자가아니면 에러
			if(cmd[i] != '*' && (cmd[i] < '0' || cmd[i] > '9')){
				fprintf(stderr,"Error 4 : 주기 형식이 잘못되었습니다.\n");
				return 1;
			}
			else{
				//*문자는 무조건 start가 0이거나 2일때 올수있음
				if(cmd[i] == '*'){
					// ,문자가 나왔을때는 단순히 *단독으로는 실행주기가 안맞음
					if(start == 2 && cmd[i+1] == '\0'){
						fprintf(stderr,"Error 5 : ,문자 이후에 *단독으로 사용할 수 없습니다.\n");
						return 1;
					}
					//*문자는 뒤에 아무것도 없거나 /문자가 뒤에 있어야함
					if(cmd[i+1] != '\0' && cmd[i+1] != '/'){
						fprintf(stderr,"Error 4 : 주기 형식이 잘못되었습니다.\n");
						return 1;
					}
					//*문자는 100으로 정의
					prev_number = 100;
					//다음문자로 이동
					i++;
				}
				//숫자이면
				else if(cmd[i] >= '0' && cmd[i] <= '9'){
					j = 0;
					number[j++] = cmd[i++];
					while(cmd[i] >= '0' && cmd[i] <= '9')
						number[j++] = cmd[i++];
					number[j] = '\0';
					curr_number = atoi(number);
					//범위에 들어가는지 확인
					if(index == 1 &&(curr_number < 0 || curr_number > 59)){
						fprintf(stderr,"Error 6 : 분(항목)의 범위를 벗어났습니다.\n");
						return 1;
					}
					else if(index == 2 &&(curr_number < 0 || curr_number > 23)){
						fprintf(stderr,"Error 7 : 시(항목)의 범위를 벗어났습니다.\n");
						return 1;
					}
					else if(index == 3 &&(curr_number < 1 || curr_number > 31)){
						fprintf(stderr,"Error 8 : 일(항목)의 범위를 벗어났습니다.\n");
						return 1;
					}
					else if(index == 4 &&(curr_number < 1 || curr_number > 12)){
						fprintf(stderr,"Error 9 : 월(항목)의 범위를 벗어났습니다.\n");
						return 1;
					}
					else if(index == 5 &&(curr_number < 0 || curr_number > 6)){
						fprintf(stderr,"Error 10 : 요일(항목)의 범위를 벗어났습니다.\n");
						return 1;
					}
					//prev_number에 값대입
					prev_number = curr_number;
				}
			}
			//start 1로 만듬
			start = 1;
		}
		//start가 0이아니면
		else{
			// -문자는 뒤에 /문자가 나올 수 있다
			if(cmd[i] == '-'){
				// /문자가 앞에 나오거나, -문자가 두번이상 나오면 에러
				if(in_slash || in_minus){
					//-연결값 이전에 /이 나오는것은 문법이 맞지않음
					fprintf(stderr,"Error 4 : 주기 형식이 잘못되었습니다.\n");
					return 1;
				}
				//-문자 다음에는 무조건 숫자가 와야함
				if(!(cmd[i+1] >= '0' && cmd[i+1] <= '9')){
					fprintf(stderr,"Error 4 : 주기 형식이 잘못되었습니다.\n");
					return 1;
				}
				i++;
				in_minus++;
			}
			//같은문자는 ,이전에 두번이상 못나옴
			else if(cmd[i] == '/'){
				if(in_slash){
					// /이 ,이전에 두번 나오는것은 문법이 맞지않음
					fprintf(stderr,"Error 4 : 주기 형식이 잘못되었습니다.\n");
					return 1;
				}
				//범위가 없거나 *문자가 아니면
				if(in_minus == 0 && prev_number != 100){
					fprintf(stderr,"Error 11: /문자 이전에는 범위 또는 *문자를 사용해야합니다.\n");
					return 1;
				}
				// /문자 뒤에는 반드시 숫자가 와야함
				if(!(cmd[i+1] >= '0' && cmd[i+1] <= '9')){
					fprintf(stderr,"Error 4 : 주기 형식이 잘못되었습니다.\n");
					return 1;
				}
				i++;
				in_slash++;
			}
			else if(cmd[i] == ','){
				//start 2로변경
				start = 2;
				//이전숫자 -1로 변경
				prev_number = -1;
				//slash,minus개수 0으로 변경
				in_slash = 0;
				in_minus = 0;
				// ,문자뒤에는 *또는 숫자만 올 수 있음
				if(cmd[i+1]!= '*' && !(cmd[i+1] >= '0' && cmd[i+1] <= '9')){
					fprintf(stderr,"Error 4 : 주기 형식이 잘못되었습니다.\n");
					return 1;
				}
				//다음 문자로 이동
				i++;
			}
			else if(cmd[i] >= '0' && cmd[i] <= '9'){
				j = 0;
				number[j++] = cmd[i++];
				while(cmd[i] >= '0' && cmd[i] <= '9')
					number[j++] = cmd[i++];
				number[j] = '\0';
				curr_number = atoi(number);
				//범위에 들어가는지 확인
				if(index == 1 &&(curr_number < 0 || curr_number > 59)){
					fprintf(stderr,"Error 6 : 분(항목)의 범위를 벗어났습니다.\n");
					return 1;
				}
				else if(index == 2 &&(curr_number < 0 || curr_number > 23)){
					fprintf(stderr,"Error 7 : 시(항목)의 범위를 벗어났습니다.\n");
					return 1;
				}
				else if(index == 3 &&(curr_number < 1 || curr_number > 31)){
					fprintf(stderr,"Error 8 : 일(항목)의 범위를 벗어났습니다.\n");
					return 1;
				}
				else if(index == 4 &&(curr_number < 1 || curr_number > 12)){
					fprintf(stderr,"Error 9 : 월(항목)의 범위를 벗어났습니다.\n");
					return 1;
				}
				else if(index == 5 &&(curr_number < 0 || curr_number > 6)){
					fprintf(stderr,"Error 10 : 요일(항목)의 범위를 벗어났습니다.\n");
					return 1;
				}
				//-문자가 있고
				if(in_minus){
					//그 뒤에 in_slash가 있으면
					if(in_slash){
						//주기를 나타내므로 이전값이 더 커야함
						if(prev_number < curr_number){
							fprintf(stderr, "Error 12 : 건너뛰는 숫자는 범위보다 클 수 없습니다.\n");
							return 1;
						}
					}
					//minus만 있으면 범위이므로 이전값이 더 작아야함
					else if(prev_number > curr_number){
						fprintf(stderr,"Error 13 : 값 사이의 범위 지정이 잘못되었습니다.\n");
						return 1;
					}
				}
				// /문자가 있으면
				else if(in_slash){
					//주기를 나타내므로 이전값이 더 커야함
					if(prev_number < curr_number){
						fprintf(stderr, "Error 12 : 건너뛰는 숫자는 범위보다 클 수 없습니다.\n");
						return 1;
					}
				}
				//prev_number업데이트
				prev_number = curr_number;
			}
			//이외에 문자는 모두 에러처리
			else{
				fprintf(stderr,"Error 4 : 주기 형식이 잘못되었습니다.\n");
				return 1;
			}
		}
	}
	return 0;
}
void execute_remove(FILE* fp, char* cmd)
{
	FILE* temp;
	FILE* log;
	time_t t;
	char buf[BUFFER_SIZE];
	char* ptr, *ptr2;
	int number;
	int i;

	i = 0;
	//command_number를 입력하지 않았을 경우
	if(cmd[i] == '\0'){
		fprintf(stderr,"Error 3 : COMMAND_NUMBER를 잘못입력하셨습니다.\n");
		return;
	}
	//command_number 체크
	while(cmd[i] != '\0'){
		if(!(cmd[i] >= '0' && cmd[i] <= '9')){
			fprintf(stderr,"Error 3 : COMMAND_NUMBER를 잘못입력하셨습니다.\n");
			return;
		}
		i++;
	}
	number = atoi(cmd);
	//초과하는지 비교
	//number는 인덱스이고 command_num은 개수이므로 +1해서 비교
	if(number+1 > command_num){
		fprintf(stderr,"Error 3 : COMMAND_NUMBER를 잘못입력하셨습니다.\n");
		return;
	}
	//임시파일 생성
	temp = fopen("crontab_temp","w+");
	chmod("crontab_temp",S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
	rewind(fp);
	i = 0;
	while(!feof(fp)){
		//한 라인씩 읽어들임
		memset(buf,0,BUFFER_SIZE);
		fgets(buf,BUFFER_SIZE,fp);
		//삭제하고자 하는 라인이면
		if(number == i){
			//해당 라인 로그에 찍기위해 cmd에저장
			strcpy(cmd,buf);
			i++;
			continue;
		}
		//이전인덱스는 그대로 저장
		else if(number > i)
			fprintf(temp,"%s",buf);
		//삭제된 인덱스 이후는 인덱스 변경필요
		else{
			ptr = buf;
			if((ptr = strstr(buf,".")) == NULL)
				//빈문자열이면 반복문탈출
				break;
			//숫자를 하나 줄여서 저장
			fprintf(temp,"%d%s",i-1,ptr);
		}
		i++;
	}
	//전체 명령어 개수 하나--
	command_num--;
	fflush(temp);
	//파일 덮어쓰기
	rename("crontab_temp","ssu_crontab_file");
	//이전파일 close
	fclose(fp);
	//새파일로 다시 오픈
	fopen("ssu_crontab_file","r+");

	//변경된 내용 출력
	while(!feof(fp)){
		memset(buf,0,BUFFER_SIZE);
		fgets(buf,BUFFER_SIZE,fp);
		printf("%s",buf);
	}
	printf("\n");
	//해당 명령어만 자름
	ptr2 = strstr(cmd,".");
	ptr2 +=2;
	//현재 시간을 받아옴
	time(&t);
	//로그파일에 로그생성
	if(access("ssu_crontab_log", F_OK) != 0)
		log = fopen("ssu_crontab_log", "w+");
	else
		log = fopen("ssu_crontab_log", "a+");
	//ctime 개행문자 제거
	ptr = ctime(&t);
	ptr[strlen(ptr)-1] = '\0';
	//로그입력
	fprintf(log, "[%s] remove %s",ptr, ptr2);
	fclose(log);
}
void runtime(struct timeval *begin, struct timeval *end)
{
	end->tv_sec -= begin->tv_sec;

	if(end->tv_usec < begin->tv_usec){
		end->tv_sec--;
		end->tv_usec += 1000000;
	}
	end->tv_usec -= begin->tv_usec;
	printf("Runtime: %ld:%06ld(sec:usec)\n", end->tv_sec, end->tv_usec);
}

