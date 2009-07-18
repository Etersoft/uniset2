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


TableBlockStorage::TableBlockStorage(const char* name, int inf_sz, int sz, int block_num, int block_lim)
{
	file = fopen(name, "r+");
	inf_size=inf_sz;
	int i;
	max=-1;
	lim=block_lim;
	size=sz/(sizeof(TableBlockStorageElem)+inf_size);
	block_size=size/block_num;
	size=block_size*block_num;
	TableBlockStorageElem *t = (TableBlockStorageElem*)malloc(sizeof(TableBlockStorageElem)+inf_size);
	for(int k=0;k<inf_size;k++)
		*((char*)(t)+sizeof(TableBlockStorageElem)+k)=0;
	if(file==NULL)
	{
		file = fopen(name,"w");
		cur_block=0;
		for(i=0;i<size;i++) 
		{
			if(i%block_size==0)
				t->count=-5;
			else t->count=-1;
			fwrite(t,(sizeof(TableBlockStorageElem)+inf_size),1,file);
		}
		fclose(file);
		file = fopen(name,"r+");
	}
	else
	{
		for(i=0;i<block_num;i++)
		{
			fseek(file,i*block_size*(sizeof(TableBlockStorageElem)+inf_size),0);
			fread(t,(sizeof(TableBlockStorageElem)+inf_size),1,file);
			if(t->count>=0) 
			{
				cur_block=i;
				break;
			}
		}
		fseek(file,(cur_block*block_size)*(sizeof(TableBlockStorageElem)+inf_size),0);
		for(i=0;i<block_size;i++);
		{
			fread(t,(sizeof(TableBlockStorageElem)+inf_size),1,file);
			if(t->count>max) max=t->count;
		}
	}
}

TableBlockStorage::~TableBlockStorage()
{
	fclose(file);
}

int TableBlockStorage::AddRow(char* key, char* value)
{
	TableBlockStorageElem *tbl = (TableBlockStorageElem*)malloc(sizeof(TableBlockStorageElem)+inf_size);
	int i=0,pos=-1,empty=-1,k,j;
	if(file!=NULL)
	{
		fseek(file,cur_block*block_size*(sizeof(TableBlockStorageElem)+inf_size),0);
		if(max==lim-1)
		{
			if((cur_block+2)*block_size<size)
			{
				max=-1;
				j=0;
				for(i=0;i<block_size;i++)
				{
					fseek(file,(cur_block*block_size+i)*(sizeof(TableBlockStorageElem)+inf_size),0);
					fread(tbl,(sizeof(TableBlockStorageElem)+inf_size),1,file);
					if(tbl->count>=0)
					{
						tbl->count=++max;
						fseek(file,((cur_block+1)*block_size+j)*(sizeof(TableBlockStorageElem)+inf_size),0);
						fwrite(tbl,(sizeof(TableBlockStorageElem)+inf_size),1,file);
						j++;
					}
				}
				fseek(file,((cur_block+1)*block_size+j)*(sizeof(TableBlockStorageElem)+inf_size),0);
				tbl->count=++max;
				strcpy(tbl->key,key);
				for(k=0;k<inf_size;k++)
					*((char*)(tbl)+sizeof(TableBlockStorageElem)+k)=*(value+k);
				fwrite(tbl,(sizeof(TableBlockStorageElem)+inf_size),1,file);
				tbl->count=-5;
				fseek(file,cur_block*block_size*(sizeof(TableBlockStorageElem)+inf_size),0);
				fwrite(tbl,(sizeof(TableBlockStorageElem)+inf_size),1,file);
				cur_block++;
				return 0;
			}
		}
		for(i=0;i<block_size;i++)
		{
			fread(tbl,(sizeof(TableBlockStorageElem)+inf_size),1,file);
			if((tbl->key[0]!=0)&&(tbl->count>=0))
				if(!strcmp(tbl->key,key))
					pos = i;
			if((tbl->count<0)&&(empty<0)) empty=i;
		}
		if(pos>=0)
		{
			fseek(file,(cur_block*block_size+pos)*(sizeof(TableBlockStorageElem)+inf_size),0);
			fread(tbl,(sizeof(TableBlockStorageElem)+inf_size),1,file);
			tbl->count=++max;
			for(k=0;k<inf_size;k++)
				*((char*)(tbl)+sizeof(TableBlockStorageElem)+k)=*(value+k);
			fseek(file,(cur_block*block_size+pos)*(sizeof(TableBlockStorageElem)+inf_size),0);
			fwrite(tbl,(sizeof(TableBlockStorageElem)+inf_size),1,file);
			return 0;
		}
		fseek(file,(cur_block*block_size+empty)*(sizeof(TableBlockStorageElem)+inf_size),0);
		fread(tbl,(sizeof(TableBlockStorageElem)+inf_size),1,file);
		tbl->count=++max;
		strcpy(tbl->key,key);
		for(k=0;k<inf_size;k++)
			*((char*)(tbl)+sizeof(TableBlockStorageElem)+k)=*(value+k);
		fseek(file,(cur_block*block_size+empty)*(sizeof(TableBlockStorageElem)+inf_size),0);
		fwrite(tbl,(sizeof(TableBlockStorageElem)+inf_size),1,file);
		return 0;
	}
	return 1;
}

int TableBlockStorage::DelRow(char* key)
{
	TableBlockStorageElem *tbl = (TableBlockStorageElem*)malloc(sizeof(TableBlockStorageElem)+inf_size);
	int i,j;
	if(file!=NULL)
	{
		fseek(file,cur_block*block_size*(sizeof(TableBlockStorageElem)+inf_size),0);
		if(max==lim-1)
		{
			if((cur_block+2)*block_size<size)
			{
				max=-1;
				j=0;
				for(i=0;i<block_size;i++)
				{
					fseek(file,(cur_block*block_size+i)*(sizeof(TableBlockStorageElem)+inf_size),0);
					fread(tbl,(sizeof(TableBlockStorageElem)+inf_size),1,file);
					if(tbl->count>=0)
					{
						tbl->count=++max;
						fseek(file,((cur_block+1)*block_size+j)*(sizeof(TableBlockStorageElem)+inf_size),0);
						fwrite(tbl,(sizeof(TableBlockStorageElem)+inf_size),1,file);
						j++;
					}
				}
				tbl->count=-5;
				fseek(file,cur_block*block_size*(sizeof(TableBlockStorageElem)+inf_size),0);
				fwrite(tbl,(sizeof(TableBlockStorageElem)+inf_size),1,file);
				cur_block++;
				fseek(file,cur_block*block_size*(sizeof(TableBlockStorageElem)+inf_size),0);
			}
		}
		for(i=0;i<block_size;i++)
		{
			fread(tbl,(sizeof(TableBlockStorageElem)+inf_size),1,file);
			if((tbl->key[0]!=0)&&(tbl->count>=0))
				if(!strcmp(tbl->key,key))
				{
					fseek(file,(cur_block*block_size+i)*(sizeof(TableBlockStorageElem)+inf_size),0);
					tbl->count=++max;
					tbl->key[0]=0;
					fwrite(tbl,(sizeof(TableBlockStorageElem)+inf_size),1,file);
					return 0;
				}
		}
	}
	return 1;
}

char* TableBlockStorage::FindKeyValue(char* key, char* val)
{
	TableBlockStorageElem *tbl = (TableBlockStorageElem*)malloc(sizeof(TableBlockStorageElem)+inf_size);
	int i,k;
	if(file!=NULL)
	{
		fseek(file,cur_block*block_size*(sizeof(TableBlockStorageElem)+inf_size),0);
		for(i=0;i<block_size;i++)
		{
			fread(tbl,(sizeof(TableBlockStorageElem)+inf_size),1,file);
			if((tbl->key[0]!=0)&&(tbl->count>=0))
				if(!strcmp(tbl->key,key))
				{
					for(k=0;k<inf_size;k++)
						*(val+k)=*((char*)(tbl)+sizeof(TableBlockStorageElem)+k);
					return val;
				}
		}
	}
	return 0;
}

