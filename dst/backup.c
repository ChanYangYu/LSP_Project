#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "person.h"
//전역변수
#define PAGE_PER_MAX_RECORD (PAGE_SIZE / RECORD_SIZE)
char record_filename[1024];
struct header{
    int page_num;
    int record_num;
    int del_page;
    int del_record;
};

void readPage(FILE *fp, char *pagebuf, int pagenum)
{
    if(fseek(fp,(long)pagenum*(long)PAGE_SIZE,SEEK_SET) != 0){ 
        fprintf(stderr,"Error 3 : readPage() error\n");
        exit(1);
    }   
    fread(pagebuf,PAGE_SIZE,1,fp);
}

void writePage(FILE *fp, const char *pagebuf, int pagenum)
{
    if(fseek(fp, (long)pagenum*(long)PAGE_SIZE,SEEK_SET) != 0){ 
        fprintf(stderr,"Error 4 : writePage() error\n");
        exit(1);
    }   
    if(fwrite(pagebuf,PAGE_SIZE,1,fp) != 1){ 
        fprintf(stderr,"Error 5 : fwrite fail\n");
        exit(1);
    }
}

void pack(char *recordbuf, const Person *p)
{
    char* ptr;
    long length;
    strncpy(recordbuf, p->sn, strlen(p->sn));
    strcat(recordbuf,"#");
    strncat(recordbuf,p->name, strlen(p->name));
    strcat(recordbuf,"#");
    strncat(recordbuf,p->age, strlen(p->age));
    strcat(recordbuf,"#");
    strncat(recordbuf,p->addr, strlen(p->addr));
    strcat(recordbuf,"#");
    strncat(recordbuf,p->phone, strlen(p->phone));
    strcat(recordbuf,"#");
    strncat(recordbuf,p->email, strlen(p->email));
    strcat(recordbuf,"#");
    //나머지 0xff로 채움
    ptr = recordbuf;
    length = strlen(recordbuf);
    printf("length : %ld\n", length);
    ptr += length;
    memset(ptr, 46, (RECORD_SIZE-length));

    //레코드 마지막에 #추가
    recordbuf[RECORD_SIZE-1] = '#';

    //debug
    printf("recordbuf : %s\n",recordbuf);

}

void unpack(const char *recordbuf, Person *p)
{

}

void insert(FILE *fp, const Person *p)
{
    struct header head,cur;
    struct stat statbuf;
    char record_buf[RECORD_SIZE];
    char page_buf[PAGE_SIZE];
    char *ptr;
    int page_num;
    int record_num;

    //headpage가 없을경우
    stat(record_filename,&statbuf);
    if(statbuf.st_size == 0){
        //head페이지 생성
        head.page_num = 2;
        head.record_num = 1;
        head.del_page = -1;
        head.del_record = -1;
        memset(page_buf,46,PAGE_SIZE);
        memcpy(page_buf,(char*)&head,sizeof(struct header));
        writePage(fp,page_buf,0);
        //insert할 데이터 페이지 생성
        pack(record_buf, p);
        memset(page_buf,46,PAGE_SIZE);
        strncpy(page_buf,record_buf, RECORD_SIZE);
        writePage(fp,page_buf,1);
        return;
    }
    readPage(fp,page_buf, 0);
    //debug
    memcpy((char*)&head,page_buf,sizeof(head));
    printf("page_num : %d, record_num : %d, del_page : %d, del_record : %d\n",head.page_num,
            head.record_num, head.del_page, head.del_record);

    //삭제된 레코드가 없으면
    if(head.del_page == -1 && head.del_record == -1){
        page_num = head.record_num / PAGE_PER_MAX_RECORD + 1;
        printf("page_num : %d\n",page_num);
        //쓸페이지가 현재 있는 총 페이지(header가 있으므로 -1해줘야함)보다 크면
        if(page_num > head.page_num-1){
            //새로운 페이지 할당
            pack(record_buf, p);
            memset(page_buf,46,PAGE_SIZE);
            strncpy(page_buf,record_buf, RECORD_SIZE);
            writePage(fp,page_buf,page_num);
            //header업데이트
            head.page_num++;
            head.record_num++;
            memset(page_buf,46,PAGE_SIZE);
            memcpy(page_buf,(char*)&head,sizeof(struct header));
            writePage(fp,page_buf,0);
            return;
        }
        else{
            readPage(fp,page_buf,page_num);
            //레코드의 이어붙일 자리
            record_num = head.record_num % PAGE_PER_MAX_RECORD;
            ptr = page_buf;
            ptr += record_num*RECORD_SIZE;
            pack(record_buf, p);
            memcpy(ptr,record_buf, RECORD_SIZE);
            writePage(fp,page_buf,page_num);
            //header업데이트
            head.record_num++;
            memset(page_buf,46,PAGE_SIZE);
            memcpy(page_buf,(char*)&head,sizeof(struct header));
            writePage(fp,page_buf,0);
            return;
        }
    }
}

void delete(FILE *fp, const char *sn)
{

}

int main(int argc, char *argv[])
{
    FILE *fp;
    Person person;


    if(argc < 3){
        fprintf(stderr,"Error 1 : 인자값이 잘못 입력되었습니다.\n");
        exit(1);
    }
   if((fp = fopen(argv[2], "r+")) == NULL){
        fprintf(stderr,"Error 0 : 레코드 파일 오픈에 실패하였습니다.\n");
        exit(1);
    }
    //레코드파일 이름 저장
    strcpy(record_filename, argv[2]);
    //i옵션
    if(!strcmp(argv[1],"i")){
        if(argc < 8){
            fprintf(stderr,"Error 2 : i옵션에 해당하는 필드값이 부족합니다.\n");
            exit(1);
        }
        //Person 구조체값 초기화
        strcpy(person.sn, argv[3]);
        strcpy(person.name, argv[4]);
        strcpy(person.age, argv[5]);
        strcpy(person.addr, argv[6]);
        strcpy(person.phone, argv[7]);
        strcpy(person.email, argv[8]);
        //insert호출
        insert(fp, &person);
    }


    return 1;
}




