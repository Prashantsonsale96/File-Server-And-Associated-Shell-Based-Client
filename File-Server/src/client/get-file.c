#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<errno.h>
#include<signal.h>

#define filebuffsize 1024

int bytes_read = 0;

void sigproc(int sig)
{
	printf("Received signal SIGINT: Downloaded %d bytes so far.",bytes_read);
	exit(1);
}

int main(int argc, char* argv[])
{
	signal(SIGINT,sigproc);
	int sockfd,n=0;
	char filebuffer[filebuffsize];
	struct sockaddr_in addr;
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd == -1)
	{
		printf("ERROR in socket \n");
		fprintf(stderr,"ERROR: %s",strerror(errno));
		exit(0);
	}

	int portno = atoi(argv[3]);
	bzero((char*)&addr,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(portno);
	addr.sin_addr.s_addr = inet_addr(argv[2]);

	char buffer[265];
	bzero(buffer,256);
	strcpy(buffer,argv[1]);
	//printf("fileName Received: %s  length : %ld\n",buffer,strlen(buffer));

	if(connect(sockfd,(struct sockaddr*)&addr,sizeof(addr)) < 0)
	{
		fprintf(stderr,"ERROR: %s",strerror(errno));
		exit(1);
	}
	//printf("Connetec to server\n");
	n=write(sockfd,buffer,strlen(buffer));
	if(n < 0)
	{
		fprintf(stderr,"ERROR: %s",strerror(errno));
	}
	bzero(buffer,sizeof(buffer));

	while((n = read(sockfd,filebuffer,filebuffsize-1)) > 0)
	{
		if(argv[4][0] == 'd')
			printf("%s",filebuffer);
		bytes_read += n;
		bzero(filebuffer,filebuffsize);
	}

	fprintf(stdout,"Bytes Received: %d\n",bytes_read);
	
	return 0;

}
