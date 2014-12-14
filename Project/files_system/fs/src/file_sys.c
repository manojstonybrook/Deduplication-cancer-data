/*
 * fs.c
 *
 *  Created on: Nov 27, 2014
 *      Author: manoj
 */

#include <file_sys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sha1.h>

struct SuperBlock *super;		// superblock
FILE *fp;
int block_entries[128];

void fs_init(void)
{

 fp = fopen(FS_NAME, "r+");

 if(fp==NULL)//if file does not exists
 {
	 printf("\nFILE SYSTEM Don't Exist\n");
	 return;
 }
 else
	 printf("\n FS is initialized\n");

 initialize_superblock();
 //fclose(fp);
 memset(block_entries, 0, ENTRY);

}

void initialize_superblock(void)
{
   uint sb_tail, block_tail, test;
   uint sb_tblocks, sb_wblocks;

   //Superblock TAIL INITIALIZE
   fseek(fp,superblock_tail_addr,SEEK_SET);
   test = fread(&sb_tail,sizeof(int),1,fp);

   if(sb_tail == 0)
   {
	 printf("First time here");
	 SuperBlockTail_Update(SuperBlockAddr_start);
   }
   /*else
   {
	   printf("\n sb_tail %d", sb_tail);
   }*/

   //Datablock TAIL INITIALIZE
    fseek(fp,datablock_tail_addr,SEEK_SET);
    fread(&block_tail,sizeof(int),1,fp);

   if(block_tail == 0)
   {
	 printf("First time here");
	 DataBlockTail_Update(DataBlock_Start);
   }
   /*else
   {
	   printf("\n db_tail %x\n", block_tail);
   }*/

   // Superblock number of block written initialize
   fseek(fp,superblock_total_blocks,SEEK_SET);
   fread(&sb_tblocks,sizeof(int),1,fp);

   fseek(fp,superblock_written_blocks,SEEK_SET);
   fread(&sb_wblocks,sizeof(int),1,fp);

   super = (struct SuperBlock *)malloc(sizeof(struct SuperBlock));
   super->tail_sb = sb_tail;
   super->tail_db = block_tail;
   super->total_blocks = sb_tblocks;
   super->written_blocks = sb_wblocks;

}

int SuperBlockTail_Update(int offset)
{
	size_t n;
	fseek(fp,superblock_tail_addr,SEEK_SET);
	n = fwrite(&offset,sizeof(int),1,fp);
	if(n != 1)
	{
		printf("SuperBlockTail_Update error");
		return -1;
	}
	return 0;
}

int DataBlockTail_Update(int offset)
{
	size_t n;
	fseek(fp,datablock_tail_addr,SEEK_SET);
	n = fwrite(&offset,sizeof(int),1,fp);
	if(n != 1)
	{
		printf("DataBlockTail_Update error");
		return -1;
	}

	return 0;
}

void superblock_update(void)
{
	 fseek(fp,superblock_total_blocks,SEEK_SET);
	 fwrite(&super->total_blocks,sizeof(int),1,fp);

	 fseek(fp,superblock_written_blocks,SEEK_SET);
	 fwrite(&super->written_blocks,sizeof(int),1,fp);

}

int	file_meta_data_write(struct File *f)
{
	fseek(fp,super->tail_sb,SEEK_SET);
	fwrite(f, sizeof(struct File), 1, fp);

	//int n_blocks = f->f_size/BLKSIZE;
	//int entry = f->n_blocks/ENTRY;

	//int addr = f->f_direct[entry];
    //fseek(fp,addr,SEEK_SET);
    //fwrite(block_entries, BLKSIZE, 1, fp);
    super->tail_sb = super->tail_sb + sizeof(struct File);
    SuperBlockTail_Update(super->tail_sb);
    DataBlockTail_Update(super->tail_db);
    superblock_update();
	return 0;

}

int file_get_block(struct File *f, uint32_t file_blockno, uint *pblk)
{

  if(file_blockno >= NDIRECT * ENTRY)
  {
	 printf("\nFile Size Exceeded\n");
	 return -1;
  }


  if(file_blockno%ENTRY == 0)
  {
	 f->f_direct[file_blockno/ENTRY] = super->tail_db;
	 super->tail_db = super->tail_db + BLKSIZE;
	 /*if(file_blockno/ENTRY > 0)
	 {
		 int addr = f->f_direct[(file_blockno/ENTRY) - 1];
		 fseek(fp,addr,SEEK_SET);
		 fwrite(block_entries, BLKSIZE, 1, fp);
		 memset(block_entries, 0, ENTRY);
	 }*/
  }

  int offset = file_blockno%ENTRY;

  int base_addr = (int)((int *)f->f_direct[file_blockno/ENTRY] + offset);

  fseek(fp,base_addr,SEEK_SET);
  fwrite(&super->tail_db, sizeof(int), 1, fp);
  *pblk = super->tail_db;
  super->tail_db += BLKSIZE;

  //block_entries[offset] = super->tail_db;
  //if(alloc_block(pblk) < 0)
  //{
	// printf("\n error in block allocation for write");
	  //return -1;
  //}

  return 0;
}


int	file_data_write(struct File *f, unsigned char *buf, size_t count, int block_no)
{
    int err;
	uint pblk, hash_addr;
	uint addr;
	uint hash_second_table_addr;
	uint second_table_offset;

	uint32_t hash = generate_hash(buf, count);
    uint hash_first_table_addr = HashBlock_Start + (hash & HashLowBits);

    fseek(fp,hash_first_table_addr,SEEK_SET);
    fread(&addr, sizeof(uint), 1, fp);

    if(!addr)
    {

    	if(alloc_block(&hash_addr) < 0)
    	{
        	printf("\n error in block allocation for write");
        	return -1;
    	}
    	fseek(fp,hash_first_table_addr,SEEK_SET);
    	fwrite(&hash_addr, sizeof(uint), 1, fp);
    	flush_block(hash_addr);

    	if((err = file_get_block(f, block_no, &pblk)) < 0)
    	{
    		printf("\n error in getting block for write");
    		return -1;
    	}

    	flush_block(pblk);
    	fseek(fp,pblk,SEEK_SET);
    	fwrite(buf, sizeof(unsigned char), count, fp);

    	f->f_size += count;
    	f->f_written += count;
    	f->n_blocks += 1;
    	super->total_blocks+=1;
    	super->written_blocks+=1;

    	second_table_offset = (hash & HashHighBits) >> 25;
    	//printf("\n %d", second_table_offset);
    	hash_second_table_addr = (uint)((uint *)hash_addr + second_table_offset);

    	fseek(fp,hash_second_table_addr,SEEK_SET);
    	fwrite(&pblk, sizeof(uint), 1, fp);

    }
    else
    {
    	uint blk_addr = 0;
    	second_table_offset = (hash & HashHighBits) >> 25;
    	hash_second_table_addr = (uint)((uint *)addr + second_table_offset);
    	fseek(fp,hash_second_table_addr,SEEK_SET);
    	fread(&blk_addr, sizeof(uint), 1, fp);

    	if(blk_addr && blk_addr < super->tail_db)
    	{
       	   write_deduplicate_block_addr(f, block_no, blk_addr);
    	   f->f_size += count;
    	   f->n_blocks += 1;
    	   super->total_blocks+=1;

    	}
    	else
    	{
    		//printf("hash addr %x", hash_addr);
			if((err = file_get_block(f, block_no, &pblk)) < 0)
			{
				printf("\n error in getting block for write");
				return -1;
			}

			flush_block(pblk);
			fseek(fp,pblk,SEEK_SET);
			fwrite(buf, sizeof(unsigned char), count, fp);

			f->f_size += count;
			f->f_written += count;
			f->n_blocks += 1;
			super->total_blocks  += 1;
			super->written_blocks += 1;

			fseek(fp,hash_second_table_addr,SEEK_SET);
			fwrite(&pblk, sizeof(uint), 1, fp);

    	}


    }


	return 0;
}

int file_stat(char * key, struct File *f)
{

	int i, addr = sizeof(struct SuperBlock);
    int n_files = (super->tail_sb - sizeof(struct SuperBlock))/sizeof(struct File);

    //printf("\n n_files %d", n_files);
    for(i = 0; i < n_files; i ++)
    {
    	fread(f, sizeof(struct File), 1, fp);
    	if(strcmp(f->key, key) == 0)
    		break;
    }
    if(i == n_files)
    {
    	return -1;
    }
	return 0;
}



int write_deduplicate_block_addr(struct File *f, uint32_t file_blockno, int pblk)
{

  int i;
  char buf1[512];
  if(file_blockno >= NDIRECT * ENTRY)
  {
	 printf("\nFile Size Exceeded\n");
	 return -1;
  }

  if(file_blockno%ENTRY == 0)
  {
	 f->f_direct[file_blockno/ENTRY] = super->tail_db;
	 super->tail_db = super->tail_db + BLKSIZE;

	 /*if(file_blockno/ENTRY > 0)
	 {
		 int addr = f->f_direct[(file_blockno/ENTRY) - 1];
		 fseek(fp,addr,SEEK_SET);
		 fwrite(block_entries, BLKSIZE, 1, fp);
		 memset(block_entries, 0, ENTRY);
	 }*/
  }

  int offset = file_blockno%ENTRY;
  uint base_addr = (uint)((uint *)f->f_direct[file_blockno/ENTRY] + offset);
  fseek(fp,base_addr,SEEK_SET);
  fwrite(&pblk, sizeof(uint), 1, fp);
  //super->tail_db += BLKSIZE;

  //if(file_blockno == 5168)
  {

  //fseek(fp,pblk,SEEK_SET);
  //fread(buf1, sizeof(char), BLKSIZE, fp);

  //for(i = 0; i < 512; i++)
	//  printf("\n%d %c ", i, buf1[i]);
  }


  //int offset = file_blockno%ENTRY;
  //printf("\n dedup block_no %d baddr %d addr %d offset %d", file_blockno, file_blockno/ENTRY, pblk, file_blockno%ENTRY);
  //block_entries[offset] = pblk;
  return 0;
}


int	file_data_read(struct File *f, unsigned char *buf, int block_no)
{
	   int i;
	   if(block_no >= (f->n_blocks))
		  return -1;

	   unsigned char buf1[512];
	   int f_direct_no = block_no/ENTRY;
	   int offset = block_no%ENTRY;
	   uint *base_addr = (uint *)f->f_direct[f_direct_no];
	   uint addr = (uint)(base_addr + offset);
	   uint val;

	   fseek(fp,addr,SEEK_SET);
	   fread(&val, sizeof(uint), 1, fp);
	   fseek(fp,val,SEEK_SET);
       fread(buf1, sizeof(unsigned char), BLKSIZE, fp);
	   for(i = 0; i < BLKSIZE; i++)
	   {
		   unsigned char val,val2;
		   val = buf1[i];
		   val2 = buf1[i+1];

		   //fread(&val, sizeof(unsigned char), 1, fp);
		   if(!(val == 'A' || val == 'G' || val == 'C' || val == 'T') && !(val2 == 'A' || val2 == 'G' || val2 == 'C' || val2 == 'T'))
		   {
			   //printf("cc %c\n", val);
		   //if(val == 0)
			   break;
		   }
		   else
			   buf[i] = val;
	   }
	   return i;

}

uint32_t generate_hash(const char *buf, size_t count)
{

	SHA1Context sha;
	SHA1Reset(&sha);
	SHA1Input(&sha, (const unsigned char *)buf, count);

	if(SHA1Result(&sha))
	{
		//printf("SHA OUTPUT: %08x\n", sha.Message_Digest[4]);
		return sha.Message_Digest[4];
	}
	else
		printf("SHA not generated\n");

	return 0;
}

int alloc_block(uint * addr)
{
	int db_addr = super->tail_db;

	if(db_addr < DataBlock_Start || db_addr >  DataBlock_end)
	{
	   printf("\n Not able to allocate block");
	   return -1;
	}
    *addr = db_addr;
    super->tail_db += BLKSIZE;
    return 0;
}

void fs_sync(void)
{
   //Remove everything and initialize superblock
	int n = (DataBlock_Start  - SuperBlockAddr_start)/BLKSIZE;
	int i;
	char buf[BLKSIZE];
	memset(buf, 0,  BLKSIZE);
	fseek(fp, SuperBlockAddr_start, SEEK_SET);
	//printf("\n blocks %d ", n);
	for(i = 0; i < n+1; i++)
	{
      fwrite(buf, sizeof(unsigned char), BLKSIZE, fp);
	}

    SuperBlockTail_Update(SuperBlockAddr_start);
    DataBlockTail_Update(DataBlock_Start);
    super->total_blocks = 0;
    super->written_blocks = 0;
    superblock_update();
    super->tail_sb = SuperBlockAddr_start;
    super->tail_db = DataBlock_Start;

}

void flush_block(uint addr)
{
	unsigned char buf[BLKSIZE];
	memset(buf, NULL, BLKSIZE);
	fseek(fp,addr,SEEK_SET);
	fwrite(buf, sizeof(unsigned char), BLKSIZE, fp);
}

float deduplication_ratio()
{
  //printf("dedup %d %d", super->total_blocks, super->written_blocks);
  return (super->total_blocks/(1.0*super->written_blocks));
}
