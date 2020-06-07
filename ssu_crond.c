#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#define BUFFER_SIZE 1024

int check_execute(char* ptr, int num, int min_limit, int max_limit);

int main(void)
{
	struct tm *tmbuf;
	time_t t;
	int i;
	FILE *log, *file;
	char buf[BUFFER_SIZE];
	char cmd[BUFFER_SIZE];
	char *token, *ptr;
	//log파일 오픈
	if(access("ssu_crontab_log",F_OK) == 0){
		if((log = fopen("ssu_crontab_log", "a+")) == NULL){
			fprintf(stderr,"ssu_crond fopen error\n");
			exit(1);
		}
	}
	else{
		if((log = fopen("ssu_crontab_log", "w+")) == NULL){
			fprintf(stderr,"ssu_crond fopen error\n");
			exit(1);
		}
	}
	//무한반복문 수행
	while(1)
	{
		//파일이 존재하지않으면 sleep
		if(access("ssu_crontab_file",F_OK) != 0){
			sleep(1);
			continue;
		}
		//시간측정
		time(&t);
		tmbuf = localtime(&t);
		//0초가 아니면 continue
		if(tmbuf->tm_sec != 0)
			continue;
		//파일오픈
		if((file = fopen("ssu_crontab_file","r+")) == NULL){
			fprintf(stderr,"ssu_crond fopen error\n");
			exit(1);
		}
		while(!feof(file)){
			memset(buf,0,BUFFER_SIZE);
			memset(cmd,0,BUFFER_SIZE);
			fgets(buf,BUFFER_SIZE,file);
			//개행문자 널문자로 변경
			buf[strlen(buf)-1] = '\0';
			if(strlen(buf) == 0)
				continue;
			memcpy(cmd,buf,BUFFER_SIZE);
			//인덱스. 문자열은 버림
			token = strtok(buf," ");
			//min분석
			token = strtok(NULL," ");
			if(check_execute(token,tmbuf->tm_min, 0,59))
				continue;
			token = strtok(NULL," ");
			if(check_execute(token,tmbuf->tm_hour, 0,23))
				continue;
			token = strtok(NULL," ");
			if(check_execute(token,tmbuf->tm_mday, 1,31))
				continue;
			token = strtok(NULL," ");
			if(check_execute(token,tmbuf->tm_mon+1, 1,12))
				continue;
			token = strtok(NULL," ");
			if(check_execute(token,tmbuf->tm_wday, 0,6))
			continue;
			token = strstr(cmd, " ");
			token++;
			for(i = 0; i < 5; i++){
				token = strstr(token," ");
				token ++;
			}
			//명령어 수행
			system(token);
			ptr = ctime(&t);
			token = strstr(cmd, " ");
			token++;
			ptr[strlen(ptr)-1] = '\0';
			//log 출력
			fprintf(log, "[%s] run %s\n", ptr, token);
			fflush(log);
		}
		fclose(file);
		//1초에 한번씩 수행
		sleep(1);
	}
}
int check_execute(char* ptr, int num, int min_limit, int max_limit)
{
	char number[10];
	int curr_num;
	int start_num;
	int end_num;
	int i, j;

	i = 0;
	while(ptr[i] != '\0')
	{
		//*문자이면
		if(ptr[i]== '*'){
			// *문자 하나면 통과
			if(ptr[i+1] == '\0')
				return 0;
			else if(ptr[i+1] == '/'){
				// /다음 문자로 이동
				i+=2;
				j = 0;
				while(ptr[i] >= '0' && ptr[i] <= '9')
					number[j++] = ptr[i++];
				number[j] = '\0';
				curr_num = atoi(number);
				//전체값에서 curr_num(점프)하면서 일치하는지 비교
				//범위가 *이므로 start, end범위값 limit값으로 대체
				//*/2인 경우 1,3,5,7~ */3인 경우 2,5,8,11~
				for(j = min_limit + curr_num-1; j <= max_limit; j+= curr_num){
					//해당하는 항목의 시간과 같으면
					if(j == num)
						return 0;
				}
				//,문자일 경우 다음 문자로 이동
				if(ptr[i] == ',')
					i++;
			}
			// *문자 뒤에 /나 \0문자가 아니면
			else{
				printf("Error : 실행주기 표기가 잘못되었습니다.\n");
				return 1;
			}
		}
		//숫자이면
		else if(ptr[i] >= '0' && ptr[i] <= '9'){
			j = 0;
			while(ptr[i] >= '0' && ptr[i] <= '9')
				number[j++] = ptr[i++];
			number[j] = '\0';
			curr_num = atoi(number);
			//범위지정문자 이면
			if(ptr[i] == '-'){
				//curr_num을 start_num으로 지정
				start_num = curr_num;
				// -다음 문자로 이동
				i++;
				j = 0;
				//-이후 end_num을 구함
				while(ptr[i] >= '0' && ptr[i] <= '9')
					number[j++] = ptr[i++];
				number[j] = '\0';
				end_num = atoi(number);
				//다음에 /로 연속되어있으면
				if(ptr[i] == '/'){
					//다음 숫자로 이동
					i++;
					j = 0;
					// /이후 curr_num을 구함
					while(ptr[i] >= '0' && ptr[i] <= '9')
						number[j++] = ptr[i++];
					number[j] = '\0';
					curr_num = atoi(number);
					//end_num포함
					for(j = start_num + curr_num-1; j <= end_num; j+= curr_num){
						//해당하는 항목의 시간과 같으면
						if(j == num)
							return 0;
					}
				}
				//범위항목만 있으므로 해당조건 충족하면 return 0
				else if(start_num <= num && end_num >= num) 
					return 0;
			}
			//숫자하나만 있는경우
			else if(ptr[i] == '\0' || ptr[i] == ','){
				if(curr_num == num)
					return 0;
			}
			//이외에 문자가 있으면 에러처리
			else{
				printf("Error : 실행주기 표기가 잘못되었습니다.\n");
				return 1;
			}
			//, 문자일 경우 다음 문자로 이동
			if(ptr[i] == ',')
				i++;
		}
		//, 숫자 *문자 이외에 경우
		else{
			printf("Error : 실행주기 표기가 잘못되었습니다.\n");
			return 1;
		}
	}
	return 1;
}
