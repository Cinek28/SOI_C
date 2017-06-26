#pragma once

#include <stdio.h>

#define NAME_MAX 32
#define BLOCK_SIZE 2048
//Flagi poczatku pliku i zajetosci bloku:
#define USED (1 << 0)
#define BEGINNING (1 << 1)

struct VirtualFileSystem
{
	FILE * F;
	unsigned int inodes_num;
	struct VirtualFileSystem_inode * inodes;
};
struct VirtualFileSystem_superblock
{
	size_t size;
};


struct VirtualFileSystem_inode
{
	unsigned int flags;
	char name[NAME_MAX];
	unsigned int size;
	unsigned int next_node;
};

struct VirtualFileSystem * fs_create(const char * file_name, const size_t size);
struct VirtualFileSystem * fs_open(const char * file_name);
void fs_close(struct VirtualFileSystem * v);
void fs_delete(const char * file_name);

void fs_info(const struct VirtualFileSystem * v);
void fs_ls(const struct VirtualFileSystem * v);
int fs_copy_to(const struct VirtualFileSystem * v, const char * source_file_name, const char * destination_file_name);
int fs_copy_from(const struct VirtualFileSystem * v, const char * source_file_name, const char * destination_file_name);
int fs_delete_file(const struct VirtualFileSystem * v, const char * file_name);

unsigned int fs_inodes_from_size(const size_t size);
unsigned int fs_required_inodes_for(const size_t size);
size_t fs_get_block_position(const struct VirtualFileSystem * v, const size_t inode);
