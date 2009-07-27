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

/*!	Функции класса TableBlockStorage, таблицы ключ-значение с ограниченным кол-вом перезаписей для
	каждого блока памяти, при достижении предела, происходит переход в следующий блок
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Storages.h"

#define block_begin -5
#define empty_elem -1

bool KeyCompare(void* key1, void* key2, int cnt)
{
	return !memcmp((char*)key1+sizeof(TableBlockStorageElem),key2,cnt);
}

TableBlockStorage::TableBlockStorage()
{
	file=NULL;
}

TableBlockStorage::TableBlockStorage(const char* name, int key_sz, int inf_sz, int sz, int block_num, int block_lim, int seek, bool create)
{
	file=NULL;
	if(!Open(name, key_sz, inf_sz, sz, block_num, block_lim, seek))
		if(create)
			Create(name, key_sz, inf_sz, sz, block_num, block_lim, seek);
		else
			file=NULL;
}

TableBlockStorage::~TableBlockStorage()
{
	for(int i=0;i<block_size;i++)
	{
		delete mem[i];
	}
	delete mem;

	if(file!=NULL) fclose(file);
}

void* TableBlockStorage::KeyPointer(void* pnt)
{
	return (char*)pnt+sizeof(TableBlockStorageElem);
}

void* TableBlockStorage::ValPointer(void* pnt)
{
	return (char*)pnt+sizeof(TableBlockStorageElem)+k_size;
}

void TableBlockStorage::filewrite(int seek, bool needflush)
{
	fseek(file,seekpos+(seek+cur_block*block_size)*full_size,0);
	fwrite(mem[seek],full_size,1,file);
	if(needflush) fflush(file);
}

bool TableBlockStorage::CopyToNextBlock(void)
{
	int i;
	TableBlockStorageElem *tbl = (TableBlockStorageElem*)new char[full_size];

	if(max==lim-1)
	{
		if((cur_block+2)*block_size<=size)
		{
			max=-1;;
			for(i=0;i<block_size;i++)
			{
				if(mem[i]->count>=0)
				{
					mem[i]->count=++max;
					cur_block++;
					filewrite(i,false);
					cur_block--;
				}
			}
			tbl->count=block_begin;
			fseek(file,seekpos+cur_block*block_size*full_size,0);
			fwrite(tbl,full_size,1,file);
			fflush(file);
			cur_block++;
		}
	}
	delete tbl;
	return true;
}

bool TableBlockStorage::Open(const char* name, int key_sz, int inf_sz, int sz, int block_num, int block_lim, int seek)
{
	if(file!=NULL) fclose(file);

	file = fopen(name, "r+");
	if(file==NULL) return false;

	seekpos=seek;
	StorageAttr *sa = new StorageAttr();
	if(fseek(file,seekpos,0)==0)
		fread(sa,sizeof(StorageAttr),1,file);
	else 
	{
		delete sa;
		return false;
	}

	full_size = sizeof(TableBlockStorageElem)+key_sz+inf_sz;

	int tmpsize=(sz-sizeof(StorageAttr))/(full_size);
	int tmpblock=tmpsize/block_num;
	tmpsize=tmpblock*block_num;

	if((sa->k_size!=key_sz)||(sa->inf_size!=inf_sz)||(sa->size!=tmpsize)||(sa->block_number!=block_num)||(sa->lim!=block_lim)||(sa->seekpos!=seek))
	{
		delete sa;
		return false;
	}
	delete sa;

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
		mem[i]=(TableBlockStorageElem*)new char[full_size];
	}

	TableBlockStorageElem *t = (TableBlockStorageElem*)new char[full_size];
	
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
	delete t;
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
	full_size = sizeof(TableBlockStorageElem)+k_size+inf_size;
	int i;

	size=(sz-sizeof(StorageAttr))/(full_size);

	block_number=block_num;
	block_size=size/block_num;
	size=block_size*block_num;
	max=-1;

	mem = new TableBlockStorageElem*[block_size];
	for(i=0;i<block_size;i++)
	{
		mem[i]=(TableBlockStorageElem*)new char[full_size];
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

	/*!	Поле счетчика записей при создании служит флагом на используемость блока и на пустоту ячейки записи:
		block_begin=(-5) - заполняются первые элементы каждого блока, если там другое значение, то этот блок используется, empty_elem=(-1) - все остальные пустые записи
	*/

	mem[0]->count=block_begin;
	for(i=1;i<block_size;i++)
		mem[i]->count=empty_elem;

	/*!	Цикл инициализирует все блоки в файле*/
	for(i=0;i<size;i++) 
	{
		if((i!=0)&&(i%block_size==0)) cur_block++;
		filewrite(i%block_size,false);
	}
	cur_block=0;
	fflush(file);

	int emp = sz-size*full_size-sizeof(StorageAttr);
	if(emp>0)
	{
		char* empty=new char[emp];
		fwrite(empty,emp,1,file);
		fflush(file);
	}
	delete sa;
	return true;
}

bool TableBlockStorage::AddRow(void* key, void* value)
{
	int i=0,pos=-1,empty=-1,k;
	if(file==NULL) return false;

	CopyToNextBlock();
	for(i=0;i<block_size;i++)
	{
		if(mem[i]->count>=0)
			if(KeyCompare(mem[i],key,k_size)) pos = i;
		if((mem[i]->count<0)&&(empty<0)) empty=i;
	}

	if(pos>=0) empty=pos;
	else memcpy(KeyPointer(mem[empty]),key,k_size);

	mem[empty]->count=++max;
	memcpy(ValPointer(mem[empty]),value,inf_size);
	filewrite(empty);
	return true;
}

bool TableBlockStorage::DelRow(void* key)
{
	int i;
	if(file==NULL) return false;

	/*!	При удалении счетчик перезаписей также увеличивается
	*/

	CopyToNextBlock();
	for(i=0;i<block_size;i++)
	{
		if(mem[i]->count>=0)
			if(KeyCompare(mem[i],key,k_size))
			{
				mem[i]->count=++max;
				for(int k=0;k<k_size;k++)
					*((char*)KeyPointer(mem[i])+k)=0;
				filewrite(i);
				return true;
			}
	}
}

void* TableBlockStorage::FindKeyValue(void* key, void* val)
{
	int i,k;
	if(file!=NULL)
	{
		for(i=0;i<block_size;i++)
		{
			if(mem[i]->count>=0)
				if(KeyCompare(mem[i],key,k_size))
				{
					memcpy(val,ValPointer(mem[i]),inf_size);
					return val;
				}
		}
	}
	return 0;
}

