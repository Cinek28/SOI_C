#include "monitor2.hpp"
#include <stdio.h>

//Instancja monitora:
PCMonitor mon;
//Funkcja (wątek) produkujący literę (producent A lub B w zależności od argumentu):
void *producerA(void * args)
{
  while(true)
  {
	char x=*((char *)args);
//	printf("A wyprodukowalem %c\n",x);
	mon.putA();
	sleep(rand()%10);
  }
}
//Funkcja (wątek) produkujący literę (producent A lub B w zależności od argumentu):
void *producerB(void * args)
{
  while(true)
  {
	char x=*((char *)args);
//	printf("A wyprodukowalem %c\n",x);
	mon.putB();
	sleep(rand()%10);
  }
}

//Funkcja (wątek) konsumujący (konsument A lub B- w zależności od argumentu):
void *consumerA(void * args)
{
  char n=*((char *)args);
  while(true)
  {
      mon.getA();
//	printf("B_skonsumowalem %c\n",x);
	//sleep(rand()%3);
  }

}

void *consumerB(void * args)
{
  char n=*((char *)args);
  while(true)
  {
      mon.getB();
//	printf("B_skonsumowalem %c\n",x);
	//sleep(rand()%3);
  }

}

int main()
{
  pthread_t prodA, consA, prodB,consB;
  char a='A';
  char b='B';
  pthread_create(&prodA,NULL,producerA, &a);
  pthread_create(&consA,NULL,consumerA, &a);
  pthread_create(&prodB,NULL,producerB, &b);
  pthread_create(&consB,NULL,consumerB, &b);

  pthread_join(prodA,NULL);
  pthread_join(consA,NULL);
  pthread_join(prodB,NULL);
  pthread_join(consB,NULL);

  return 0;
}
