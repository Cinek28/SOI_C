#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>


//Struktura przechowująca wyprodukowany obiekt:
typedef struct
{
    char Element; //dane do bufora
    pid_t whoReads; //zmienna wskazująca kto ostatnio czytał z bufora- jeżeli nikt to 0
} product;

const int CAPACITY = 100; //rozmiar bufora
const int MIN_BUFOR = 3; //minimalna liczba elementów w buforze

//Ile elementów mają wykonać producenci:
const int NOOFELEMENTS = 100;

//Który element się wykonuje:
int* Counter;

//Kolekcja FIFO:

product* bufor;
int* Count;

//Wskaźniki na wartości pid procesów producentów i konsumentów:
pid_t* producers;
pid_t* consumers;

//Semafory:

sem_t *full, *empty, *mutex, *status_flag_b, *status_flag_produce, *status_flag_consume, *status_flag_minimum, *status_flag_read;

//Funkcje potrzebne w programie:
//Tworzenie producenta A- tworzy jeden element:
void createProducerA();
//Producent B - tworzy 2 elementy:
void createProducerB();
//Konsumenci:
void createConsumers();
//Funkcja produkująca element:
void produceA();
//Funkcja produkująca 2 elementy:
void produceB();
//Funkcja konsumenta:
void consume();
//Funkcje bufora:
void push_back(product temp)
{

    bufor[*Count] = temp;
    printf("Pushed %d element: %c. Process: %d.\n",*Count+1, bufor[*Count].Element,getpid());
    *Count += 1;
};

void pop_first()
{
    printf("Popped %d element: %c. Process: %d.\n",*Count, bufor[*Count-1].Element,getpid());
    bufor[*Count-1].Element = 'E';
    bufor[*Count-1].whoReads = 0;
    --(*Count);
};

int main()
{

    //Pamięć dzielona dla wszystkich procesów:

    bufor = mmap(NULL, sizeof(bufor)*(CAPACITY+1), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    consumers = mmap(NULL, sizeof(pid_t)*2, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    producers = mmap(NULL, sizeof(pid_t)*2, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    empty = mmap(NULL, sizeof(sem_t), PROT_READ |PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1, 0);
    full = mmap(NULL, sizeof(sem_t), PROT_READ |PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1, 0);
    mutex = mmap(NULL, sizeof(sem_t), PROT_READ |PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1, 0);
    status_flag_b = mmap(NULL, sizeof(sem_t), PROT_READ |PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1, 0);
    status_flag_consume = mmap(NULL, sizeof(sem_t), PROT_READ |PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1, 0);
    status_flag_minimum = mmap(NULL, sizeof(sem_t), PROT_READ |PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1, 0);
    status_flag_produce = mmap(NULL, sizeof(sem_t), PROT_READ |PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1, 0);
    status_flag_read = mmap(NULL, sizeof(sem_t), PROT_READ |PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1, 0);
    Count = mmap(NULL, sizeof(int), PROT_READ |PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1, 0);
    Counter = mmap(NULL, sizeof(int), PROT_READ |PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1, 0);
    *Counter = 0;
//Inicjalizacja semaforów:

    sem_init(full,1,0);
    sem_init(empty,1,CAPACITY);
    sem_init(mutex,1,1);
    sem_init(status_flag_b,1,1);
    sem_init(status_flag_produce,1,1);
    sem_init(status_flag_consume,1,1);
    sem_init(status_flag_minimum,1,0);
    sem_init(status_flag_read,1,0);


    createConsumers();
    createProducerA();
    createProducerB();
    //Zeby nie zakończył się przed potomkami:
    while((*Counter) < NOOFELEMENTS)
    {
    }
    // printf("%d %d %d %d\n",producers[0],producers[1],consumers[0],consumers[1]);
    return 0;
}


//Tworzenie producenta A- tworzy jeden element:
void createProducerA()
{
    pid_t pid;
    pid = fork();
    if(pid == 0)
    {
        produceA();
    }
    else
    {
        producers[0] = pid;
    }
    return;
};
//Producent B - tworzy 2 elementy:
void createProducerB()
{
    pid_t pid;
    pid = fork();
    if(pid == 0)
    {
        produceB();
    }
    else
    {
        producers[1] = pid;
    }
    return;
};

void createConsumers()
{
    int i;
    for(i = 0; i < 2; ++i)
    {
        pid_t pid;
        pid = fork();
        if(pid == 0)
        {
            consume();
        }
        else
        {
            consumers[i] = pid;
        }
    }
    return;
};


void produceA()
{
    int temp;
    product newProdA;
    newProdA.Element = 'A';
    newProdA.whoReads = 0;
    while((*Counter)< NOOFELEMENTS)
    {
        //usleep(1000);
        sem_wait(status_flag_produce);
        sem_wait(empty);
        sem_wait(mutex);
        push_back(newProdA);
        sem_getvalue(status_flag_read,&temp);
        if(temp <= -1)
        {
            sem_post(status_flag_read);
        }
        if(*Count == MIN_BUFOR+1)
        {
            sem_post(status_flag_minimum);
        }
        sem_getvalue(status_flag_b,&temp);
        if(*Count == CAPACITY-1 && temp == 1)
            sem_wait(status_flag_b);
        sem_post(mutex);
        sem_post(full);
        sem_post(status_flag_produce);
        ++(*Counter);
    }
};

void produceB()
{
    int temp;
    product newProdB, newProdC;
    newProdB.Element = 'B';
    newProdB.whoReads = 0;
    newProdC.Element = 'C';
    newProdC.whoReads = 0;
    while((*Counter) < NOOFELEMENTS-1)
    {
        //usleep(1000);
        sem_wait(status_flag_b);
        sem_wait(status_flag_produce);
        sem_wait(empty);
        sem_wait(empty);
        sem_wait(mutex);
        push_back(newProdB);
        sem_getvalue(status_flag_read,&temp);
        if(*Count == MIN_BUFOR+1)
        {
            sem_post(status_flag_minimum);
        }
        push_back(newProdC);
        if(*Count == MIN_BUFOR+1)
        {
            sem_post(status_flag_minimum);
        }
        if(temp <= -1)
        {
            sem_post(status_flag_read);
        }
        if(*Count >= CAPACITY-1)
        {
            sem_post(mutex);
            sem_post(full);
            sem_post(full);
            sem_post(status_flag_produce);
        }
        else
        {
            sem_post(mutex);
            sem_post(full);
            sem_post(full);
            sem_post(status_flag_produce);
            sem_post(status_flag_b);
        }
        ++(*Counter);
    }
};

void consume()
{
    int t,p;
    while((*Counter)<NOOFELEMENTS)
    {
        usleep(10);
        sem_wait(status_flag_consume);
        sem_wait(mutex);
        sem_getvalue(status_flag_minimum,&t);
        sem_getvalue(status_flag_read,&p);
        if(*Count <= 3 && t == 0)
        {
            sem_post(mutex);
            sem_wait(status_flag_minimum);
            sem_wait(mutex);
        }
        if(bufor[*Count-1].whoReads == 0 || bufor[*Count-1].whoReads == getpid())
        {
            bufor[*Count-1].whoReads = getpid();
            printf("Reading %d, element: %c. Process: %d.\n",*Count,bufor[*Count-1].Element,getpid());
            sem_post(mutex);
            sem_post(status_flag_consume);
            sem_wait(status_flag_read);

        }
        else //if(bufor[*Count-1].whoReads!=0 && bufor[*Count-1].whoReads!= getpid())
        {
            pop_first();
            if(*Count == CAPACITY-2)
                sem_post(status_flag_b);
            sem_post(status_flag_read);
            sem_wait(full);
            sem_post(empty);
            sem_post(mutex);
            sem_post(status_flag_consume);
        }
    }
};
