/*This is an advanced version of the priority semaphores implemented for many number of priority
classes, the number can be specified by fixing NUM_OF_PRIO_CLASSES to the desired value*/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<pthread.h>
#include<semaphore.h>
#include<errno.h>
#include<string.h>
#include<time.h>
#define NUM_OF_PRIO_CLASSES 10

int K, r_count=0;
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

sem_t *
enqueue_sorted(List_Elem *head, int prio){
	List_Elem *p, *new_elem;
	p = head;
	while(p->next != NULL && p->next->priority > prio){
		p = p->next;
	}	
	new_elem = (List_Elem *)malloc(sizeof(List_Elem));
	new_elem->sem = (sem_t *)malloc(sizeof(sem_t));
	sem_init(new_elem->sem, 0, 0);
	new_elem->priority = prio;
	
	new_elem->next = p->next;
	p->next = new_elem;

	return new_elem->sem;		
}
sem_t * 
dequeue_sorted(List_Elem *head){
	List_Elem *p;
	sem_t *s;
	p = head->next;
	s = head->next->sem;
	head->next = head->next->next;	/*rearrange list*/
	free(p);
	return s;
}

void 
sem_prioritywait(Sem *s, int prio){	
	sem_t *new_sem;
	sem_wait(s->ME);
	if(--s->count < 0){
		new_sem = enqueue_sorted(s->sem_list, prio);
		sem_post(s->ME);
		sem_wait(new_sem);
		sem_destroy(new_sem);
	} else{ sem_post(s->ME); }	
}

void 
sem_prioritypost(Sem *s){
	sem_t *sem;
	sem_wait(s->ME);
	if(++s->count <=0){
		sem = dequeue_sorted(s->sem_list);
		sem_post(sem);
	} 
	sem_post(s->ME);
}

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
	sem_prioritywait(semPrio, data->priority);	
	data->N = 1;
	write(fd[1], data, sizeof(threads_t));
	pthread_exit(NULL);
}
/*************************************************************************************************************/
/*************************************************************************************************************/

void 
print_list(){
	List_Elem *p;
	p = semPrio->sem_list;
	int size=0;
	while(p->next != NULL && size<= sizeof(semPrio->sem_list)){
		printf("<--%d-->", p->next->priority);
		size++;
		p = p->next;
	}
	printf("\n");
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
	semPrio->sem_list->next = NULL;
	/*pipe semaphore*/
	S1 = (sem_t *)malloc(sizeof(sem_t));
	sem_init(S1, 0, 0);
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
		th_data[i].priority = ((rand() % NUM_OF_PRIO_CLASSES) + 1); /*priority classes C1-C10*/
		th_data[i].N = 0;
		pthread_create(&th[i], NULL, thread_func, (void *)&th_data[i]);		
		sleep(1);
	}
	//sleep(5);
	for(i = 0; i<K; i++)
		pipe_to_file();
	printf("++++++++++SORTED PRIORITY LINKED LIST++++++++++++++\n");
	print_list();
	printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
	
	for(i = 0; i<K; i++){
		sem_prioritypost(semPrio);
		pipe_to_file();
	}
	
	for(i = 0;i < K; i++)
		pthread_join(th[i], NULL);
	
}
