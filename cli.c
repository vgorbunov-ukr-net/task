/*
 * cli.c
 *
 *  Created on: Dec 24, 2020
 *      Author: viktor
 */

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/sem.h>
#include "common.h"




typedef union {
	unsigned int ipint;
	unsigned char ipchar[4];
} ipaddr_union;

/* structure for store IP addresses statistic */
typedef struct {
	ipaddr_union addr;
	unsigned int count;
} ipaddr_record;

void print_ipaddr_record(ipaddr_record* p)
{
	if (p) {
		  printf("\t IP : %u.%u.%u.%u Counter=%u\n",p->addr.ipchar[0],p->addr.ipchar[1],p->addr.ipchar[2],p->addr.ipchar[3],p->count);
	} else {
		printf("can not print not existed value\n");
	}
}

void print_help(void)
{
	printf("\n");
	printf("\t Supported commands:\n");
	printf("\t start (packets are being sniffed from now)\n");
	printf("\t stop (packets are not sniffed)\n");
	printf("\t show [010.000.002.013] (show number of packets received from 10.0.2.13)\n");
	printf("\t stat (show all collected statistic)\n");
	printf("\t exit (exit for daemon)\n");
	printf("\t exitcli (exit for cli)\n");
}





int main()
{
	/* shared memory initialization for sending strings with commands */
	int shmid_command;
	char *shmp_command;

	shmid_command = shmget(SHM_COM_KEY, 100, 0666);
	if (shmid_command == -1) {
		perror("shmid_command");
		return 1;
	}

	shmp_command = (char*) shmat(shmid_command, (void*) 0, 0);
	if (shmp_command == (void*) -1) {
		perror("shmp_command");
		return 1;
	}

	/* shared memory initialization for getting strings with data */
	int shmid_data;
	char *shmp_data;

	shmid_data = shmget(SHM_DAT_KEY, 100, 0666);
	if (shmid_data == -1) {
		perror("shmid_data");
		return 1;
	}

	shmp_data = (char*) shmat(shmid_data, (void*) 0, 0);
	if (shmp_data == (void*) -1) {
		perror("shmp_data");
		return 1;
	}
#ifdef SEMAPHORES
	int semid;
	union semun{
		int val;
		struct semid_ds *buf;
		unsigned short *array;
		struct seminfo *__buff;
	};
	union semun arg;
	struct sembuf lock_shmem_com = {0,-1,0};
	struct sembuf rel_shmem_com = {0,1,0};

    semid=semget(SEM_KEY,2,0666);

    /*set up in semaphore 0 value 1*/
    arg.val=1;
    semctl(semid,0,SETVAL,arg);
    semctl(semid,1,SETVAL,arg);
#endif
    const unsigned int size_com=50;
    char str[size_com];



while (!strstr(str, "exitcli"))
{
	/*
	 * Send commands
	 */


	char *command;
	command = shmp_command;
    /* modify shared memory via manipulation with pointer (command) and input string */
	fgets(str, size_com, stdin);

    if(strstr(str,"--help"))
	print_help();
#ifdef SEMAPHORES
    semop(semid, &lock_shmem_com,1);
#endif
	for(unsigned int i=0;i<size_com;i++)
		command[i]=str[i];
	int command_length = strlen(command);
     /* add 0 at the end of string */
	command += command_length;
	*command = 0;
#ifdef SEMAPHORES
	semop(semid, &rel_shmem_com,1);
#endif
	usleep(50000);//50ms delay - give some time for daemon to react on command

	while(1){

		usleep(50000);//50ms
#ifdef SEMAPHORES
		semop(semid, &lock_shmem_com,1);
#endif
		if(*shmp_command=='*'){
#ifdef SEMAPHORES
			semop(semid, &rel_shmem_com,1);
#endif
			break;
		}
#ifdef SEMAPHORES
		else
			semop(semid, &rel_shmem_com,1);
#endif
	}


///*without semaphores*/
//     /* wait for acknowledge '*' */
//	while(*shmp_command!='*'){
//		//printf("waiting for command..\n");
//		sleep(1);
//	}


	/*
	 * Read the Data from daemon
	 */

		char *str_data;
		str_data = shmp_data;
		if (*str_data != 0 && *str_data != '*' && *str_data != '!') {/* except special symbols */

			while (*str_data != '!') {/* wait for end of message */
				if (*str_data != '*')
					printf("\t %s\n", str_data);
				/* acknowledge: erase shared memory by '*' */
				if (*shmp_data != '!') {
					memset(shmp_data, 0, 50);
					*shmp_data = '*';
					shmp_data++;
					*shmp_data = 0;
					shmp_data--;
					usleep(300000);
				}
			}
		}
}
}




