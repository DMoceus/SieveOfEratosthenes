#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>

const int MAX_WORKERS = 32;
const int MAX_SIZE = 100000000;
int workers = 0;
int size = 0;
bool threadBool = NULL;
bool processBool = NULL;
int something;
sem_t semaphore;
pthread_mutex_t mutex;
int* intArray;

void* magic(void* bullshit);


int main(int argc, char** argv)
{
	while((something = getopt(argc, argv,"uw:s:pt")) != -1)
	{
		switch(something)
		{
			case 'u':
				fprintf(stderr,"./sieve [-u] [-w <num-workers>] [-s <sieve-size>] [-{pt}]\n");
				exit(0);
				break;
				
			case 'w':
				if(atoi(optarg)>MAX_WORKERS)
				{
					fprintf(stderr,"Workers must be at maximum %d, you entered %d\n",MAX_WORKERS,atoi(optarg));
					exit(1);
				}
				else
				{
					workers = atoi(optarg);
				}
				break;
				
			case 's':
				if(atoi(optarg)>MAX_SIZE)
				{
					fprintf(stderr,"Sieve Size must be at maximum %d, you entered %d\n",MAX_SIZE,atoi(optarg));
					exit(1);
				}
				size = atoi(optarg);
				break;
				
			case 'p':
				if(threadBool)
				{
					fprintf(stderr,"Cannot declare both -p and -t, choose one.\n");
					exit(1);
				}
				processBool = true;
				break;
				
			case 't':
				if(processBool)
				{
					fprintf(stderr,"Cannot declare both -p and -t, choose one.\n");
					exit(1);
				}
				threadBool = true;
				break;
				
			default:
				fprintf(stderr,"./sieve [-u] [-w <num-workers>] [-s <sieve-size] [-{pt}]\n");
				exit(0);
				break;
		}
	}
	if(!threadBool && !processBool)
	{
		fprintf(stderr,"Please choose an operation mode: -p for processes, -t for threads.\n");
		exit(1);
	}
	if(!workers || !size)
	{
		fprintf(stderr,"Either workers or size were not specified, please specify both.\n");
		exit(1);
	}
//*********************Multiple Threads********************
	if(threadBool)
	{
		startTiming();
		int i,x;
		intArray = malloc(size*sizeof(int));
		pthread_mutex_init(&mutex,NULL);
		pthread_t pthreads[workers];
		
		intArray[0]=2;
		intArray[1]=1;
		for(i=2;i<size;i++)
		{
			intArray[i]=0;
		}
//~~~~~~~~~~~~~~~~~~~~~Thread Creation / Management~~~~~~~~~
		for(i=0;i<workers;i++)
		{
			if(pthread_create(&pthreads[i],NULL,magic,(void*)(i+1)) != 0)
			{
				perror("pthread_create");
				exit(1);
			}
		}
		for(i=0;i<workers;i++)
		{
			if(pthread_join(pthreads[i],NULL) != 0)
			{
				perror("pthread_join");
				exit(1);
			}
		}
//~~~~~~~~~~~~~~~~~~~~~Output~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		int primes=0;
		for(i=1;i<(size);i++)
		{
			if(intArray[i]==0)
			{
				primes++;
			}
		}
		fprintf(stdout,"Primes less than %d: %d\n",size,primes);
		primes=0;
		for(i=(size-1);((primes<10) && (i>0));i--)
		{
			if(intArray[i]==0)
			{
				fprintf(stdout,"%d\n",i);
				primes++;
			}
		}
		for(i=0;i<workers;i++)
		{
			primes=0;
			for(x=1;x<size;x++)
			{
				if(intArray[x]==(i+1))
				{
					primes++;
				}
			}
			fprintf(stdout,"Worker %d was the last to knock out %d values\n",i+1,primes);
		}
		stopTiming();
		secondTime();
	}
	
	
	
	
//*********************Multiple Processes******************
	if(processBool)
	{
		startTiming();
//~~~~~~~~~~~~~~~~~~~~~Shared Memory~~~~~~~~~~~~~~~~~~~~~~~
		int i,k,shm_fd,rval;
		int j=0;
		int* shared_mem;
		pid_t pid;
		sem_init(&semaphore,1,1);
		if((shm_fd = shm_open("/dmorris",O_CREAT | O_RDWR, 0666)) == -1)
		{
			perror("shm_open");
			exit(1);
		}
		if((rval=shm_unlink("/dmorris"))==-1)
		{
			perror("shm_unlink");
			exit(1);
		}
		if(ftruncate(shm_fd,(size)*sizeof(int))==-1)
		{
			perror("ftruncate");
			exit(1);
		}
		shared_mem = mmap(NULL,(size)*sizeof(int),PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd,0);
		if(shared_mem == MAP_FAILED)
		{
			perror("mmap");
			exit(1);
		}
		if(close(shm_fd)==-1)
		{
			perror("close");
			exit(1);
		}
		shared_mem[0]=2;
		shared_mem[1]=1;
		for(i=2;i<size;i++)
		{
			shared_mem[i]=0;
		}
//~~~~~~~~~~~~~~~~~~~~~Forking~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		for(i=0;i<workers;i++)
		{
			pid=fork();
			if(pid==-1)
			{
				perror("fork");
				exit(1);
			}
//~~~~~~~~~~~~~~~~~~~~~Child~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			if(pid==0)
			{
				//CRITICAL SECTION START
				sem_wait(&semaphore);
					j=shared_mem[0];
					while(shared_mem[j]!=0)
					{
						shared_mem[0]++;
						j=shared_mem[0];
						if(j==size-2)
						{
							sem_post(&semaphore);
							exit(0);
						}
					}
					shared_mem[0]++;
				sem_post(&semaphore);
				//CRITICAL SECTION END
				while(true)
				{
					for(k=(j+1);k<size;k++)
					{
						if(k%j==0)
						{
							shared_mem[k]=i+1;
						}
					}
					//CRITICAL SECTION START
					sem_wait(&semaphore);
						j=shared_mem[0];
						while(shared_mem[j]!=0)
						{
							shared_mem[0]++;
							j=shared_mem[0];
							if(j==size-2)
							{
								sem_post(&semaphore);
								exit(0);
							}
						}
						shared_mem[0]++;
					sem_post(&semaphore);
					//CRITICAL SECTION END
				}
			}
		}
//~~~~~~~~~~~~~~~~~~~~~Parent~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~		
		for(i=0;i<workers;i++)
		{
			if(wait(NULL)==-1)
			{
				perror("wait");
				exit(1);
			}
		}
		
		int primes=0;
		for(i=1;i<(size);i++)
		{
			if(shared_mem[i]==0)
			{
				primes++;
			}
		}
		fprintf(stdout,"Primes less than %d: %d\n",size,primes);
		primes=0;
		for(i=(size-1);((primes<10) && (i>0));i--)
		{
			if(shared_mem[i]==0)
			{
				fprintf(stdout,"%d\n",i);
				primes++;
			}
		}
		for(i=0;i<workers;i++)
		{
			primes=0;
			for(j=1;j<size;j++)
			{
				if(shared_mem[j]==(i+1))
				{
					primes++;
				}
			}
			fprintf(stdout,"Worker %d was the last to knock out %d values\n",i+1,primes);
		}
		if(munmap(shared_mem, (size)*sizeof(int))==-1)
		{
			perror("munmap");
			exit(1);
		}
		stopTiming();
		secondTime();
		//printf("Exited Properly\n");
	}
	return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~Threaded Function~~~~~~~~~~~~~~~~~~~~~~~~~~

void* magic(void* bullshit)
{
	int j,k;
	//CRITICAL SECTION START
	pthread_mutex_lock(&mutex);
		j=intArray[0];
		if(j==size)
		{
			pthread_mutex_unlock(&mutex);
			pthread_exit(0);
		}
		while(intArray[j]!=0)
		{
			intArray[0]++;
			j=intArray[0];
			if(j==(size-1))
			{
				pthread_mutex_unlock(&mutex);
				pthread_exit(0);
			}
			if(j==size)
			{
				pthread_mutex_unlock(&mutex);
				pthread_exit(0);
			}
		}
		intArray[0]++;
	pthread_mutex_unlock(&mutex);
	//CRITICAL SECTION END
	while(true)
	{
		for(k=(j+1);k<size;k++)
		{
			if(k%j==0)
			{
				intArray[k]=((int)bullshit);
			}
		}
		//CRITICAL SECTION START
		pthread_mutex_lock(&mutex);
			j=intArray[0];
			if(j==size)
			{
				pthread_mutex_unlock(&mutex);
				pthread_exit(0);
			}
			while(intArray[j]!=0)
			{
				intArray[0]++;
				j=intArray[0];
				if(j==(size-2))
				{
					pthread_mutex_unlock(&mutex);
					pthread_exit(0);
				}
				if(j==size)
				{
					pthread_mutex_unlock(&mutex);
					pthread_exit(0);
				}
			}
			intArray[0]++;
		pthread_mutex_unlock(&mutex);
		//CRITICAL SECTION END
	}
}





















