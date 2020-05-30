#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <dirent.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

struct file_info{
	char f_name[500];
	long f_size;
	struct file_info* next;
};

struct file_info* create_head = NULL;
struct file_info* delete_head = NULL;


struct dirent** old_list;
char dst_path[BUFFER_SIZE];
long tar_size;
int count;
int is_rOption = 0;
int is_tOption = 0;
int is_mOption = 0;
int is_file = 0;

void catch_signal(int signo);
void record_log(int argc, char* argv[]);
void sync_execute(char* src, char* dst);
void execute_rOption(char* src, char* dst, char* src_dir);
void execute_tOption(char* dst);
void sub_file_permission(char* dir_path);
void usage(void);

int main(int argc, char* argv[])
{
	struct sigaction sigact;
	struct stat statbuf;
	int i = 1;

	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigact.sa_handler = catch_signal;
	sigaction(SIGINT, &sigact, NULL);

	//인자 개수 확인
	if(argc < 3){
		usage();
		exit(1);
	}
	//옵션확인
	if(!strcmp(argv[1],"-r")){
		is_rOption = 1;
		i++;
	}
	else if(!strcmp(argv[1],"-t")){
		is_tOption = 1;
		i++;
	}
	else if(!strcmp(argv[1],"-m")){
		is_mOption = 1;
		i++;
	}
	//src파일 존재여부 확인
	if(access(argv[i],F_OK) != 0){
		usage();
		exit(1);
	}
	//src접근 권한확인
	if(access(argv[i],R_OK) != 0){
		usage();
		exit(1);
	}
	//파일정보 stat함수로 받아옴
	stat(argv[i], &statbuf);
	//디렉토리가 아니면 is_file 1로 설정
	if(!S_ISDIR(statbuf.st_mode))
		is_file = 1;
	else{
		//디렉토리의 경우 실행권한 확인
		if(access(argv[i],X_OK) != 0){
			usage();
			exit(1);
		}
	}
	//옵션값이 있을경우 4개이상, 없을경우 3개이상의 인덱스가 있어야함
	if(argc < i+2){
		usage();
		exit(1);
	}
	//dst파일 파일 존재여부 확인
	if(access(argv[i+1],F_OK) != 0){
		usage();
		exit(1);
	}
	stat(argv[i+1],&statbuf);
	//dst가 디렉토리가 아니면 종료
	if(!S_ISDIR(statbuf.st_mode)){
		usage();
		exit(1);
	}
	//실행권한 확인
	if(access(argv[i+1],X_OK) != 0){
		usage();
		exit(1);
	}
	//dst경로 전역변수로 저장
	memset(dst_path, 0, BUFFER_SIZE);
	strcpy(dst_path, argv[i+1]);
	if((count = scandir(argv[i+1], &old_list, NULL, alphasort)) < 0){
		fprintf(stderr,"scandir error in main()\n");
		exit(1);
	}

	//sync수행
	sync_execute(argv[i], argv[i+1]);
	//log 기록
	record_log(argc, argv);
	//메모리 해제
	for(i = 0; i < count; i++)
		free(old_list[i]);
	free(old_list);
	system("rm -r ssu_rsync_log_temp");
	system("rm -r backup_2484.tar");
	exit(0);
}
void sync_execute(char* src, char* dst)
{
	struct stat statbuf1, statbuf2;
	struct dirent **src_list, **dst_list;
	struct flock lock;
	struct file_info *new_file;
	char fname[BUFFER_SIZE];
	char buf[BUFFER_SIZE];
	char sub_path[BUFFER_SIZE];
	char* ptr;
	char backup;
	int i, j, fd;
	int src_count, dst_count;
	int jump_next = 0;

	//dst폴더 하위 모든 파일의 read권한 확인
	sub_file_permission(dst);
	//dst폴더로 이동
	getcwd(sub_path,BUFFER_SIZE);
	chdir(dst);
	//백업파일 생성
	sprintf(buf,"tar -cf backup_2484.tar *");
	system(buf);
	//백업파일 이동

	if(strstr(sub_path, " ") == NULL)
		sprintf(buf,"mv backup_2484.tar %s", sub_path);
	//경로에 빈칸이 있으면
	else
		sprintf(buf,"mv backup_2484.tar \"%s\"", sub_path);
	system(buf);
	chdir(sub_path);

	//src가 파일이면
	if(is_file){
		//이름저장
		strcpy(fname,src);
		//경로 /제거
		ptr = src;
		while(1){
			if(strstr(ptr,"/") == NULL)
				break;
			ptr = strstr(ptr,"/");
			ptr++;
			strcpy(fname,ptr);
		}
		//t옵션일 경우
		if(is_tOption){
			//다른 경로의 파일 일경우
			if(src != ptr){
				backup = *ptr;
				*ptr = '\0';
				//src 경로의 파일 src.tar로 압축하여 가지고옴
				getcwd(sub_path,BUFFER_SIZE);
				chdir(src);
				sprintf(buf,"tar -cf src.tar \"%s\"",fname);
				system(buf);
				sprintf(buf,"mv src.tar \"%s\"", sub_path);
				system(buf);
				chdir(sub_path);
				*ptr = backup;
			}
			//현재 디렉토리 내부 파일일 경우
			else{
				sprintf(buf,"tar -cf src.tar \"%s\"",fname);
				system(buf);
			}
			execute_tOption(dst);
			return;
		}
		//src파일 정보 statbuf1에 저장
		stat(src, &statbuf1);
		//========lock=========
		if((fd = open(src, O_RDONLY)) < 0){
			fprintf(stderr,"open error in execute_sync()\n");
			exit(1);
		}
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_whence = SEEK_SET;
		lock.l_len = 0;
		fcntl(fd, F_SETLK, &lock);
		//=====================
		for(i = 0; i < count; i++){
			if(!strcmp(old_list[i]->d_name, ".") || !strcmp(old_list[i]->d_name,".."))
				continue;
			sprintf(buf,"%s/%s",dst,old_list[i]->d_name);
			stat(buf, &statbuf2);
			//디렉토리의 같은이름의 파일이 있으면
			if(!strcmp(fname, old_list[i]->d_name)){
				//소스파일 이름과 목적디렉토리의 서브디렉토리 이름이 같을때
				if(S_ISDIR(statbuf2.st_mode)){
					//경로에 빈칸이 있는지 점검
					if(strstr(dst," ") == NULL && strstr(old_list[i]->d_name," ") == NULL)
						sprintf(buf,"rm -r %s/%s",dst, old_list[i]->d_name);
					else
						sprintf(buf,"rm -r \"%s/%s\"",dst, old_list[i]->d_name);
					//해당 디렉토리 제거
					system(buf);
					continue;
				}
				//size, 수정시간이 같으면
				if(statbuf1.st_size == statbuf2.st_size && statbuf1.st_mtime == statbuf2.st_mtime){
					//파일을 수정할 필요가 없으면 jump_next = 1
					jump_next = 1;
					continue;
				}
				//그 외에경우는 덮어써짐
			}
			//m옵션일 경우 삭제되는 파일(이름이 같지않은 경우)
			else if(is_mOption){
				new_file = (struct file_info*)malloc(sizeof(struct file_info));
				strcpy(new_file->f_name, old_list[i]->d_name);
				new_file->f_size = 0;
				new_file-> next = NULL;
				if(delete_head == NULL)
					delete_head = new_file;
				else{
					new_file->next = delete_head;
					delete_head = new_file;
				}
				//해당 파일 제거
				if(strstr(dst," ") == NULL && strstr(old_list[i]->d_name, " ") == NULL)
					sprintf(buf,"rm -r %s/%s",dst, old_list[i]->d_name);
				else
					sprintf(buf,"rm -r \"%s/%s\"",dst, old_list[i]->d_name);
				system(buf);
			}

		}
		//jump_next가 0이면 같은파일이 존재하지않으므로
		if(jump_next == 0){
			new_file = (struct file_info*)malloc(sizeof(struct file_info));
			strcpy(new_file->f_name, fname);
			new_file->f_size = statbuf1.st_size;
			new_file-> next = NULL;
			if(create_head == NULL)
				create_head = new_file;
			else{
				new_file->next = create_head;
			create_head = new_file;
			}
			//경로에 공백검사
			if(strstr(src," ") == NULL && strstr(dst," ") == NULL && strstr(fname, " ") == NULL)
				sprintf(buf,"cp -rp %s %s/%s", src,dst,fname);
			else
				sprintf(buf,"cp -rp \"%s\" \"%s/%s\"", src,dst,fname);
			//파일 복사
			system(buf);
		}
		//=========unlock==========
		lock.l_type = F_UNLCK;
		fcntl(fd, F_SETLK, &lock);
		close(fd);
		return;
	}
	//디렉토리의 경우
	else{
		//src디렉토리 스캔
		if((src_count = scandir(src,&src_list,NULL,alphasort)) < 0){
			fprintf(stderr,"scandir error in sync_execute()\n");
			exit(1);
		}
		if(is_tOption){
			getcwd(sub_path,BUFFER_SIZE);
			chdir(src);
			system("tar -cf src.tar *");
			sprintf(buf,"mv src.tar \"%s\"", sub_path);
			system(buf);
			chdir(sub_path);
			execute_tOption(dst);
			return;
		}
		//t 옵션이 아닌경우
		else{
			for(i = 0; i < src_count; i++){
				if(!strcmp(src_list[i]->d_name, ".") || !strcmp(src_list[i]->d_name, ".."))
					continue;
				sprintf(buf,"%s/%s",src,src_list[i]->d_name);
				stat(buf,&statbuf1);
				//src 내부 파일이 접근이 불가능하면
				if(access(buf, R_OK) != 0){
					fprintf(stderr,"Error : %s 파일에 대한 접근권한이 없습니다.\n",buf);
					//시그널 발생
					raise(SIGINT);
				}
				//디렉토리일 경우
				if(S_ISDIR(statbuf1.st_mode)){
					//r옵션일 경우
					if(is_rOption){
						//d_name sub_path에 복사
						memset(sub_path, 0, BUFFER_SIZE);
						strcpy(sub_path, src_list[i]->d_name);
						//r옵션 수행
						execute_rOption(src, dst, sub_path);
					}
					//아래는 파일동기화이므로 수행할 필요x
					continue;
				}
				//파일동기화 수행
				//========lock=========
				sprintf(buf,"%s/%s",src,src_list[i]->d_name);
				if((fd = open(buf, O_RDONLY)) < 0){
					fprintf(stderr,"open error in execute_sync()\n");
					exit(1);
				}
				lock.l_type = F_WRLCK;
				lock.l_start = 0;
				lock.l_whence = SEEK_SET;
				lock.l_len = 0;
				fcntl(fd, F_SETLK, &lock);
				//=====================
				//dst파일과 비교
				for(j = 0; j < count; j++){
					if(!strcmp(old_list[j]->d_name, ".") || !strcmp(old_list[j]->d_name,".."))
						continue;
					sprintf(buf,"%s/%s",dst,old_list[j]->d_name);
					stat(buf, &statbuf2);
					//디렉토리에 같은이름의 파일이 있으면
					if(!strcmp(src_list[i]->d_name, old_list[j]->d_name)){
						//소스파일 이름과 목적디렉토리의 서브디렉토리 이름이 같을때
						if(S_ISDIR(statbuf2.st_mode)){
							//해당 디렉토리 제거
							if(strstr(dst," ") == NULL && strstr(old_list[j]->d_name," ") == NULL)
								sprintf(buf,"rm -r %s/%s",dst, old_list[j]->d_name);
							else
								sprintf(buf,"rm -r \"%s/%s\"",dst, old_list[j]->d_name);
							system(buf);
							//디렉토리의 경우 m옵션을 밑에서 한번에 처리하므로 break
						}
						//size, 수정시간이 같으면
						if(statbuf1.st_size == statbuf2.st_size && statbuf1.st_mtime == statbuf2.st_mtime){
							//파일을 수정할 필요가 없으면 jump_next = 1
							jump_next = 1;
						}
						//m옵션이 밑에서 처리되므로 반복문 break
						break;
					}
				}
				//같은파일인 경우 jump
				if(jump_next){
					jump_next = 0;
					continue;
				}
				//같은 파일이 존재하지않으므로
				new_file = (struct file_info*)malloc(sizeof(struct file_info));
				strcpy(new_file->f_name, src_list[i]->d_name);
				new_file->f_size = statbuf1.st_size;
				new_file-> next = NULL;
				if(create_head == NULL)
					create_head = new_file;
				else{
					new_file->next = create_head;
					create_head = new_file;
				}
				if(strstr(src," ") == NULL && strstr(dst," ") == NULL && strstr(src_list[i]->d_name," ") == NULL)
					sprintf(buf,"cp -rp %s/%s %s/%s", src,src_list[i]->d_name,dst,src_list[i]->d_name);
				else
					sprintf(buf,"cp -rp \"%s/%s\" \"%s/%s\"", src,src_list[i]->d_name,dst,src_list[i]->d_name);
				//파일 복사
				system(buf);
				//=========unlock==========
				lock.l_type = F_UNLCK;
				fcntl(fd, F_SETLK, &lock);
				close(fd);
			}
			//m옵션처리
			if(is_mOption){
				if((dst_count = scandir(dst,&dst_list,NULL,alphasort)) < 0){
					fprintf(stderr,"scandir error in sync_execute()\n");
					exit(1);
				}
				for(j = 0; j < dst_count; j++){
					if(!strcmp(dst_list[j]->d_name, ".") || !strcmp(dst_list[j]->d_name, ".."))
						continue;
					for(i = 2; i < src_count; i++){
						//dst파일이 src 디렉토리에 있는 이름이면
						if(!strcmp(dst_list[j]->d_name, src_list[i]->d_name))
							break;
						//src에 없는 파일이름이면
						if(i+1 == src_count){
							new_file = (struct file_info*)malloc(sizeof(struct file_info));
							strcpy(new_file->f_name, dst_list[j]->d_name);
							new_file->f_size = 0;
							new_file-> next = NULL;
							if(delete_head == NULL)
								delete_head = new_file;
							else{
								new_file->next = delete_head;
								delete_head = new_file;
							}
							//해당 파일 제거
							if(strstr(dst," ") == NULL && strstr(dst_list[j]->d_name," ") == NULL)
								sprintf(buf,"rm -r %s/%s",dst, dst_list[j]->d_name);
							else
								sprintf(buf,"rm -r \"%s/%s\"",dst, dst_list[j]->d_name);
							system(buf);
						}
					}
				}
				//메모리 해제
				for(i = 0; i < dst_count; i++)
					free(dst_list[i]);
				free(dst_list);
			}
		}
		//메모리 해제
		for(i = 0; i < src_count; i++)
			free(src_list[i]);
		free(src_list);
	}
}
void record_log(int argc, char* argv[])
{
	struct file_info* cur;
	time_t t;
	FILE* fp, *fp2;
	char* fname = "ssu_rsync_log";
	char* ptr;
	char buf[BUFFER_SIZE];
	int i;

	if(access(fname, F_OK) != 0)
		fp = fopen(fname, "w+");
	else{
		if((fp = fopen(fname, "a+")) == NULL){
			fprintf(stderr, "fopen error in record_log()\n");
			exit(1);
		}
	}
	system("cp ssu_rsync_log ssu_rsync_log_temp");
	time(&t);
	ptr = ctime(&t);
	//개행 제거
	ptr[strlen(ptr)-1] = '\0';
	fprintf(fp,"[%s] ",ptr);
	for(i = 0; i < argc; i++)
		fprintf(fp,"%s ",argv[i]);
	fprintf(fp,"\n");
	//t옵션일 경우
	if(is_tOption){
		//totalSize 출력
		fprintf(fp,"\ttotalSize %ldbytes\n",tar_size);
		//tar로그 오픈
		fp2 = fopen("tar_log.txt", "r+");
		//파일끝까지 읽음
		while(!feof(fp2)){
			memset(buf,0,BUFFER_SIZE);
			fgets(buf,BUFFER_SIZE,fp2);
			if(strlen(buf) != 0)
				fprintf(fp,"\t%s",buf);
		}
		fclose(fp2);
		system("rm -r tar_log.txt");
	}
	else{
		cur = create_head;
		while(cur != NULL){
			fprintf(fp,"\t%s %ldbytes\n",cur->f_name, cur->f_size);
			cur = cur->next;
		}
		cur = delete_head;
		while(cur != NULL){
			fprintf(fp,"\t%s delete\n",cur->f_name);
			cur = cur->next;
		}
	}
	fclose(fp);
}
void execute_rOption(char* src, char* dst, char* src_dir)
{
	struct dirent** f_list;
	struct stat statbuf1, statbuf2;
	struct file_info* new_file;
	char buf[BUFFER_SIZE];
	char* ptr;
	int i,count;

	sprintf(buf,"%s/%s",dst,src_dir);
	//dst에 서브디렉토리가 없으면 생성
	if(access(buf, F_OK) != 0){
		//경로에 공백있는지 검사
		if(strstr(dst," ") == NULL && strstr(src_dir," ") == NULL)
			sprintf(buf,"mkdir %s/%s", dst, src_dir);
		else
			sprintf(buf,"mkdir \"%s/%s\"", dst, src_dir);
		system(buf);
	}
	//서브디렉토리 주소 buf에 저장
	sprintf(buf,"%s/%s",src,src_dir);
	//서브디렉토리 스캔
	if((count = scandir(buf,&f_list,NULL,alphasort)) < 0){
		fprintf(stderr,"scandir error in execute_rOption()\n");
		exit(1);
	}
	ptr = src_dir + strlen(src_dir);
	*ptr++ = '/';
	*ptr = '\0';
	for(i = 0; i < count; i++){
		//서브디렉토리의 파일들의 정보를 읽음
		if(!strcmp(f_list[i]->d_name, ".") || !strcmp(f_list[i]->d_name, ".."))
			continue;
		sprintf(buf,"%s/%s%s",src, src_dir, f_list[i]->d_name);
		if(stat(buf, &statbuf1) < 0){
			fprintf(stderr,"stat error in execute_rOption()\n");
			exit(1);
		}
		//src파일의 read접근권한확인 필수
		if(access(buf,R_OK) != 0){
			fprintf(stderr,"Error : %s 파일에 대한 접근권한이 없습니다.\n",buf);
			//파일복구
			raise(SIGINT);
		}
		//서브디렉토리의 파일이 디렉토리이면
		if(S_ISDIR(statbuf1.st_mode)){
			//재귀로 디렉토리 탐색
			strcpy(ptr, f_list[i]->d_name);
			execute_rOption(src, dst, src_dir);
			*ptr = '\0';
			continue;
		}
		//dst디렉토리에 해당하는파일이 존재하는지 판단
		sprintf(buf, "%s/%s%s",dst,src_dir,f_list[i]->d_name);
		if(access(buf, F_OK) == 0){
			stat(buf, &statbuf2);
			//기본파일이고
			if(!S_ISDIR(statbuf2.st_mode)){
				//수정시간과 사이즈가 같으면
				if((statbuf2.st_mtime == statbuf1.st_mtime)&&(statbuf2.st_size == statbuf1.st_size))
					//동기화x
					continue;
			}
			else{
				//중복되는 디렉토리 삭제
				if(strstr(dst, " ") == NULL && strstr(src_dir," ") == NULL && strstr(f_list[i]->d_name," ") == NULL)
					sprintf(buf,"rm -r %s/%s%s", dst, src_dir, f_list[i]->d_name);
				else
					sprintf(buf,"rm -r \"%s/%s%s\"", dst, src_dir, f_list[i]->d_name);
				system(buf);
			}
		}
		//파일이면 create파일 리스트에 추가
		new_file = (struct file_info*)malloc(sizeof(struct file_info));
		strcpy(ptr, f_list[i]->d_name);
		//생성파일리스트에 추가
		strcpy(new_file->f_name, src_dir);
		new_file->f_size = statbuf1.st_size;
		new_file-> next = NULL;
		if(create_head == NULL)
			create_head = new_file;
		else{
			new_file->next = create_head;
			create_head = new_file;
		}
		//파일 동기화

		//모든 경로에 공백이 없으면
		if(strstr(src_dir," ") == NULL && strstr(src," ") == NULL && strstr(dst, " ") == NULL)
			sprintf(buf, "cp -rp %s/%s %s/%s",src,src_dir, dst,src_dir);
		//그외에 경우
		else
			sprintf(buf, "cp -rp \"%s/%s\" \"%s/%s\"",src,src_dir, dst,src_dir);
		system(buf);
		//ptr다시 널문자로 만듬
		*ptr = '\0';
	}
	for(i = 0; i < count; i++)
		free(f_list[i]);
	free(f_list);
}

void sub_file_permission(char* dir_path)
{
	struct dirent** f_list;
	struct stat statbuf;
	char buf[BUFFER_SIZE];
	int count, i;

	//해당 dir 스캔
	if((count = scandir(dir_path, &f_list, NULL, alphasort)) < 0){
		fprintf(stderr,"scandir error in rOption_check_permission()\n");
		exit(1);
	}
	//재귀로 탐색
	for(i = 0; i < count; i++){
		if(!strcmp(f_list[i]->d_name, ".") || !strcmp(f_list[i]->d_name, ".."))
			continue;
		sprintf(buf,"%s/%s",dir_path, f_list[i]->d_name);
		//dir내부 파일 read권한 확인
		if(access(buf, R_OK) != 0){
			fprintf(stderr,"Error : %s 파일에 대한 접근권한이 없습니다.\n",buf);
			raise(SIGINT);
		}
		stat(buf, &statbuf);
		//디렉토리이면 재귀로 확인
		if(S_ISDIR(statbuf.st_mode))
			sub_file_permission(buf);
	}
	//메모리 해제
	for(i = 0; i < count; i++)
		free(f_list[i]);
	free(f_list);
}
void execute_tOption(char* dst)
{
	struct stat statbuf;
	char buf[BUFFER_SIZE];

	stat("src.tar",&statbuf);
	tar_size = statbuf.st_size;

	sprintf(buf,"tar -xvf src.tar -C \"%s\" >tar_log.txt", dst);
	system(buf);
	//tar파일 삭제
	sprintf(buf,"rm -r src.tar");
	system(buf);
}

void catch_signal(int signo)
{
	char buf[BUFFER_SIZE];
	
	//동기화중이던 dst디렉토리 삭제

	if(strstr(dst_path," ") == NULL)
		sprintf(buf,"rm -r %s", dst_path);
	else
		sprintf(buf,"rm -r \"%s\"", dst_path);
	system(buf);
	//dst디렉토리 생성
	if(strstr(dst_path," ") == NULL)
		sprintf(buf,"mkdir %s", dst_path);
	else
		sprintf(buf,"mkdir \"%s\"", dst_path);
	system(buf);
	//backup.tar 해당폴더 압축해제
	if(strstr(dst_path," ") == NULL)
		sprintf(buf,"tar -xf backup_2484.tar -C %s", dst_path);
	else
		sprintf(buf,"tar -xf backup_2484.tar -C \"%s\"", dst_path);
	system(buf);
	//backup.tar 파일삭제
	system("rm -r backup_2484.tar");
	//로그파일 롤백
	rename("ssu_rsync_log_temp", "ssu_rsync_log");
	exit(0);
}
void usage()
{
	printf("=========================== usage ===========================\nssu_rsync [OPTION] <SRC> <DST>\n");
	printf("[OPTION]\n -r : Files within src's subdirectory are also synchronized\n -t : Bundle and synchronize targets at once.\n -m : Synchronize so that dst src is exactly the same.\n<SRC>\n - source file or directory.\n<DST>\n - destnation directory.\n");
	printf("=============================================================\n");
}
