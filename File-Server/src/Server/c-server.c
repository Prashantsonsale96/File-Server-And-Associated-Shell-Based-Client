#include<cstdio>
#include<cstdlib>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<errno.h>
#include<pthread.h>
#include<signal.h>
#include<iostream>
#include<queue>

using namespace std;

#define MAXCON 5
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t producer;
pthread_cond_t consumer;
pthread_t* worker_threads;        //thread pool.
int thread_ids[999];

int bytes_write=0;
int buffersize = 1024;
int num_worker_threads;
queue<int> q;

/*
 Signal handler to handle signal SIGINT.
*/
void sigproc(int sig)
{
	printf("\nTotal Bytes Sent: %d\n",bytes_write);
	//Cleanup
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&producer);
	pthread_cond_destroy(&consumer);
	free(worker_threads);
	exit(1);
}


void* function(void* thread_num)
{
	int count = 0,n=0,newfd;
	bool skip;
	char fname[256],buffer[buffersize];
	for(;;)
	{
		skip = false;
		pthread_mutex_lock(&mutex);
		if(q.size() == 0)
		{
			pthread_cond_wait(&consumer,&mutex);
		}
		newfd = q.front();
		q.pop();
		pthread_cond_signal(&producer);
		pthread_mutex_unlock(&mutex);

		bzero(fname,256);
		n=read(newfd,fname,sizeof(fname));
		if(n == -1)
		{
			fprintf(stderr,"ERROR: %s",strerror(errno));
			close(newfd);
			continue;
		}

		FILE *fp;
		printf("File name: %s\n",fname);
		fp = fopen(fname,"r");
		if(fp == NULL)
		{
			fprintf(stderr,"ERROR: %s",strerror(errno));
			close(newfd);
			continue;
		}

		char data;
		while((data = fgetc(fp)) != EOF)
		{
			if(count == buffersize)
			{
				count = 0;
				n=write(newfd,buffer,buffersize);
				bytes_write += n;
				bzero(buffer,buffersize);
				if(n<0)
				{
					skip = true;
					fprintf(stderr,"ERROR: %s",strerror(errno));
					break;
				}
			}
			buffer[count++] = data;
		}

		if(skip == true)
		{
			fclose(fp);
			close(newfd);
		}

		if(n != 0)
		{
			n=write(newfd,buffer,buffersize);
			bytes_write+=n;
			if(n < 0) skip=true;
			bzero(buffer,buffersize);
		}

		fclose(fp);
		close(newfd);
		fprintf(stdout,"Bytes Sent: %d",bytes_write);
	}
	pthread_exit(NULL);
}


int main(int argc, char* argv[])
{
	signal(SIGINT, sigproc);
	int sockfd,newfd,i,status;
	struct sockaddr_in serv,client;
	pthread_mutex_init(&mutex, NULL);
  	pthread_cond_init (&producer, NULL);
  	pthread_cond_init (&consumer, NULL);
    
	if(argc < 4)
    {
        fprintf(stderr,"Usage: %s port_no worker_thread queue_size\n",argv[0]);
        exit(0);
    }
	
	sockfd = socket(AF_INET,SOCK_STREAM,0);

	if(sockfd == -1)
	{
		fprintf(stderr,"ERROR: %s",strerror(errno));
		exit(1);
	}

	int port = atoi(argv[1]);
	bzero((char*)&serv,sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_port = htons(port);
	serv.sin_addr.s_addr = INADDR_ANY;

	if(bind(sockfd,(struct sockaddr*)&serv, sizeof(serv)) < 0)
	{
		fprintf(stderr,"ERROR: %s",strerror(errno));
		exit(1);
	}

	num_worker_threads = atoi(argv[2]);
	int request_size = atoi(argv[3]);
	worker_threads = (pthread_t*)malloc(num_worker_threads * sizeof(pthread_t));

	for(i=0;i<num_worker_threads;++i)
	{
		thread_ids[i] = i;
		status = pthread_create(&worker_threads[i],NULL,function,&thread_ids[i]);
		if(status < 0 )
		{
			fprintf(stderr,"ERROR: %s",strerror(errno));
			exit(1);
		}
	}

	if(listen(sockfd,MAXCON) == -1)
	{
		fprintf(stderr,"ERROR:%s",strerror(errno));
		exit(1);
	}

	socklen_t length = sizeof(client);
	for(;;)
	{

		if(request_size != 0)
		{
			pthread_mutex_lock(&mutex);
			while(q.size() >= request_size)
			{
				pthread_cond_wait(&producer,&mutex);
			}
			pthread_mutex_unlock(&mutex);
		}

		
		newfd=accept(sockfd,(struct sockaddr*)&client,&length);
		if(newfd == -1)
		{
			fprintf(stderr,"ERROR: %s",strerror(errno));
			exit(1);
		}

		pthread_mutex_lock(&mutex);
		q.push(newfd);
		pthread_cond_signal(&consumer);
		pthread_mutex_unlock(&mutex);

		printf("\nBytes sent: %d\n",bytes_write);
	}

	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&producer);
	pthread_cond_destroy(&consumer);
	free(worker_threads);	
}
