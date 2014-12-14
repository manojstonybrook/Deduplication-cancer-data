/*
 * file_system.c
 *
 *  Created on: Nov 27, 2014
 *      Author: manoj
 */
#include <stdio.h>
#include <file_sys.h>
#define STORE
//#define CAC
#define SYNC

#include "../cac/content_aware_chunking.h"

char* file_name = "ecoli_s1.fasta";

int digit_numb(int num)
{
	if(num == 0)
	{
		return 1;
	}
	int dc=0;
	int num_copy = num;
	while(num_copy!=0)
	{
		dc++;
		num_copy=(num_copy)/10;
	}
	return dc;
}

int write_file_chunks(char *name,struct File *new_file)
{

	int n,j;
	unsigned char buf[512];
	FILE *readfile;

	readfile = fopen(name, "r");
	if(readfile ==NULL)
		printf("file not found");

	int block_no;
	if(new_file->f_size%BLKSIZE == 0)
		block_no=new_file->f_size/BLKSIZE;
	else
		block_no = (new_file->f_size/BLKSIZE)+1;

	int dup_count = 0;
	while((n = fread(&buf, sizeof(char), BLKSIZE ,readfile)) > 0)
	{
		//printf("%d %d", block_no, new_file->f_size);
		dup_count+=file_data_write(new_file, buf, n, block_no);
		block_no++;

	}
	//printf("\n");
	return dup_count;

}

void write_chunks()
{
	int nchunks = main_cac_fn(file_name);
	printf("\nNo of chunks: %d",nchunks);
	int i;
	struct File* new_file = (struct File*)malloc(sizeof(struct File));

	strcpy(new_file->key,file_name);
	int dup_count=0;
	for(i =0; i< nchunks;i++)
	{
		char file_name[sizeof "outfile10000"];
		sprintf(file_name, "outfile%d",i);
		dup_count+=write_file_chunks(file_name,new_file);
		//printf("\n\ni is %d",i);
	}
	file_meta_data_write(new_file);
	printf("\nDuplicate count %d",dup_count);
	//printf("\nNo. of blocks %d",block_no);
	float dedup_ratio;
	dedup_ratio = deduplication_ratio();

	printf("\n system_dedup_ratio %f", dedup_ratio);
	printf("\n finished");
}

void write_file(char *name)
{

	FILE *readfile;
	int n;
	//char* name = "ecoli2_parsed.txt";
	unsigned char buf[512];
	struct File* new_file = (struct File*)malloc(sizeof(struct File));

	strcpy(new_file->key,name);
	readfile = fopen(name, "r");

	int block_no=0;
	while((n = fread(&buf, sizeof(char), BLKSIZE ,readfile)) > 0)
	{
		//printf("\n n %d", n);
		file_data_write(new_file, buf, n, block_no);
		block_no++;
	}

	float dedup_ratio;
	dedup_ratio = deduplication_ratio();
	file_meta_data_write(new_file);
	printf("\n file size %d", new_file->f_size);
	printf("\n system_dedup_ratio %f", dedup_ratio);
	printf("\n finished");
}

int main()
{

int n;
unsigned char buf[512];
fs_init();

#ifdef STORE
	#ifdef SYNC
		fs_sync();
	#endif
	#ifdef CAC
		write_chunks();
	#else
		write_file(file_name);
    #endif


#else
	char *dump = "dump.fasta";
	struct File* file = (struct File*)malloc(sizeof(struct File));
	int err = file_stat(file_name, file);
	if(err < 0)
	{
		printf("\n File not Found");
		return 0;
	}
	FILE *dp = fopen(dump, "w");

	int b_no = 0;
	int count=0;
	printf("\n blocks %d", file->n_blocks);
	while((n = file_data_read(file, buf, b_no)) > 0)
	//while(b_no < file->n_blocks)
	{

	  n = file_data_read(file, buf, b_no);
	  if(n < 512)
	  {
		  printf("\nsize b_no %d %d",count, n);
		  count++;
	  }
	  fwrite(buf, sizeof(char), n, dp);
	  b_no++;

	  //printf("\n b_no n %d %d", b_no, n);

	}

	fclose(dp);
	printf(" \n Data wrote on manoj_dump.fasta \n finished");
#endif

return 0;

}

/*int main()
{

int n;
char* name = "ecoli_s.fasta";
char buf[512];
fs_init();

#ifdef STORE
	FILE *readfile;

    //If any new test case
#ifdef SYNC
	fs_sync();
#endif

	struct File* new_file = (struct File*)malloc(sizeof(struct File));
	strcpy(new_file->key,name);

	readfile = fopen(name, "r");
	int block_no=0;
	while((n = fread(&buf, sizeof(char), BLKSIZE ,readfile)) > 0)
	{
		//printf("\n iter %d n %d",block_no, n, sizeof(char));
		file_data_write(new_file, buf, n, block_no);
		block_no++;
	}

	float dedup_ratio;
	dedup_ratio = deduplication_ratio();
	file_meta_data_write(new_file);

	printf("\n system_dedup_ratio %f", dedup_ratio);
	printf("\n finished");

#else

	char *dump = "dump_manoj.fasta";
	struct File* file = (struct File*)malloc(sizeof(struct File));
	file_stat(name, file);

	FILE *dp = fopen(dump, "w");

	int b_no = 0;

	while((n = file_data_read(file, buf, b_no)) > 0)
	{
	  printf("\n %d", n);
	  fwrite(buf, sizeof(char), n, dp);
	  b_no++;
	}
	//printf("\n file size %d", file->f_size);

	fclose(dp);
	printf(" \n Data wrote on dump.fasta \n finished");
#endif

return 0;

}
*/
