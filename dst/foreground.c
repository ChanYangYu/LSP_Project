#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>

#define BUFFER_SIZE 1024
#define ARGUMENT_SIZE 64
#define MAX_ARGUMENT_NUM 4

struct trash_file_info{
	char fname[100];
	char d_time[50];
	char m_time[50];
	int index;
};

char saved_path[BUFFER_SIZE];

void delete(int argc, char **argv);
void delete_to_trash(char *t_path,char *o_path,char *fname);
int delete_endtime(int argc, char **argv);
void check_info_size();
void size(int argc, char **argv);
void recover(int argc, char **argv);
void tree();
void search_tree(int depth, char* d_name);
void help();

void foreground()
{
	char command[BUFFER_SIZE];
	char command_name[ARGUMENT_SIZE];
	char *token;
	char **argv;
	int argc = 0;
	int i;

	//argv 메모리 할당
	argv = malloc(sizeof(char*) * MAX_ARGUMENT_NUM);
	for(i = 0; i < MAX_ARGUMENT_NUM;i++)
		argv[i] = malloc(sizeof(char) * ARGUMENT_SIZE);
	//현재 작업디렉토리 저장
	getcwd(saved_path,BUFFER_SIZE);
	//프롬프트 실행
	while(1)
	{
		printf("20162484>");
		fgets(command,BUFFER_SIZE,stdin);
		//개행문자 제거
		command[strlen(command)-1] = '\0';
		//토큰으로 command 문자열 분할
		if((token = strtok(command," ")) == NULL)
			//더이상 분할될 문자열이 없으면 continue
			continue;
		//exit 명령어가 나오면 종료
		if(!strcmp(token,"exit"))
			break;
		//명령어이름 command_name에 저장
		strcpy(command_name,token);
		//인자값들 argv에 저장
		while((token = strtok(NULL," ")) != NULL)
			strcpy(argv[argc++], token);
		//명령어 수행
		if(!strcmp(command_name,"delete"))
			delete(argc,argv);
		else if(!strcmp(command_name,"size"))
			size(argc,argv);
		else if(!strcmp(command_name,"recover"))
			recover(argc,argv);
		else if(!strcmp(command_name,"tree"))
			tree();
		else if(!strcmp(command_name,"help"))
			help();
		else
			help();
		argc = 0;
	}
}
void delete(int argc, char **argv)
{
	char trash_path[BUFFER_SIZE];
	char origin_path[BUFFER_SIZE];
	char filename[100];
	char* token;
	int endtime_check;
	//파일이름이 인자값에 없으면 에러메세지 출력 후 종료
	if(argc < 1){
		fprintf(stderr,"Error 1 : 인자값이 잘못 입력되었습니다.\n");
		return;
	}
	//trash디렉토리 체크
	if(access("trash",F_OK) < 0){
		if(mkdir("./trash",0777) < 0){
			fprintf(stderr,"mkdir error for delete()\n");
			return;
		}
		mkdir("./trash/files",0777);
		mkdir("./trash/info",0777);
	}
	//check디렉토리로 작업디렉토리 이동
	if(chdir("check") < 0){
		fprintf(stderr, "Error 2 : 지정한 디렉토리가 존재하지 않습니다.\n");
		return;
	}
	//해당파일이 없으면 에러메세지 출력 후 종료
	if(access(argv[0], F_OK) < 0){
		fprintf(stderr,"Error 3 : 파일이 존재하지 않습니다.\n");
		chdir(saved_path);
		return;
	}
	//삭제할 파일경로 origin_path에 저장
	strcpy(origin_path,argv[0]);
	//파일이름 추출
	token = strtok(argv[0],"/");
	strcpy(filename,token);
	while((token = strtok(NULL,"/")) != NULL)
		strcpy(filename,token);
	//endtime,option이 모두 없을때
	if(argc < 2)
		delete_to_trash(trash_path,origin_path,filename);
	//endtime,옵션 처리
	else{
		//endtime이 있을경우
		if(argv[1][0] != '-'){
			//리턴값이 0이아니면 delete 수행
			if((endtime_check = delete_endtime(argc,argv))){
				//i옵션이 없는경우
				if(endtime_check == 1)
					delete_to_trash(trash_path,origin_path,filename);
				//i옵션이 있는경우
				else{
					sprintf(trash_path,"rm -r %s",origin_path);
					system(trash_path);
				}
				printf("삭제완료!\n");
			}
		}
		//i옵션일 경우
		else if(!strcmp(argv[1],"-i")){
			sprintf(trash_path,"rm -r %s",origin_path);
			system(trash_path);
		}
		//r옵션이 단독으로 사용될 경우에도 에러처리
		else if(!strcmp(argv[1],"-r"))
			fprintf(stderr,"Error 1 : 인자값이 잘못 입력 되었습니다.\n");
		else
			fprintf(stderr,"Error 1 : 인자값이 잘못 되었습니다.\n");
	}
	chdir(saved_path);
}
void delete_to_trash(char *t_path,char *o_path,char *fname)
{
	char info_path[BUFFER_SIZE];
	int index_num;
	struct stat statbuf;
	struct tm *tmbuf;
	time_t t;
	FILE* fp;

	//파일정보 stat버퍼에 저장
	if(stat(o_path,&statbuf) < 0){
		fprintf(stderr,"stat error for delete()\n");
		return;
	}
	sprintf(t_path,"%s/trash/files/%s",saved_path,fname);

	//trash파일에 중복된 파일이있는지 체크
	index_num = 0;
	while(access(t_path,F_OK) == 0){
		index_num++;
		sprintf(t_path,"%s/trash/files/%s_%d",saved_path,fname,index_num);
	}
	//파일 trash로 이동
	rename(o_path, t_path);
	//파일 info파일 생성
	if(index_num)
		sprintf(info_path, "%s/trash/info/%s_%d",saved_path,fname,index_num);
	else
		sprintf(info_path, "%s/trash/info/%s",saved_path,fname);

	//info파일 생성
	if((fp = fopen(info_path,"w")) == NULL){
		fprintf(stderr,"creat error for delete()\n");
		return;
	}
	//info파일 작성
	fprintf(fp,"[Trash info]\n");
	//절대경로가 아닌경우
	if(o_path[0] != '/'){
		if(o_path[0] == '.')
			fprintf(fp,"%s/check%s\n",saved_path,o_path+1);
		else
			fprintf(fp,"%s/check/%s\n",saved_path,o_path);
	}
	//절대경로인 경우
	else
		fprintf(fp,"%s\n",o_path);
	//현재 삭제시간 파일에 write
	time(&t);
	tmbuf = localtime(&t);
	fprintf(fp,"D : %04d-%02d-%02d %02d:%02d:%02d\n",tmbuf->tm_year+1900,
			tmbuf->tm_mon+1, tmbuf->tm_mday, tmbuf->tm_hour,
			tmbuf->tm_min, tmbuf->tm_sec);
	//mtime 파일에 write
	t = statbuf.st_mtime;
	tmbuf = localtime(&t);
	fprintf(fp,"M : %04d-%02d-%02d %02d:%02d:%02d\n",tmbuf->tm_year+1900,
			tmbuf->tm_mon+1, tmbuf->tm_mday, tmbuf->tm_hour,
			tmbuf->tm_min, tmbuf->tm_sec);
	fclose(fp);
	check_info_size();
}

int delete_endtime(int argc, char **argv)
{
	char* token;
	char yes_no[10];
	int exist_iOption = 0;
	struct tm tmbuf;
	time_t t;
	time_t now;
	//인자값이 파일명 날짜 시간 세개의 인자가 최소한 있어야함
	if(argc < 3){
		fprintf(stderr,"Error 1 : 인자값이 잘못 입력 되었습니다.\n");
		return 0;
	}
	//segmentation 오류 막기위함
	if(argv[1][0] < '0' || argv[1][0] > '9'){
		fprintf(stderr,"Error 1 : 인자값이 잘못 입력 되었습니다.\n");
		return 0;
	}
	token = strtok(argv[1],"-");
	tmbuf.tm_year = atoi(token) - 1900;
	token = strtok(NULL,"-");
	tmbuf.tm_mon= atoi(token) -1;
	token = strtok(NULL,"-");
	tmbuf.tm_mday = atoi(token);

	//segmentation오류에러를 막기위함
	if(argv[2][0] < '0' || argv[2][0] > '9'){
		fprintf(stderr,"Error 1 : 인자값이 잘못 입력 되었습니다.\n");
		return 0;
	}
	token = strtok(argv[2],":");
	tmbuf.tm_hour = atoi(token);
	token = strtok(NULL,":");
	tmbuf.tm_min = atoi(token);
	t = mktime(&tmbuf);
	time(&now);
	//과거시간을 입력하면 에러
	if(now > t){
		fprintf(stderr,"Error 4 : 이미 지난 시간에는 삭제할 수 없습니다.\n");
		return 0;
	}
	//인자값이 4개 이상이면
	if(argc > 3){
		//r옵션처리
		if(!strcmp(argv[3],"-r")){
			while(1){
				printf("Delete [y/n]?");
				fgets(yes_no,sizeof(yes_no),stdin);
				if(yes_no[0] == 'n' || yes_no[0] == 'N')
					return 0;
				else if(yes_no[0] == 'y' || yes_no[0] == 'Y')
					break;
			}
		}
		//-i옵션일 경우
		else if(!strcmp(argv[3],"-i"))
			exist_iOption = 1;
		//그 외에 경우 에러메세지 출력
		else{
			fprintf(stderr,"Error 1 : 인자값이 잘못 되었습니다.\n");
			return 0;
		}
	}
	time(&now);
	printf("삭제중입니다..잠시만 기다려주세요\n");
	//예약시간까지 프롬프트 대기
	while(now < t){
		sleep(1);
		time(&now);
	}
	//iOption이 있을경우 2리턴
	if(exist_iOption)
		return 2;
	else
		return 1;
}
void check_info_size()
{
	struct dirent **f_list;
	struct stat statbuf;
	time_t min_time = 0;		//가장오래된 파일의 수정시간
	long d_size = 0;
	long f_size;
	int count;
	int i;
	char buffer[BUFFER_SIZE];
	char fname[100];
	sprintf(buffer,"%s/trash/info",saved_path);
	if((count = scandir(buffer, &f_list, NULL, alphasort)) < 0){
		fprintf(stderr,"scandir error for check_info_size()\n");
		return;
	}
	//info디렉토리로 이동
	chdir(buffer);
	// . ..파일을 스킵하고 파일의 사이즈 측정하기 위해 2부터 시작
	for(i = 2; i < count; i++){
		if(stat(f_list[i]->d_name,&statbuf) < 0)
			printf("stat error for check_info_size()\n");
		d_size += statbuf.st_size;
	}
	while(d_size > 2000){
		//첫파일을 min값으로 지정
		stat(f_list[2]->d_name,&statbuf);
		min_time = statbuf.st_mtime;
		f_size = statbuf.st_size;
		strcpy(fname,f_list[2]->d_name);
		//가장오래된 파일 search
		for(i = 3; i < count; i++){
			stat(f_list[i]->d_name,&statbuf);
			if(statbuf.st_mtime < min_time){
				min_time = statbuf.st_mtime;
				f_size = statbuf.st_size;
				strcpy(fname,f_list[i]->d_name);
			}
		}
		//가장오래된파일삭제
		sprintf(buffer,"%s/trash/info/%s",saved_path,fname);
		if(remove(buffer) < 0){
			fprintf(stderr,"remove error for check_info_size()\n");
			//메모리 해제
			for(i = 0; i < count; i++)
				free(f_list[i]);
			free(f_list);
			return;
		}
		d_size -= f_size;
		//files에 있는 원본파일 삭제
		sprintf(buffer,"%s/trash/files/%s",saved_path,fname);
		if(remove(buffer) < 0){
			fprintf(stderr,"remove error for check_info_size()\n");
			//메모리 해제
			for(i = 0; i < count; i++)
				free(f_list[i]);
			free(f_list);
			return;
		}
	}
	//메모리 해제
	for(i = 0; i < count; i++)
		free(f_list[i]);
	free(f_list);
	chdir(saved_path);
}
void size(int argc, char** argv)
{
	char command[BUFFER_SIZE];
	char path[BUFFER_SIZE];

	//인자값 체크
	if(argc < 1){
		fprintf(stderr,"Error 1 : 인자값이 잘못 입력되었습니다.\n");
		return;
	}
	//상대경로 ./~형태로 변경
	if(argv[0][0] != '.')
		sprintf(path,"./%s",argv[0]);
	else
		strcpy(path,argv[0]);
	//옵션이 없을경우
	if(argc < 2)
		sprintf(command,"du %s --max-depth=0 -a | sort -k 2",path);
	
	else{
		if(!strcmp(argv[1],"-d") && argc > 2)
			sprintf(command,"du %s --max-depth=%s -a | sort -k 2",path,argv[2]);
		else{
			fprintf(stderr,"Error 1 : 인자값이 잘못 되었습니다.\n");
			return;
		}
	}
	system(command);
}
void recover(int argc, char ** argv)
{
	struct dirent **f_list;
	struct stat statbuf;
	struct tm *tm;
	struct trash_file_info same_names[100];
	char buffer[BUFFER_SIZE];
	char files_path[BUFFER_SIZE];
	char fname[100];
	FILE* fp;
	time_t t;
	char* token;
	int i,j,count;
	int temp;
	int overlap_num = 0;
	int index_num = 1;

	if(argc < 1){
		fprintf(stderr,"Error 1 : 인자값이 잘못 입력 되었습니다.\n");
		return;
	}

	sprintf(buffer,"%s/trash/info",saved_path);
	if((count = scandir(buffer, &f_list, NULL, alphasort)) < 0){
		fprintf(stderr, "Error 6 : trash 디렉토리가 존재하지 않습니다.\n");
		return;
	}

	chdir(buffer);
	//l옵션 수행
	if(argc > 1 && !strcmp(argv[1],"-l")){
		//same_names 재활용
		for(i = 2; i < count; i++){
			fp = fopen(f_list[i]->d_name,"r");
			fgets(buffer,BUFFER_SIZE,fp);
			fgets(buffer,BUFFER_SIZE,fp);
			buffer[strlen(buffer)-1] = '\0';
			//경로에서 파일이름 추출
			token = strtok(buffer,"/");
			strcpy(fname,token);
			while((token = strtok(NULL,"/")) != NULL)
				strcpy(fname,token);
			stat(f_list[i]->d_name,&statbuf);
			same_names[i-2].index = (int)statbuf.st_mtime;
			strcpy(same_names[i-2].fname, fname);
		}
		for(i = 0; i < count-2; i++){
			for(j = i+1; j < count-2; j++){
				if(same_names[i].index > same_names[j].index){
					temp = same_names[i].index;
					same_names[i].index = same_names[j].index;
					same_names[j].index = temp;
					strcpy(buffer,same_names[i].fname);
					strcpy(same_names[i].fname,same_names[j].fname);
					strcpy(same_names[j].fname,buffer);
				}
			}
			t = (time_t)same_names[i].index;
			tm = localtime(&t);
			printf("%d. %s\t\t %04d-%02d-%02d %02d:%02d:%02d\n",i+1,same_names[i].fname, tm->tm_year+1900,
					tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
		}
		printf("\n");
	}
	else if(argc > 1){
		fprintf(stderr,"Error 1 : 인자값이 잘못 되었습니다.\n");
		chdir(saved_path);
		return;
	}
	//해당하는 파일 탐색
	for(i = 2; i < count; i++){
		if(strstr(f_list[i]->d_name, argv[0]) == NULL)
			continue;
		fp = fopen(f_list[i]->d_name,"r");
		fgets(buffer,BUFFER_SIZE,fp);
		fgets(buffer,BUFFER_SIZE,fp);
		buffer[strlen(buffer)-1] = '\0';
		strcpy(same_names[overlap_num].fname,buffer);
		//경로에서 파일이름 추출
		token = strtok(buffer,"/");
		strcpy(fname,token);
		while((token = strtok(NULL,"/")) != NULL)
			strcpy(fname,token);
		//삭제하고자 하는 파일이름과 같으면 same_names에 추가
		if(!strcmp(argv[0],fname)){
			fgets(buffer,BUFFER_SIZE,fp);
			buffer[strlen(buffer)-1] = '\0';
			strcpy(same_names[overlap_num].d_time,buffer);
			fgets(buffer,BUFFER_SIZE,fp);
			buffer[strlen(buffer)-1] = '\0';
			strcpy(same_names[overlap_num].m_time,buffer);
			same_names[overlap_num].index = i;
			overlap_num++;
		}
		fclose(fp);
	}

	//중복되는 이름이 있으면
	if(overlap_num > 1){
		for(i = 0; i < overlap_num; i++)
			printf("%d. %s %s %s\n", i+1, argv[0], same_names[i].d_time, same_names[i].m_time);
		while(1){
			printf("Choose : ");
			fgets(buffer,BUFFER_SIZE,stdin);
			i = atoi(buffer);
			if(i > 0 && i < overlap_num+1){
				i--;
				break;
			}
		}
	}
	//이름이 단하나이면 i를 0으로
	else if(overlap_num == 1)
		i = 0;
	//해당이름이 없으면 종료
	else{
		printf("There is no '%s' in the 'trash' directory\n", argv[0]);
		chdir(saved_path);
		return;
	}
	//해당경로에 중복된 이름이 있는지 검사
	if(access(same_names[i].fname,F_OK) == 0){
		sprintf(buffer,"%d_%s",index_num, same_names[i].fname);
		while(access(buffer,F_OK) == 0){
			index_num++;
			sprintf(buffer,"%d_%s",index_num, same_names[i].fname);
		}
	}
	//중복되지않으면 이름복사
	else
		strcpy(buffer,same_names[i].fname);

	//원본파일 경로 files_path에 저장
	sprintf(files_path,"%s/trash/files/%s",saved_path,f_list[same_names[i].index]->d_name);
	//원본파일 이동
	rename(files_path,buffer);
	//info파일 삭제
	remove(f_list[same_names[i].index]->d_name);
	//메모리 해제
	for(i = 0; i < count; i++)
		free(f_list[i]);
	free(f_list);
	chdir(saved_path);
}
void tree()
{
	struct dirent **f_list, **f_list2;
	struct stat statbuf;
	int count;
	int i;
	printf("check\n");
	if((count = scandir("./check",&f_list,NULL,alphasort)) < 0){
		fprintf(stderr,"scandir error for tree()\n");
		return;
	}
	chdir("check");
	for(i = 2; i < count; i++){
		if(i == count - 1){
			printf("└───── %s\n",f_list[i]->d_name);
			stat(f_list[i]->d_name,&statbuf);
			if(S_ISDIR(statbuf.st_mode))
				search_tree(1,f_list[i]->d_name);
		}
		else{
			printf("├───── %s\n",f_list[i]->d_name);
			stat(f_list[i]->d_name,&statbuf);
			if(S_ISDIR(statbuf.st_mode)){
				printf("│");
				search_tree(1,f_list[i]->d_name);
			}
		}
	}
	//메모리 해제
	for(i = 0; i < count; i++)
		free(f_list[i]);
	free(f_list);
	chdir(saved_path);
}

void search_tree(int depth, char* d_name)
{
	struct dirent **f_list;
	struct stat statbuf;
	char buf[BUFFER_SIZE];
	int count;
	int i,j;

	if((count = scandir(d_name,&f_list,NULL,alphasort)) < 0){
		fprintf(stderr,"scandir error for tree()\n");
		return;
	}
	getcwd(buf,BUFFER_SIZE);
	chdir(d_name);
	for(i = 2; i < count; i++){
		if(i == count - 1){
			for(j = 0; j < depth; j++)
				printf("   ");
			printf("└───── %s\n",f_list[i]->d_name);
			stat(f_list[i]->d_name,&statbuf);
			if(S_ISDIR(statbuf.st_mode))
				search_tree(depth+1,f_list[i]->d_name);
		}
		else{
			for(j = 0; j < depth; j++)
				printf("   ");
			printf("├───── %s\n",f_list[i]->d_name);
			stat(f_list[i]->d_name,&statbuf);
			if(S_ISDIR(statbuf.st_mode))
				search_tree(depth+1,f_list[i]->d_name);
		}
	}
	//메모리 해제
	for(i = 0; i < count; i++)
		free(f_list[i]);
	free(f_list);
	chdir(buf);
}

void help()
{
	printf("\n<command-list>\n");
	printf("delete [FILENAME] [END_TIME] [OPTION <-i|-r>]\n");
	printf("size [FILENAME] [OPTION <-d>]\n");
	printf("recover [FILENAME] [OPTION <-l>]\n");
	printf("tree\n");
	printf("exit\n");
	printf("help\n\n");
}
