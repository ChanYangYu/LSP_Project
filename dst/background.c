#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024
void main(void)
{
	struct dirent **old, **new;
	struct stat statbuf;
	struct tm *tmbuf;
	time_t t;
	char buf[BUFFER_SIZE];
	FILE* fp;
	int mtime_list[BUFFER_SIZE];
	int count1,count2;
	int i,j;

	if(access("log.txt",F_OK) == 0)
		fp = fopen("log.txt","a+");
	else{
		if((fp = fopen("log.txt","w")) == NULL){
			fprintf(stderr,"fopen error for background()\n");
			return;
		}
	}
	
	getcwd(buf,BUFFER_SIZE);
	strcat(buf,"/check");
	chdir(buf);
	if((count1 = scandir(buf,&old,NULL,alphasort)) < 0){
		fprintf(stderr,"scandir error for background()\n");
		return;
	}
	
	for(i = 2; i < count1;i++){
		stat(old[i]->d_name,&statbuf);
		mtime_list[i-2] = (int)statbuf.st_mtime;
	}
	
	while(1)
	{
		sleep(1);
		if((count2 = scandir(buf,&new,NULL,alphasort)) < 0){
			fprintf(stderr,"scandir error for background()\n");
			return;
		}
		i = 2;
		j = 2;
		while(i < count1 && j < count2){
			if(strstr(old[i]->d_name,".swp") != NULL){
				i++;
				continue;
			}
			if(strstr(new[j]->d_name,".swp") != NULL){
				j++;
				continue;
			}

			if(strcmp(old[i]->d_name,new[j]->d_name) == 0){
				stat(new[j]->d_name,&statbuf);
				if(mtime_list[i-2] != ((int)statbuf.st_mtime)){
					time(&t);
					tmbuf = localtime(&t);
					fprintf(fp,"[%04d-%02d-%02d %02d:%02d:%02d][modify_%s]\n"
							,tmbuf->tm_year+1900,tmbuf->tm_mon+1, tmbuf->tm_mday
							,tmbuf->tm_hour,tmbuf->tm_min, tmbuf->tm_sec,old[i]->d_name);
				}
				j++;
				i++;
			}
			else if(strcmp(old[i]->d_name,new[j]->d_name) < 0){
				time(&t);
				tmbuf = localtime(&t);
				fprintf(fp,"[%04d-%02d-%02d %02d:%02d:%02d][delete_%s]\n"
						,tmbuf->tm_year+1900,tmbuf->tm_mon+1, tmbuf->tm_mday
						,tmbuf->tm_hour,tmbuf->tm_min, tmbuf->tm_sec,old[i]->d_name);
				i++;
			}
			else{
				time(&t);
				tmbuf = localtime(&t);
				fprintf(fp,"[%04d-%02d-%02d %02d:%02d:%02d][create_%s]\n"
						,tmbuf->tm_year+1900,tmbuf->tm_mon+1, tmbuf->tm_mday
						,tmbuf->tm_hour,tmbuf->tm_min, tmbuf->tm_sec,new[j]->d_name);
				j++;
			}
		}
		while(i < count1){
			if(strstr(old[i]->d_name,".swp") != NULL){
				i++;
				continue;
			}
			time(&t);
			tmbuf = localtime(&t);
			fprintf(fp,"[%04d-%02d-%02d %02d:%02d:%02d][delete_%s]\n"
					,tmbuf->tm_year+1900,tmbuf->tm_mon+1, tmbuf->tm_mday
					,tmbuf->tm_hour,tmbuf->tm_min, tmbuf->tm_sec,old[i]->d_name);
			i++;
		}
		while(j < count2){
			if(strstr(new[j]->d_name,".swp") != NULL){
				j++;
				continue;
			}
			time(&t);
			tmbuf = localtime(&t);
			fprintf(fp,"[%04d-%02d-%02d %02d:%02d:%02d][create_%s]\n"
					,tmbuf->tm_year+1900,tmbuf->tm_mon+1, tmbuf->tm_mday
					,tmbuf->tm_hour,tmbuf->tm_min, tmbuf->tm_sec,new[j]->d_name);
			j++;
		}
		for(j = 2; j < count2;j++){
			stat(new[j]->d_name,&statbuf);
			mtime_list[j-2] = (int)statbuf.st_mtime;
		}
		old = new;
		count1 = count2;
		fflush(fp);
	}
	fclose(fp);
}
