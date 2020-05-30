all : ssu_crontab ssu_crond ssu_rsync

CC = gcc

ssu_crontab : ssu_crontab.c 
	$(CC) ssu_crontab.c -o ssu_crontab

ssu_crond : ssu_crond.c
	$(CC) ssu_crond.c -o ssu_crond

ssu_rsync : ssu_rsync.c
	$(CC) ssu_rsync.c -o ssu_rsync

clean :
	rm *.o
	rm ssu_rsync
	rm ssu_crond
	rm ssu_crontab
