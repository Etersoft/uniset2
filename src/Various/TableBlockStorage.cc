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
	block_size=0;
}

TableBlockStorage::TableBlockStorage(const char* name, int key_sz, int inf_sz, int sz, int block_num, int block_lim)
{
	file = fopen(name, "r+");
	k_size=key_sz;
	inf_size=inf_sz;
	int i, full_size = sizeof(TableBlockStorageElem)+k_size+inf_size;
	max=-1;
	lim=block_lim;
	size=sz/(full_size);
	block_size=size/block_num;
	size=block_size*block_num;
	TableBlockStorageElem *t = (TableBlockStorageElem*)malloc(full_size);
	for(int k=0;k<k_size+inf_size;k++)
		*((char*)(t)+sizeof(TableBlockStorageElem)+k)=0;
	mem = new TableBlockStorageElem*[block_size];
	for(i=0;i<block_size;i++)
	{
		mem[i]=(TableBlockStorageElem*)malloc(full_size);
	}
	if(file==NULL)
	{
		file = fopen(name,"w");
		cur_block=0;
		for(i=0;i<size;i++) 
		{
			if(i%block_size==0)
				t->count=-5;
			else t->count=-1;
			fwrite(t,(full_size),1,file);
		}
		mem[0]->count=-5;
		for(i=1;i<block_size;i++)
			mem[i]->count=-1;
		fclose(file);
		file = fopen(name,"r+");
	}
	else
	{
		for(i=0;i<block_num;i++)
		{
			fseek(file,i*block_size*(full_size),0);
			fread(t,(full_size),1,file);
			if(t->count>=0) 
			{
				cur_block=i;
				break;
			}
		}
		fseek(file,(cur_block*block_size)*(full_size),0);
		for(i=0;i<block_size;i++)
		{
			fread(mem[i],(full_size),1,file);
			if(mem[i]->count>max) max=mem[i]->count;
		}
	}
}

TableBlockStorage::~TableBlockStorage()
{
	fclose(file);
}

int TableBlockStorage::Open(const char* name, int key_sz, int inf_sz, int sz, int block_num, int block_lim)
{
	file = fopen(name, "r+");
	k_size=key_sz;
	inf_size=inf_sz;
	int i,full_size = sizeof(TableBlockStorageElem)+k_size+inf_size;
	max=-1;
	lim=block_lim;
	size=sz/(full_size);
	block_size=size/block_num;
	size=block_size*block_num;
	TableBlockStorageElem *t = (TableBlockStorageElem*)malloc(full_size);
	for(int k=0;k<k_size+inf_size;k++)
		*((char*)(t)+sizeof(TableBlockStorageElem)+k)=0;
	mem = new TableBlockStorageElem*[block_size];
	for(i=0;i<block_size;i++)
	{
		mem[i]=(TableBlockStorageElem*)malloc(full_size);
	}
	if(file==NULL)
	{
		return 1;
	}
	else
	{
		for(i=0;i<block_num;i++)
		{
			fseek(file,i*block_size*(full_size),0);
			fread(t,(full_size),1,file);
			if(t->count>=0) 
			{
				cur_block=i;
				break;
			}
		}
		fseek(file,(cur_block*block_size)*(full_size),0);
		for(i=0;i<block_size;i++)
		{
			fread(mem[i],(full_size),1,file);
			if(mem[i]->count>max) max=mem[i]->count;
		}
	}
	return 0;
}

int TableBlockStorage::AddRow(char* key, char* value)
{
	int full_size = sizeof(TableBlockStorageElem)+k_size+inf_size;
	TableBlockStorageElem *tbl = (TableBlockStorageElem*)malloc(full_size);
	int i=0,pos=-1,empty=-1,k,j;
	if(file!=NULL)
	{
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
						fseek(file,((cur_block+1)*block_size+j)*(full_size),0);
						fwrite(mem[i],(full_size),1,file);
						j++;
					}
				}
				/*fseek(file,((cur_block+1)*block_size+j)*(full_size),0);
				mem[j]->count=++max;
				for(k=0;k<k_size;k++)
					*((char*)mem[j]+sizeof(TableBlockStorageElem)+k)=*(key+k);
				for(k=0;k<inf_size;k++)
					*((char*)mem[j]+sizeof(TableBlockStorageElem)+k_size+k)=*(value+k);
				fwrite(mem[j],(sizeof(TableBlockStorageElem)+inf_size),1,file);*/
				tbl->count=-5;
				fseek(file,cur_block*block_size*(full_size),0);
				fwrite(tbl,(full_size),1,file);
				cur_block++;
				//return 0;
			}
		}
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
			for(k=0;k<inf_size;k++)
				*((char*)mem[pos]+sizeof(TableBlockStorageElem)+k_size+k)=*(value+k);
			fseek(file,(cur_block*block_size+pos)*(full_size),0);
			fwrite(mem[pos],(full_size),1,file);
			return 0;
		}
		mem[empty]->count=++max;
		for(k=0;k<k_size;k++)
			*((char*)mem[empty]+sizeof(TableBlockStorageElem)+k)=*(key+k);
		for(k=0;k<inf_size;k++)
			*((char*)mem[empty]+sizeof(TableBlockStorageElem)+k_size+k)=*(value+k);
		fseek(file,(cur_block*block_size+empty)*(full_size),0);
		fwrite(mem[empty],(full_size),1,file);
		return 0;
	}
	return 1;
}

int TableBlockStorage::DelRow(char* key)
{
	int full_size = sizeof(TableBlockStorageElem)+k_size+inf_size;
	TableBlockStorageElem *tbl = (TableBlockStorageElem*)malloc(full_size);
	int i,j;
	if(file!=NULL)
	{
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
						fseek(file,((cur_block+1)*block_size+j)*(full_size),0);
						fwrite(mem[i],(full_size),1,file);
						j++;
					}
				}
				tbl->count=-5;
				fseek(file,cur_block*block_size*(full_size),0);
				fwrite(tbl,(full_size),1,file);
				cur_block++;
			}
		}
		for(i=0;i<block_size;i++)
		{
			if((*((char*)mem[i]+sizeof(TableBlockStorageElem))!=0)&&(mem[i]>=0))
				if(KeyCompare((char*)mem[i],key,k_size))
				{
					fseek(file,(cur_block*block_size+i)*(full_size),0);
					mem[i]->count=++max;
					*((char*)mem[i]+sizeof(TableBlockStorageElem))=0;
					fwrite(mem[i],(full_size),1,file);
					return 0;
				}
		}
	}
	return 1;
}

char* TableBlockStorage::FindKeyValue(char* key, char* val)
{
	int i,k;
	if(file!=NULL)
	{
		//fseek(file,cur_block*block_size*(full_size),0);
		for(i=0;i<block_size;i++)
		{
			//fread(tbl,(full_size),1,file);
			if((*((char*)mem[i]+sizeof(TableBlockStorageElem))!=0)&&(mem[i]>=0))
				if(KeyCompare((char*)mem[i],key,k_size))
				{
					for(k=0;k<inf_size;k++)
						*(val+k)=*((char*)mem[i]+sizeof(TableBlockStorageElem)+k_size+k);
					return val;
				}
		}
	}
	return 0;
}

