#ifndef __monitor_h
#define __monitor_h

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#endif

class Semaphore
{
public:

  Semaphore( int value )
  {
#ifdef _WIN32
	sem = CreateSemaphore( NULL, value, 1, NULL );
#else
     if( sem_init( & sem, 0, value ) != 0 )
       throw "sem_init: failed";
#endif
  }
  ~Semaphore()
  {
#ifdef _WIN32
	CloseHandle( sem );
#else
	  sem_destroy( & sem );
#endif
  }

  void p()
  {
#ifdef _WIN32
	  WaitForSingleObject( sem, INFINITE );
#else
     if( sem_wait( & sem ) != 0 )
       throw "sem_wait: failed";
#endif
  }

  void v()
  {
#ifdef _WIN32
	  ReleaseSemaphore( sem, 1, NULL );
#else
     if( sem_post( & sem ) != 0 )
       throw "sem_post: failed";
#endif
  }


private:

#ifdef _WIN32
	HANDLE sem;
#else
	sem_t sem;
#endif
};

class Condition
{
  friend class Monitor;

public:
	Condition() : w( 0 )
	{
		waitingCount = 0;
	}

	void wait()
	{
		w.p();
	}

	bool signal()
	{
		if( waitingCount )
		{
			-- waitingCount;
			w.v();
			return true;
		}//if
		else
			return false;
	}

private:
	Semaphore w;
	int waitingCount; //liczba oczekujacych watkow
};


class Monitor
{
public:
	Monitor() : s( 1 ) {}

	void enter()
	{
		s.p();
	}

	void leave()
	{
		s.v();
	}

	void wait( Condition & cond )
	{
		++ cond.waitingCount;
		leave();
		cond.wait();
	}

	void signal( Condition & cond )
	{
		if( cond.signal() )
			enter();
	}


private:
	Semaphore s;
};

//Dodane definicje:
#define SIZE 10

//Element wstawiany do bufora:
struct Element
{
    char element;
    int aRead = 0; // wskazuje czy konsument A czytał element;
    int bRead = 0; //wskazuje czy konsument B czytał element;
};

//Klasa PCMonitor do poblemu producent-konsument:
class PCMonitor : Monitor
{
  private:
	Element buffer[SIZE];   //Bufor SIZE- elementowy
	int count;				// Liczba elementów w buforze
	int in;					// Miejsce zapisu w buforze
	int out; 				// Skąd czytać element
	Condition ProducerA;
	Condition ProducerB;
	Condition ConsumerA;
	Condition ConsumerB;
  public:
	PCMonitor();
	void putA();
	void putB();
    char getA();
    char getB();
};

PCMonitor::PCMonitor()
{
  in=out=count=0;
}

//Funkcja dodawania elementu:
void PCMonitor::putA()
{
  enter();
    //Bufor zapełniony- oczekiwanie na wyczyszczenie (producent A oczekuje):
  if(count == SIZE){
	wait(ProducerA);
  }
  	buffer[in].element = 'A';
  	buffer[in].aRead = 0;
  	buffer[in].bRead = 0;
  	in = (in+1)%SIZE;
  	++count;
  	printf("Dodaje element: 'A'. Rozmiar: %d. \n",count);
  //Jeżeli elementy mogą być usuwane:
  if(count>3)
  {
	if(buffer[out].aRead == 0)
	  signal(ConsumerA);
	if(buffer[out].bRead == 0)
	  signal(ConsumerB);
  }
  leave();
}

//Funkcja dodawania elementu:
void PCMonitor::putB()
{
  enter();
  //Do bufora nie wejdą dwa elementy na raz (oczekuje producent B):
  if(count >= SIZE-1)
	wait(ProducerB);

  	buffer[in].element = 'B';
  	buffer[in].aRead = 0;
  	buffer[in].bRead = 0;
  	in = (in+1)%SIZE;
  	++count;
  	printf("Dodaje element: 'B'. Rozmiar: %d.\n",count);
  	buffer[in].element = 'B';
  	buffer[in].aRead = 0;
  	buffer[in].bRead = 0;
  	in = (in+1)%SIZE;
  	++count;
  	printf("Dodaje element: 'B'. Rozmiar: %d.\n",count);
  //Jeżeli elementy mogą być usuwane:
  if(count>3)
  {
	if(buffer[out].aRead == 0)
	  signal(ConsumerA);
	if(buffer[out].bRead == 0)
	  signal(ConsumerB);
  }
  leave();
}
//Zadanie konsumentów:
char PCMonitor::getA()
{
  enter();
  char value;
  // Jeżeli A już przeczytał- oczekuje:
  if((buffer[out].aRead == 1 || count <= 3))
  {
	wait(ConsumerA);
  }
  //Odczytanie zawartości czytanego elementu bufora:
  value = buffer[out].element;

    printf("Czyta A- element: %c. Rozmiar: %d.\n",value,count);
    //Ustawienie odczytania elementu przez konsumenta A:
	buffer[out].aRead = 1;

  //Jeżeli zostało przeczytane przez obu konsumentów:
  if(buffer[out].aRead == 1 && buffer[out].bRead == 1)
  {
    out = (out+1)%SIZE;
    if(count <= 3)
        system("PAUSE");
	--count;
	printf("Usuwanie elementu z bufora: %c. Rozmiar: %d.\n",value,count);

  }
  //Obudzenie producenta A i B jeżeli flagi są odblokowane:
  if(count == SIZE - 2)
	signal(ProducerB);
  else if(count == SIZE - 1)
	signal(ProducerA);
  //Budzenie konsumentów (jeżeli nie mogą ich obudzić producenci):
  if(count == SIZE && buffer[out].bRead == 0)
  {
    signal(ConsumerB);
  }
  leave();
  return value;
}

char PCMonitor::getB()
{
  enter();

  char value;
  // Jeżeli A już przeczytał- oczekuje:
  if((buffer[out].bRead == 1 || count <= 3))
  {
    //Zwiększenie flagi oczekiwania na usunięcie konsumenta A
	wait(ConsumerB);
  }
  //Odczytanie zawartości czytanego elementu bufora:
  value = buffer[out].element;
  //Sprawdzenie kto czyta:
    printf("Czyta B- element: %c. Rozmiar: %d.\n",value,count);
    //Ustawienie odczytania elementu przez konsumenta A:
	buffer[out].bRead = 1;
  //Jeżeli zostało przeczytane przez obu konsumentów:
  if(buffer[out].aRead == 1 && buffer[out].bRead == 1)
  {
    out = (out+1)%SIZE;
    if(count <= 3)
        system("PAUSE");
	--count;
	printf("Usuwanie elementu z bufora: %c. Rozmiar: %d.\n",value,count);

  }
  //Obudzenie producenta A i B jeżeli flagi są odblokowane:
  if(count == SIZE - 2)
	signal(ProducerB);
  if(count == SIZE - 1)
	signal(ProducerA);
  //Budzenie konsumentów (jeżeli nie mogą ich obudzić producenci):
  if(count == SIZE && buffer[out].aRead == 0)
  {
    signal(ConsumerA);
  }
  leave();
  return value;
}

#endif



