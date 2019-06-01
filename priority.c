#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<semaphore.h>
#include<errno.h>
#include<string.h>
#include<time.h>


int K;
char output_file[20];

typedef struct thread{
	int id;
	int priority;
	int N;
}threads_t;

typedef struct List_Elem{
	sem_t *sem;
	int priority;
	struct List_Elem *next;
}List_Elem;

typedef struct Sem{
	sem_t *ME;
	int count;
	struct List_Elem *sem_list;
}Sem;

threads_t *th_data;
Sem *semPrio;
sem_t *S1;

FILE *fp;
int fd[2];

sem_t *enqueue_sorted(List_Elem *head, int prio){
	List_Elem *p, *new_elem;
	p = head;
	while(prio > p->next->priority)
		p = p->next;
	new_elem = (List_Elem *)malloc(sizeof(List_Elem));
	new_elem->sem = (sem_t *)malloc(sizeof(sem_t));
	sem_init(new_elem->sem, 0, 0);
	new_elem->priority = prio;
	new_elem->next = p->next;
	p->next = new_elem;
	return new_elem->sem;		
}

void sem_prioritywait(Sem *s, int prio){	
	sem_t *new_sem;
	sem_wait(s->ME);
	
	if(--s->count < 0){
		new_sem = enqueue_sorted(s->sem_list, prio);
		sem_post(s->ME);
		sem_wait(new_sem);
		sem_destroy(new_sem);
	} else{ sem_post(s->ME); }	
}



/**********************************THREAD START ROUTINE********************************************************/
/*************************************************************************************************************/
static void *
thread_func(void *arg){
	threads_t *data = (threads_t *)arg;
	printf("Thread %d priority: %d\n",data->id, data->priority);
	
	while(1){
		sleep(2);

		int res = write(fd[1], data, sizeof(threads_t));
		if(res != 1){ perror("pipe write(): "); }
		sem_post(S1);
		/*requesting to access CR*/
		sem_prioritywait(semPrio, data->priority);	
		printf("I am inside bitches am not letting u in\n");
		
		
	}
}

void 
init(int value){
	th_data = (threads_t *)malloc(K * sizeof(threads_t));
	
	/*priority semaphore*/
	semPrio = (Sem *)malloc(sizeof(Sem));
	semPrio->count = value;
	semPrio->ME = (sem_t *)malloc(sizeof(sem_t));
	sem_init(semPrio->ME, 0, 1);
	
	semPrio->sem_list = (List_Elem *)malloc(sizeof(List_Elem));
	semPrio->sem_list->priority = -1;
	semPrio->sem_list->next = (List_Elem *)malloc(sizeof(List_Elem));
	semPrio->sem_list->next->priority = 1000000;
	semPrio->sem_list->next->next = NULL;
	
	
	/*pipe semaphore*/
	S1 = (sem_t *)malloc(sizeof(sem_t));
	sem_init(S1, 0, 0);
	int res = pipe(fd);
	if(res < 0){
		perror("pipe:");
		exit(1);
	}
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
		th_data[i].id = i;
		th_data[i].priority = ((rand() % 3) + 1);
		th_data[i].N = 0;
		pthread_create(&th[i], NULL, thread_func, (void *)&th_data[i]);		
	}
	
	int j=0;
	while(1){
		sleep(2);
		sem_wait(S1);
		threads_t thread_data;
		int res = read(fd[0], &thread_data, sizeof(threads_t));
		if(res != 1){ perror("pipe read(): "); }
		//printf("MAIN: thread %d priority %d Status: %d\n", thread_data.id, thread_data.priority, thread_data.N);
		if(j==0) { fp = fopen(output_file, "w"); }
		else{ fp = fopen(output_file, "a"); }
		fprintf(fp, "Thread %d:  ", thread_data.id);
		fprintf(fp, "priority %d:  ", thread_data.priority);
		if(!thread_data.N){ fprintf(fp, "status: %s\n", "I am requesting to access critical region"); }
		else{ fprintf(fp, "status: %s\n", "I am inside critical region"); }
		j++;
		fclose(fp);
	}

	for(i=0; i<K; i++){
		pthread_join(th[i], NULL);
	}
	
	return 0;
}
