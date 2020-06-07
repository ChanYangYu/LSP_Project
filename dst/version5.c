#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>

int main(void)
{
	struct timeval start, end;
	int N, k, x = 1;
	int count = 0;
	int limit;
	int *p;

	//N값 입력받음
	printf("Input N : ");
	scanf("%d",&N);

	p = (int*)malloc(sizeof(N*sizeof(int)));

	//프로그램 시작시간 측정
	gettimeofday(&start,NULL);
	//N개의 소수를 출력할 때 까지 반복
	printf("2 ");
	p[0] = 2;
	limit = 0;
	while(count < N-1){
		x = x + 2;
		//limit값 설정
		if((int)sqrt((float)x) >= p[limit]){
			limit++;
		}
		//limit까지 반복
		for(k = 0; k <= limit; k++){
			if(x % p[k] == 0)
				break;
		}
		//나누어지는 수가없으면 소수이므로
		if(k > limit){
			printf("%d ", x);
			//개수 증가
			p[count++] = x;
		}
	}
	//종료시간 측정
	gettimeofday(&end,NULL);
	
	//end.usec이 더 작으면 end.sec을 usec으로 변환하여 뺄셈을 해줌
	if((end.tv_usec - start.tv_usec) < 0){
		end.tv_sec--;
		printf("\nExecution time : %ld(sec) %ld(usec)\n",
				end.tv_sec - start.tv_sec,
				end.tv_usec - start.tv_usec + 1000000);
	}
	//이외에 경우에는 end - start값을 출력
	else{
		printf("\nExecution time : %ld(sec) %ld(usec)\n",
				end.tv_sec - start.tv_sec,
				end.tv_usec - start.tv_usec);
	}
	exit(1);
}
