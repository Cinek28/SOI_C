#include <stdio.h>
#include <stdlib.h>
#include "FILE_SYSTEM.h"

#include <string.h>

int main(int ArgC, char ** ArgV)
{
	char * fs_name;
	char * command;
	struct VirtualFileSystem* v;
	
	if(ArgC < 3)
	{
		printf("%s <fs name> command (...)\n", ArgV[0]);
		printf(
			"Dostepne komendy: \n"
			"- create <rozmiar w bajtach>\n"
			"- info\n"
			"- ls\n"
			"- add <nazwa pliku zrodlowego> <nazwa pliku docelowego>\n"
			"- copy <source file name> <destination file name>\n"
			"- remove <file name>\n"
			"- delete\n"
		);
		return 1;
	}
	
	fs_name = ArgV[1];
	command = ArgV[2];
	
	if(strcmp("create", command) == 0)
	{
		if(ArgC == 4)
		{
			size_t size = atoi(ArgV[3]);
			v = fs_create(fs_name, size);
			if(!v)
			{
				printf("Nie udalo sie utworzyc dysku wirtualnego!\n");
				return 2;
			}
			fs_close(v);
		}
		else
			printf("%s <vfs name> create <size in bytes>\n", ArgV[0]);
	}
	else if(strcmp("info", command) == 0)
	{
		if(ArgC == 3)
		{
			v = fs_open(fs_name);
			if(!v)
			{
				printf("Nie udalo sie otworzyc dysku wirtualnego!\n");
				return 2;
			}
			
			fs_info(v);
			
			fs_close(v);
		}
		else
			printf("%s <vfs name> info\n", ArgV[0]);
	}
	else if(strcmp("ls", command) == 0)
	{
		if(ArgC == 3)
		{
			v = fs_open(fs_name);
			if(!v)
			{
				printf("Nie udalo sie otworzyc dysku wirtualnego!\n");
				return 2;
			}
			
			fs_ls(v);
			
			fs_close(v);
		}
		else
			printf("%s <vfs name> ls\n", ArgV[0]);
	}
	else if(strcmp("add", command) == 0)
	{
		if(ArgC == 5)
		{
			v = fs_open(fs_name);
			if(!v)
			{
				printf("Nie udalo sie otworzyc dysku wirtualnego!\n");
				return 2;
			}
			
			printf("Wysylanie pliku, wynik: %d\n", fs_copy_to(v, ArgV[3], ArgV[4]));
			
			fs_close(v);
		}
		else
			printf("%s <vfs name> add <source file name> <destination file name>\n", ArgV[0]);
	}
	else if(strcmp("copy", command) == 0)
	{
		if(ArgC == 5)
		{
			v = fs_open(fs_name);
			if(!v)
			{
				printf("Nie udalo sie otworzyc dysku wirtualnego!\n");
				return 2;
			}
			
			printf("Pobieranie pliku, wynik: %d\n", fs_copy_from(v, ArgV[3], ArgV[4]));
			
			fs_close(v);
		}
		else
			printf("%s <vfs name> copy <source file name> <destination file name>\n", ArgV[0]);
	}
	else if(strcmp("remove", command) == 0)
	{
		if(ArgC == 4)
		{
			v = fs_open(fs_name);
			if(!v)
			{
				printf("Nie udalo sie otworzyc dysku wirtualnego!\n");
				return 2;
			}
			
			fs_delete_file(v, ArgV[3]);
			
			fs_close(v);
		}
		else
			printf("%s <vfs name> copy <source file name> <destination file name>\n", ArgV[0]);
	}
	else if(strcmp("delete", command) == 0)
	{
		if(ArgC == 3)
		{
			fs_delete(fs_name);
		}
		else
			printf("%s <vfs name> delete\n", ArgV[0]);
	}
	else
	{
		printf("%s invalid command `%s`\n", ArgV[0], command);
		return 1;
	}
	
	
	return 0;
}
