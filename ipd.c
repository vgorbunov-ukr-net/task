/*
 * ipd.c
 *
 *  Created on: Dec 24, 2020
 *      Author: viktor
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <sys/types.h>
#include <ifaddrs.h>

#include <netdb.h>
#include <linux/if_link.h>

#include <netinet/ether.h>
#include <netinet/ip.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <string.h>
#include <errno.h>
#include <string.h>


#define SHM_COM_KEY 0x1234
#define SHM_DAT_KEY 0x4321

#define NOP   0
#define START 1
#define STOP  2
#define SHOW  3
#define STAT  4
#define EXIT  5


typedef union {
	uint32_t ipint;
	unsigned char ipchar[4];
} ipaddr_union;


/*
 * structure for store IP addresses statistic
 */
typedef struct {
	ipaddr_union addr;
	unsigned long long count;/* ToDo:Workaround log base 10 */
} ipaddr_record;


void print_ipaddr_record(ipaddr_record* p)
{
	if (p) {
		  printf("\t IP : %u.%u.%u.%u  Count= %llu\n",p->addr.ipchar[0],p->addr.ipchar[1],p->addr.ipchar[2],p->addr.ipchar[3],p->count);
	} else {
		printf("can not print not existed value\n");
	}
}

void send_ipaddr_record(ipaddr_record* p, char *str_to_send)
{
	if (p) {
		  sprintf(str_to_send,"\t IP : %u.%u.%u.%u  Count= %llu\n",p->addr.ipchar[0],p->addr.ipchar[1],p->addr.ipchar[2],p->addr.ipchar[3],p->count);
	} else {
		printf("can not print not existed value\n");
	}
}


/*
 * create IP record, take pointer on first record, return pointer on created record
 */
ipaddr_record* make_ipaddr_record(ipaddr_record *p_base, uint32_t ip, uint32_t c, uint32_t counter_of_records) {
	ipaddr_record *p_created;
	p_created = p_base + counter_of_records;
	p_created->addr.ipint = ip;
	p_created->count = c;
	return p_created;/* p_base is pointer on array[1000] allocated my malloc, therefore returned pointer\object is not local*/
}


/*
 * find IP address among IP records and return pointer on it, in case not found NULL is returned
 */
ipaddr_record* find_ipaddr_record(uint32_t ip, const uint32_t size, ipaddr_record *p_base) {
	ipaddr_record *p_found = NULL;
	ipaddr_record *p_temp;
	p_temp = p_base;

	for (uint32_t i = 0; i < size; i++) {
		if (p_temp->addr.ipint == ip) {
			p_found = p_temp;
		}
		p_temp++;
	}
	return p_found;
}


typedef struct {
	unsigned int Command;
	unsigned int IP;
}com;

/* constructor*/
com *command_make()
{
  com *cmd = malloc(sizeof(com));
  cmd->Command=NOP;
  cmd->IP=0u;
  return cmd;
}

/*
 * convert command string to command structure with integer IP
 */
com *command_decoder(char *str_com, com *cmd) {

	if (strstr(str_com, "start")) {
		cmd->Command = START;
		cmd->IP = 0;
	} else if (strstr(str_com, "exit")){
		cmd->Command = EXIT;
		cmd->IP = 0;
	}else if (strstr(str_com, "stop")) {
		cmd->Command = STOP;
		cmd->IP = 0;
	} else if (strstr(str_com, "stat")) {
		cmd->Command = STAT;
		cmd->IP = 0;
	} else if (strstr(str_com, "show")) {
		cmd->Command = SHOW;
		char *c = strchr(str_com, '[');
		c++;
		char ip[3 * 4 + 3] = { 0 }; //{xxx.xxx.xxx.xxx}
		unsigned int i = 0;
		while (i < (3 * 4 + 3)) {
			ip[i] = *c;
			i++;
			c++;
		}
		/* in ip: 100.111.122.133 */
		char oct_0[4];
		oct_0[0] = ip[0];
		oct_0[1] = ip[1];
		oct_0[2] = ip[2];
		oct_0[3] = 0; //100
		char oct_1[4];
		oct_1[0] = ip[4];
		oct_1[1] = ip[5];
		oct_1[2] = ip[6];
		oct_1[3] = 0; //111
		char oct_2[4];
		oct_2[0] = ip[8];
		oct_2[1] = ip[9];
		oct_2[2] = ip[10];
		oct_2[3] = 0; //122
		char oct_3[4];
		oct_3[0] = ip[12];
		oct_3[1] = ip[13];
		oct_3[2] = ip[14];
		oct_3[3] = 0; //133

		ipaddr_union temp;
		temp.ipchar[0] = atoi(oct_0); //conversion from uint to uchar
		temp.ipchar[1] = atoi(oct_1);
		temp.ipchar[2] = atoi(oct_2);
		temp.ipchar[3] = atoi(oct_3);
		cmd->IP = temp.ipint;
		//printf("\t IP=%u\n",cmd->IP);
	}
	return cmd;
}

void command_free(com *a) {
	free(a);
}




int main(int argc, char** argv)
{
    /* Our process ID and Session ID */
    pid_t pid, sid;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
            exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then
       we can exit the parent process. */
    if (pid > 0) {
            exit(EXIT_SUCCESS);
    }

    /* Change the file mode mask */
    umask(0);

    /* Open any logs here */

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
            /* Log the failure */
            exit(EXIT_FAILURE);
    }

    /* Change the current working directory */
    if ((chdir("/")) < 0) {
            /* Log the failure */
            exit(EXIT_FAILURE);
    }

    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);




	char *fileName;
	unsigned int strBuffLength = 14;//4 bytes + ',' + 8bytes + '\n'
	char strBuff[strBuffLength];

	uint32_t size = 1000; //let us gues that we will have not more than 1000 IP addresses between two reboots
	ipaddr_record *p_base = calloc(size, sizeof(ipaddr_record));
	ipaddr_record *p_next;p_next = p_base;
	uint32_t counter_of_records = 0;

	if(argv[1]==NULL) {fileName="/home/ipd.csv";}//file is not defined from console

    /*
     * read IP data from .csv file stored between reboots
     *    IP,count
     *    IP,count
     *    ...
     */


	FILE *logFile = fopen(fileName,"r");
	if (logFile == NULL) {
		printf("File with IP data does not exist\n");
	} else {
		while (fgets(strBuff, strBuffLength, logFile)) {
			char *ip = strtok(strBuff, ",");
			char *count = strtok(NULL, "\n");
			p_next = make_ipaddr_record(p_base, atoi(ip), atoll(count), counter_of_records);
			counter_of_records++;
			p_next++;
		}
	fclose(logFile);
	}




	int sock;
	sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)); //ETH_P_ALL// address family packet, raw low level socket, each ip packet
	if (sock < 0) {
		printf("\terror during the socket creation\n");
		perror("sock");
		return -1;
	}

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 200000;
	int status = 0;

	/* SO_RCVTIMEO -Receive time out*/
	status = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (struct timeval*) &timeout, sizeof(struct timeval));
	//status = setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, interface, IF_NAMESIZE);/*try to bind socket to interface name,result is the same*/
	if (status == -1) {
		perror("setsockopt");
	}

	/* shared memory initialization in daemon for receiving commands */
	int shmid_command;
	char *shmp_command;
	char *str_command;

	shmid_command = shmget(SHM_COM_KEY, 100, 0666 | IPC_CREAT);
	if (shmid_command == -1) {
		perror("shmid_command");
		return 1;
	}

	shmp_command = (char*) shmat(shmid_command, (void*) 0, 0);
	if (shmp_command == (void*) -1) {
		perror("shmp_command");
		return 1;
	}

	/* shared memory initialization for sending data */
	int shmid_data;
	char *shmp_data;

	shmid_data = shmget(SHM_DAT_KEY, 100, 0666 | IPC_CREAT);
	if (shmid_data == -1) {
		perror("shmid_data");
		return 1;
	}

	shmp_data = (char*) shmat(shmid_data, (void*) 0, 0);
	if (shmp_data == (void*) -1) {
		perror("shmp_data");
		return 1;
	}

	unsigned int flag_to_sniff = 0u;

	com *command_to_exec = command_make(); //to store and execute received command

	unsigned char *buffer = (unsigned char*) malloc(2000); //to receive data, biggest IP packet is 65535 bytes

	/*
	 *  main loop
	 */


	while (command_to_exec->Command != EXIT) //exit in case of "exit" command
	{

		memset(buffer, 0, 2000);
		struct sockaddr saddr;
		int saddr_len = sizeof(saddr);
		int buflen;

		/* Receive a network packet and copy in to buffer */
		buflen = recvfrom(sock, buffer, 2000, 0, &saddr, (socklen_t*) &saddr_len);
//	if (buflen < 0) {
//		perror("recvfrom");
//		return -1;//do not to exit in case if we do not get any packet
//	}

			struct iphdr *ip = (struct iphdr*) (buffer + sizeof(struct ethhdr));/* move forward from Ethernet header to IP header*/


	/* Execute received and handled command */
			if (flag_to_sniff) {
				ipaddr_record *p_found;
				p_found = find_ipaddr_record(ip->daddr, size, p_base);/* find IP record in available IP data collection */
				if (p_found == NULL)/* IP is not found among IP records, add the IP record with Count=1 */
				{
					p_next = make_ipaddr_record(p_base, ip->daddr, 1, counter_of_records);
					counter_of_records++;
					p_next++;
				} else {/* update IP record with increased Count*/
					p_found->count += 1;
				}
			}

	/*
	* decode the command received from shared memory
	*/
		if (shmp_command[0] != '*' && shmp_command[0] != 0) {/*if command is not yet read, not erased by '*' and not empty*/
			str_command = shmp_command;
			command_to_exec = command_decoder(str_command, command_to_exec);
			//erase shared memory by '*'
			memset(shmp_command, 0, 50);
			*shmp_command = '*';
			shmp_command++;
			*shmp_command = 0;
			shmp_command--;
		}



	/*
	 * handle decoded commands
	 */
		char *str_to_send;
		ipaddr_record *p_found;

		switch (command_to_exec->Command) {
		case START:
			flag_to_sniff = 1;
			break;
		case STOP:
			flag_to_sniff = 0;
			break;
		case STAT:
			p_next = p_base;
			str_to_send = shmp_data;
			for (uint32_t i = 0; i < counter_of_records; i++) {
				send_ipaddr_record(p_next, str_to_send);
				/* wait for acknowledge */
				while (*shmp_data != '*') {
					usleep(100000);
				}
				p_next++;
			}
			*str_to_send = '!';/* end of sequence */
			command_to_exec->Command = 0;
			break;
		case SHOW:
			p_found = find_ipaddr_record(command_to_exec->IP, size, p_base);
			str_to_send = shmp_data;
			send_ipaddr_record(p_found, str_to_send);
			/* wait for acknowledge */
			while (*shmp_data != '*') {
				usleep(100000);
			}
			*str_to_send = '!'; /* end of sequence */
			command_to_exec->Command = 0;
			command_to_exec->IP = 0;
			break;
		default:
			break;
		}
	}


/* cleaning */

if (shmdt(shmp_command) == -1) {
	perror("Detaching Shared Memory CMD");
	return 1;
}
if (shmctl(shmid_command, IPC_RMID, 0) == -1) {
	perror("Removing Shared Memory CMD");
	return 1;
}
if (shmdt(shmp_data) == -1) {
	perror("Detaching Shared Memory Data");
	return 1;
}
if (shmctl(shmid_data, IPC_RMID, 0) == -1) {
	perror("Removing Shared Memory Data");
	return 1;
}



  /*
   * store IP data in .csv file
   * IP,count
   * IP,count
   * ....
   */
	FILE *logFileWrite = fopen(fileName, "w");
	if (logFileWrite == NULL) {
		perror("Can not create the file to store data between reboots");
	} else {
		p_next = p_base;
		for (uint32_t i = 0; i < counter_of_records; i++) {
			fprintf(logFileWrite, "%u,%llu\n", p_next->addr.ipint,
					p_next->count);
			p_next++;
		}
		fclose(logFileWrite);
	}


close(sock);
command_free(command_to_exec);
free(buffer);
free(p_base);

return EXIT_SUCCESS;
}
