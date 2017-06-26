#include "FILE_SYSTEM.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

struct VirtualFileSystem * fs_create(const char * file_name, const size_t size)
{
	FILE * F;
	
	char block[128];//tablica do inicjalizacji katalogu
    //Zmienne pomocnicze:
	
	size_t bytes_remaining;
	size_t bytes_to_write;
	
	struct VirtualFileSystem_superblock sb;
	unsigned int inodes_num;
	
	struct VirtualFileSystem_inode * inodes;
	unsigned int I;
	
	struct VirtualFileSystem * v;
	
	//Otwarcie pliku w trybie binarnym:
	F = fopen(file_name, "wb");
	if(!F)
		return NULL;
	
	memset(block, 0, sizeof(block));
	bytes_remaining = size;
	//Inicjalizacja pliku/katalogu (wpisanie zer)
	while(bytes_remaining > 0)
	{
		bytes_to_write = sizeof(block);
//Jeżeli miejsce wyznaczone nie jest podzielne przez rozmiar bloku:
		if(bytes_to_write > bytes_remaining)
			bytes_to_write = bytes_remaining;
		
		fwrite(block, 1, bytes_to_write, F);
		
		bytes_remaining -= bytes_to_write;
	}
	
	fseek(F, 0, SEEK_SET);//Ustawienie znacznika na początek pliku
	//Informacja o rozmiarze katalogu w superbloku:
	sb.size = size;
	
	fwrite(&sb, sizeof(sb), 1, F);
	//Odczytanie możliwych do stworzenia "inodów" na podstawie rozmiaru:
	inodes_num = fs_inodes_from_size(size);
	
	inodes = malloc(sizeof(struct VirtualFileSystem_inode) * inodes_num);
	for(I = 0; I < inodes_num; I++)
		inodes[I].flags = 0;
	
	v = malloc(sizeof(struct VirtualFileSystem));
	v->F = F;
	v->inodes_num = inodes_num;
	v->inodes = inodes;
	
	return v;
}
struct VirtualFileSystem * fs_open(const char * file_name)
{
	FILE * F;
	
	size_t size_file;
	struct VirtualFileSystem_superblock sb;
	unsigned int inodes_num;
	
	struct VirtualFileSystem_inode * inodes;
	struct VirtualFileSystem * v;
	
	F = fopen(file_name, "r+b");
	if(!F)
		return NULL;
	//Odczytanie rozmiaru wirtualnego katalogu:
	fseek(F, 0, SEEK_END);
	size_file = ftell(F);//rozmiar w bajtach
    //Jeżeli rozmiar mniejszy od rozmiaru superbloku, nie może być fs:
	
	if(size_file < sizeof(struct VirtualFileSystem_superblock))
	{
		fclose(F);
		return NULL;
	}
	fseek(F, 0, SEEK_SET);
	//Odczytanie superbloku:
	if(fread(&sb, sizeof(sb), 1, F) <= 0)
	{
		fclose(F);
		return NULL;
	}
	//Jeżeli rozmiar wskazywany mniejszy niż rzeczywisty:
	if(sb.size != size_file)
	{
		fclose(F);
		return NULL;
	}
	
	inodes_num = fs_inodes_from_size(size_file);
	
	inodes = malloc(sizeof(struct VirtualFileSystem_inode) * inodes_num);
	//Jeżeli błąd odczytu:
	if(fread(inodes, sizeof(struct VirtualFileSystem_inode), inodes_num, F) <= 0)
	{
		fclose(F);
		free(inodes);
		return NULL;
	}
	
	v = malloc(sizeof(struct VirtualFileSystem));
	v->F = F;
	v->inodes_num = inodes_num;
	v->inodes = inodes;
	
	return v;
}
void fs_close(struct VirtualFileSystem * v)
{	//Zapisanie danych z inodów:
	fseek(v->F, sizeof(struct VirtualFileSystem_superblock), SEEK_SET);
	fwrite(v->inodes, sizeof(struct VirtualFileSystem_inode), v->inodes_num, v->F);
	//Zwolnienie zaalokowanej pamięci:
	fclose(v->F);
	free(v->inodes);
	free(v);
	v = NULL;
}
//Usunięcie wirtualnego dysku- katalogu:
void fs_delete(const char * file_name)
{
	unlink(file_name);
}






void fs_info(const struct VirtualFileSystem * v)
{
	unsigned int I;
	unsigned int free_nodes;
	
	printf("Inody:\n");
	for(I = 0; I < v->inodes_num; I++)
	{//Wypisuje po kolei inody:
		printf("ID: %d\n", I);
		printf("Flaga: %d\n", v->inodes[I].flags);
		printf("Nazwa: %s\n", v->inodes[I].name);
		printf("Rozmiar danych: %d\n", v->inodes[I].size);
		printf("Nastepny: %d\n", v->inodes[I].next_node);
		printf("\n");
	}
	free_nodes = 0;
	for(I = 0; I < v->inodes_num; I++)
	{
		if(!(v->inodes[I].flags & USED))
			free_nodes++;
	}
	
	printf("\n");
	printf("Wolne inody/Zajete: %d/%d\n", free_nodes, v->inodes_num);
}
//Funkcja przypominająca ls (wypisuje pliki w katalogu):
void fs_ls(const struct VirtualFileSystem * v)
{
	unsigned int I;
	for(I = 0; I < v->inodes_num; I++)
	{
		if((v->inodes[I].flags & USED) && (v->inodes[I].flags & BEGINNING))
		{
			printf("Plik: %s /%d\n", v->inodes[I].name, I);
		}
	}
}
int fs_copy_to(const struct VirtualFileSystem * v, const char * source_file_name, const char * destination_file_name)
{
	FILE * source_file;
	//Rozmiar pliku zrodlowego:
	size_t source_file_length;
	//Potrzebna liczba inodów:
	unsigned int required_inodes;
	//Zmienne pomocnicze:
	unsigned int * nodes_queue;
	unsigned int current_inode;
	unsigned int current_queue_inode;
	unsigned int I;
	char read_buffer[BLOCK_SIZE];
	//Bledna nazwa pliku:
	if(strlen(destination_file_name) == 0)
		return -1;
	
	for(I = 0; I < v->inodes_num; I++)
	{	//Jezeli wystepuje juz taka nazwa:
		if(v->inodes[I].flags & USED && v->inodes[I].flags & BEGINNING && strncmp(v->inodes[I].name, destination_file_name, NAME_MAX) == 0)
			return -2;
	}
	//Otwarcie pliku, z ktorego kopiujemy:
	source_file = fopen(source_file_name, "rb");
//Jezeli sie nie powiedzie:
	if(source_file == NULL)
		return -3;
	//Odczytanie wilekosci pliku:
	fseek(source_file, 0, SEEK_END);
	source_file_length = ftell(source_file);
	fseek(source_file, 0, SEEK_SET);
	
	required_inodes = fs_required_inodes_for(source_file_length);
	nodes_queue = malloc(required_inodes * sizeof(unsigned int));
	
	current_queue_inode = 0;
	for(current_inode = 0; current_inode < v->inodes_num; current_inode++)
	{//Sprawdzenie do ktorych inodow mozna wczytac:
		if((v->inodes[current_inode].flags & USED) == 0)
			nodes_queue[current_queue_inode++] = current_inode;
 //Jezeli odczytane wszystkie:
		if(current_queue_inode == required_inodes)
			break;
	}
	//Za malo miejsca do wpisania:
	if(current_queue_inode < required_inodes)
	{
		free(nodes_queue);
		fclose(source_file);
		
		return -4;
	}
	//Wczytanie z bufora do nowego pliku:
	for(I = 0; I < required_inodes; I++)
	{
		v->inodes[nodes_queue[I]].flags = USED;
		v->inodes[nodes_queue[I]].size = fread(read_buffer, 1, sizeof(read_buffer), source_file);
		
		fseek(v->F, fs_get_block_position(v, nodes_queue[I]), SEEK_SET);
		fwrite(read_buffer, 1, v->inodes[I].size, v->F);
		if(I == 0)
		{
			v->inodes[nodes_queue[I]].flags |= BEGINNING;
			strncpy(v->inodes[nodes_queue[I]].name, destination_file_name, NAME_MAX);
		}
		if(I < required_inodes - 1)
			v->inodes[nodes_queue[I]].next_node = nodes_queue[I + 1];
		else
			v->inodes[nodes_queue[I]].next_node = -1;
	}
	
	free(nodes_queue);
	fclose(source_file);
	
	return required_inodes;
}
int fs_copy_from(const struct VirtualFileSystem * v, const char * source_file_name, const char * destination_file_name)
{
	FILE * destination_file;
	unsigned int I;
	unsigned int start_node;
	
	char buffer[BLOCK_SIZE];
	
	destination_file = fopen(destination_file_name, "wb");
	if(!destination_file)
		return -1;
	
	//Wyszukiwanie pliku w fs:
	start_node = -1;
	for(I = 0; I < v->inodes_num; I++)
	{
		if(v->inodes[I].flags & USED && v->inodes[I].flags & BEGINNING && strncmp(v->inodes[I].name, source_file_name, NAME_MAX) == 0)
		{
			start_node = I;
			break;
		}
	}
	
	if(start_node == -1)
		return -2;
	
	while(start_node != -1)
	{//Blad odczytu:
		if(fread(buffer, 1, v->inodes[start_node].size, v->F) != v->inodes[start_node].size)
		{
			fclose(destination_file);
			return -3;
		}
		//Zapisywanie danych do pliku:
		fwrite(buffer, 1, v->inodes[start_node].size, destination_file);
		
		start_node = v->inodes[start_node].next_node;
	}
	
	fclose(destination_file);
	
	return 1;
	
}
int fs_delete_file(const struct VirtualFileSystem * v, const char * file_name)
{
	unsigned int I;
	unsigned int start_node;
	const char* empty = "";
	start_node = -1;
	for(I = 0; I < v->inodes_num; I++)
	{	//Sprawdzenie, czy istnieje i od ktorego noda:
		if(v->inodes[I].flags & USED && v->inodes[I].flags & BEGINNING && strncmp(v->inodes[I].name, file_name, NAME_MAX) == 0)
		{
			start_node = I;
			break;
		}
	}
	//Jezeli nie istnieje:
	if(start_node == -1)
		return -1;
	unsigned int temp_node;
	while(start_node != -1)
	{	//Zamiana na nieuzywany:
		v->inodes[start_node].flags &= ~USED;
		v->inodes[start_node].flags &= ~BEGINNING;
		strcpy(v->inodes[start_node].name,empty);
		v->inodes[start_node].size = 0;
		temp_node = start_node;
		start_node = v->inodes[start_node].next_node;
		v->inodes[temp_node].next_node = -1;
	}
	
	return 1;
}


//Ile inodow i danych mozna zmiescic w fs (kazdy inode powiazany z blokiem pamieci):
unsigned int fs_inodes_from_size(const size_t size)
{
	return (size - sizeof(struct VirtualFileSystem_superblock)) / (sizeof(struct VirtualFileSystem_inode) + BLOCK_SIZE);
}
unsigned int fs_required_inodes_for(const size_t size)
{
	size_t size_remaining;
	unsigned int required_inodes = 0;
	size_t current_block_length;
	
	size_remaining = size;
	//Zliczanie ile blokow pamieci zmiesci sie w danej wielkosci pamieci:
	do
	{
		required_inodes++;
		
		current_block_length = size_remaining;
		if(current_block_length > BLOCK_SIZE)
			current_block_length = BLOCK_SIZE;
		
		size_remaining -= current_block_length;
	}
	while(size_remaining > 0);
	
	return required_inodes;
}
size_t fs_get_block_position(const struct VirtualFileSystem * v, const size_t inode)
{
	return sizeof(struct VirtualFileSystem_superblock) + sizeof(struct VirtualFileSystem_inode) * v->inodes_num + BLOCK_SIZE * inode;
}
