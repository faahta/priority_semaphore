/*This is a simplified version of the priority semaphores implemented only for three priority classes*
C1 C2 and C3, refer to priority.c file to apply more number of priority classes*/
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<semaphore.h>
#include<errno.h>
#include<string.h>
#include<time.h>


int K, r_count=0;
char output_file[20];

typedef struct thread{
	int id;
	int priority;
	int N;
}threads_t;


threads_t *th_data;
sem_t *S1, *S2, *S3;
sem_t *S1;

FILE *fp;
int fd[2];

/**********************************THREAD START ROUTINE********************************************************/
/*************************************************************************************************************/
static void *
thread_func(void *arg){
	threads_t *data = (threads_t *)arg;
	sleep(4);
	printf("Thread %d priority: %d\n",data->id, data->priority);
	/*requesting to access CR*/
	int res = write(fd[1], data, sizeof(threads_t));
	if(res != 1){ perror("pipe write(): "); }
	
	if(data->priority == 3)
		sem_wait(S3);
	if(data->priority == 2)
		sem_wait(S2);
	if(data->priority == 1)
		sem_wait(S1);	
		
	data->N = 1;
	write(fd[1], data, sizeof(threads_t));
	pthread_exit(NULL);
}
/*************************************************************************************************************/
/*************************************************************************************************************/


void 
init(int value){
	th_data = (threads_t *)malloc(K * sizeof(threads_t));

	/*pipe semaphore*/
	S1 = (sem_t *)malloc(sizeof(sem_t));
	sem_init(S1, 0, 0);
	S2 = (sem_t *)malloc(sizeof(sem_t));
	sem_init(S2, 0, 0);
	S3 = (sem_t *)malloc(sizeof(sem_t));
	sem_init(S3, 0, 0);
	
	int res = pipe(fd);
	if(res < 0){
		perror("pipe:");
		exit(1);
	}
}

void pipe_to_file(){
	r_count++;
	threads_t thread_data;
	int res = read(fd[0], &thread_data, sizeof(threads_t));
	if(res != 1){ perror("pipe read(): "); }
	if(r_count==0) { fp = fopen(output_file, "w"); }
	else{ fp = fopen(output_file, "a"); }
	fprintf(fp, "Thread %d:  ", thread_data.id);
	fprintf(fp, "priority C%d:  ", thread_data.priority);
	if(!thread_data.N){ fprintf(fp, "status: %s\n", "I am requesting to access critical region"); }
	else{ fprintf(fp, "status: %s\n", "I am inside critical region"); }
	fclose(fp);
}

int 
main(int argc, char *argv[]){
	
	if(argc!=3){
		printf("usage: %s K ouput_file\n",argv[0]);
		exit(1);
	}
	K = atoi(argv[1]);
	strcpy(output_file, argv[2]);
	
	pthread_t *th;
	th = (pthread_t *)malloc(K * sizeof(pthread_t));
	init(0);

	int i;
	
	for(i = 0;i < K; i++){
		srand(time(NULL));
		th_data[i].id = i;
		th_data[i].priority = ((rand() % 10) + 1); /*priority classes C1-C10*/
		th_data[i].N = 0;
		pthread_create(&th[i], NULL, thread_func, (void *)&th_data[i]);		
		sleep(1);
	}
	//sleep(5);
	for(i = 0; i<K; i++)
		pipe_to_file();
	
	for(i = 0; i<K; i++){
		sem_post(S3);
		sem_post(S2);
		sem_post(S1);
		pipe_to_file();
	}
	
	
	for(i = 0;i < K; i++)
		pthread_join(th[i], NULL);
	
}
