/* This file is part of the UniSet project
 * Copyright (c) 2009 Free Software Foundation, Inc.
 * Copyright (c) 2009 Ivan Donchevskiy
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
// --------------------------------------------------------------------------
/*! \file
 *  \author Ivan Donchevskiy
 *  \date   $Date: 2009/07/15 15:55:00 $
 *  \version $Id: Jrn.h,v 1.0 2009/07/15 15:55:00vpashka Exp $
 */
// --------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Storages.h"

bool KeyCompare(char* key1, char* key2, int cnt)
{
	bool fl=true;
	for(int i=0;i<cnt;i++)
	{
		if(*(key1+sizeof(TableBlockStorageElem)+i)!=*(key2+i))
		{
			fl=false;
			break;
		}
	}
	return fl;
}

TableBlockStorage::TableBlockStorage()
{
	size=0;
	k_size=0;
	inf_size=0;
	max=-1;
	lim=0;
	cur_block=0;
	block_number=0;
	block_size=0;
	seekpos=0;
	file=NULL;
}

TableBlockStorage::TableBlockStorage(const char* name, int key_sz, int inf_sz, int sz, int block_num, int block_lim, int seek)
{
	if(file!=NULL) fclose(file);
	if(!Open(name, key_sz, inf_sz, sz, block_num, block_lim, seek))
		Create(name, key_sz, inf_sz, sz, block_num, block_lim, seek);
}

TableBlockStorage::~TableBlockStorage()
{
	fclose(file);
}

void TableBlockStorage::filewrite(TableBlockStorageElem* tbl,int seek, bool needflush)
{
	fseek(file,seekpos+seek*(sizeof(TableBlockStorageElem)+k_size+inf_size),0);
	fwrite(tbl,(sizeof(TableBlockStorageElem)+k_size+inf_size),1,file);
	if(needflush) fflush(file);
}

bool TableBlockStorage::CopyToNextBlock(void)
{
	int i,j,full_size = sizeof(TableBlockStorageElem)+k_size+inf_size;
	TableBlockStorageElem *tbl = (TableBlockStorageElem*)malloc(full_size);

	if(max==lim-1)
	{
		if((cur_block+2)*block_size<=size)
		{
			max=-1;
			j=0;
			for(i=0;i<block_size;i++)
			{
				if(mem[i]->count>=0)
				{
					mem[i]->count=++max;
					filewrite(mem[i],(cur_block+1)*block_size+j,false);
					j++;
				}
			}
			tbl->count=-5;
			filewrite(tbl,cur_block*block_size);
			cur_block++;
		}
	}
	return true;
}

bool TableBlockStorage::Open(const char* name, int key_sz, int inf_sz, int sz, int block_num, int block_lim, int seek)
{
	if(file!=NULL) fclose(file);

	file = fopen(name, "r+");
	if(file==NULL) return false;

	seekpos=seek;
	StorageAttr *sa = new StorageAttr();
	if(fseek(file,seekpos,0)==0) fread(sa,sizeof(StorageAttr),1,file);
	else return false;

	int full_size = sizeof(TableBlockStorageElem)+key_sz+inf_sz;

	int tmpsize=(sz-sizeof(StorageAttr))/(full_size);
	int tmpblock=tmpsize/block_num;
	tmpsize=tmpblock*block_num;

	if((sa->k_size!=key_sz)||(sa->inf_size!=inf_sz)||(sa->size!=tmpsize)||(sa->block_number!=block_num)||(sa->lim!=block_lim)||(sa->seekpos!=seek)) return false;

	k_size=key_sz;
	inf_size=inf_sz;
	lim=block_lim;
	size=(sz-sizeof(StorageAttr))/(full_size);
	block_number=block_num;
	block_size=size/block_num;
	size=block_size*block_num;

	max=-1;
	int i;

	mem = new TableBlockStorageElem*[block_size];
	for(i=0;i<block_size;i++)
	{
		mem[i]=(TableBlockStorageElem*)malloc(full_size);
	}

	TableBlockStorageElem *t = (TableBlockStorageElem*)malloc(full_size);
	
	seekpos+=sizeof(StorageAttr);
	for(i=0;i<block_num;i++)
	{
		fseek(file,seekpos+i*block_size*(full_size),0);
		fread(t,(full_size),1,file);
		if(t->count>=0) 
		{
			cur_block=i;
			break;
		}
	}
	fseek(file,seekpos+(cur_block*block_size)*(full_size),0);
	for(i=0;i<block_size;i++)
	{
		fread(mem[i],(full_size),1,file);
		if(mem[i]->count>max) max=mem[i]->count;
	}
	return true;
}

bool TableBlockStorage::Create(const char* name, int key_sz, int inf_sz, int sz, int block_num, int block_lim, int seek)
{
	if(file!=NULL) fclose(file);
	file = fopen(name, "r+");
	if(file==NULL)
	{
		FILE*f=fopen(name,"w");
		fclose(f);
		file = fopen(name, "r+");
	}
	k_size=key_sz;
	inf_size=inf_sz;
	seekpos=seek;
	lim=block_lim;
	int i,full_size = sizeof(TableBlockStorageElem)+k_size+inf_size;
	TableBlockStorageElem *t = (TableBlockStorageElem*)malloc(full_size);
	size=(sz-sizeof(StorageAttr))/(full_size);

	block_number=block_num;
	block_size=size/block_num;
	size=block_size*block_num;
	max=-1;

	mem = new TableBlockStorageElem*[block_size];
	for(i=0;i<block_size;i++)
	{
		mem[i]=(TableBlockStorageElem*)malloc(full_size);
	}

	StorageAttr *sa = new StorageAttr();
	sa->k_size=k_size;
	sa->inf_size=inf_size;
	sa->size=size;
	sa->block_number=block_number;
	sa->lim=lim;
	sa->seekpos=seekpos;

	cur_block=0;
	fseek(file,seekpos,0);
	fwrite(sa,sizeof(StorageAttr),1,file);
	fflush(file);
	seekpos+=sizeof(StorageAttr);

	for(i=0;i<size;i++) 
	{
		if(i%block_size==0)
			t->count=-5;
		else t->count=-1;
		filewrite(t,i,false);
	}
	fflush(file);

	int emp = sz-size*full_size-sizeof(StorageAttr);
	if(emp>0)
	{
		char* empty=new char[emp];
		fwrite(empty,emp,1,file);
		fflush(file);
	}
	mem[0]->count=-5;
	for(i=1;i<block_size;i++)
		mem[i]->count=-1;
	return true;
}

bool TableBlockStorage::AddRow(char* key, char* value)
{
	int full_size = sizeof(TableBlockStorageElem)+k_size+inf_size;
	int i=0,pos=-1,empty=-1,k;
	if(file==NULL) return false;

	CopyToNextBlock();
	for(i=0;i<block_size;i++)
	{
		if((*((char*)mem[i]+sizeof(TableBlockStorageElem))!=0)&&(mem[i]>=0))
			if(KeyCompare((char*)mem[i],key,k_size))
				pos = i;
		if((mem[i]->count<0)&&(empty<0)) empty=i;
	}
	if(pos>=0)
	{
		mem[pos]->count=++max;
		memcpy((void*)((char*)mem[pos]+sizeof(TableBlockStorageElem)+k_size),(void*)value,inf_size);
		filewrite(mem[pos],cur_block*block_size+pos);
		return true;;
	}
	mem[empty]->count=++max;
	memcpy((void*)((char*)mem[empty]+sizeof(TableBlockStorageElem)),(void*)key,k_size);
	memcpy((void*)((char*)mem[empty]+sizeof(TableBlockStorageElem)+k_size),(void*)value,inf_size);
	filewrite(mem[empty],cur_block*block_size+empty);
	return true;
}

bool TableBlockStorage::DelRow(char* key)
{
	int full_size = sizeof(TableBlockStorageElem)+k_size+inf_size;
	int i;
	if(file==NULL) return false;

	CopyToNextBlock();
	for(i=0;i<block_size;i++)
	{
		if((*((char*)mem[i]+sizeof(TableBlockStorageElem))!=0)&&(mem[i]>=0))
			if(KeyCompare((char*)mem[i],key,k_size))
			{
				mem[i]->count=++max;
				*((char*)mem[i]+sizeof(TableBlockStorageElem))=0;
				filewrite(mem[i],cur_block*block_size+i);
				return true;
			}
	}
}

char* TableBlockStorage::FindKeyValue(char* key, char* val)
{
	int i,k;
	if(file!=NULL)
	{
		for(i=0;i<block_size;i++)
		{
			if((*((char*)mem[i]+sizeof(TableBlockStorageElem))!=0)&&(mem[i]>=0))
				if(KeyCompare((char*)mem[i],key,k_size))
				{
					memcpy((void*)val,(void*)((char*)mem[i]+sizeof(TableBlockStorageElem)+k_size),inf_size);
					return val;
				}
		}
	}
	return 0;
}

