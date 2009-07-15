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

TableStorage::TableStorage()
{
	file = fopen("tbl", "r+");
	inf_size=60;
	size=100;
	seekpos=0;
	if(file==NULL)
	{
		file = fopen("tbl","w");
		TableStorageElem *t = (TableStorageElem*)malloc(sizeof(TableStorageElem)+inf_size);
		for(int i=0;i<size;i++) fwrite(t,(sizeof(TableStorageElem)+inf_size),1,file);
		fclose(file);
		file = fopen("tbl","r+");
	}
}

TableStorage::TableStorage(const char* name, int inf_sz, int sz, int seek)
{
	file = fopen(name, "r+");
	inf_size=inf_sz;
	size=sz/(sizeof(TableStorageElem)+inf_size);
	if(file==NULL)
	{
		file = fopen(name,"w");
		TableStorageElem *t = (TableStorageElem*)malloc(sizeof(TableStorageElem)+inf_size);
		for(int i=0;i<size;i++) fwrite(t,(sizeof(TableStorageElem)+inf_size),1,file);
		fclose(file);
		file = fopen(name,"r+");
		seekpos=0;
	}
	else seekpos=seek;
}

TableStorage::~TableStorage()
{
	fclose(file);
}

int TableStorage::AddRow(char* key, char* value)
{
	TableStorageElem *tbl = (TableStorageElem*)malloc(sizeof(TableStorageElem)+inf_size);
	int i,k;
	if(file!=NULL)
	{
		fseek(file,seekpos,0);
		for(i=0;i<size;i++)
		{
			fread(tbl,(sizeof(TableStorageElem)+inf_size),1,file);
			if(!strcmp(tbl->key,key))
			{	
				for(k=0;k<inf_size;k++)
					*((char*)(tbl)+sizeof(TableStorageElem)+k)=*(value+k);
				fseek(file,seekpos+i*(sizeof(TableStorageElem)+inf_size),0);
				fwrite(tbl,(sizeof(TableStorageElem)+inf_size),1,file);
				return 0;
			}
		}
		fseek(file,seekpos,0);
		for(i=0;i<size;i++)
		{
			fread(tbl,(sizeof(TableStorageElem)+inf_size),1,file);
			if(tbl->key[0]==0)
			{
				strcpy(tbl->key,key);
				for(k=0;k<inf_size;k++)
					*((char*)(tbl)+sizeof(TableStorageElem)+k)=*(value+k);
				fseek(file,seekpos+i*(sizeof(TableStorageElem)+inf_size),0);
				fwrite(tbl,(sizeof(TableStorageElem)+inf_size),1,file);
				return 0;
			}	
		}
	}
	return 1;
}

int TableStorage::DelRow(char* key)
{
	TableStorageElem *tbl = (TableStorageElem*)malloc(sizeof(TableStorageElem)+inf_size);
	int i;
	if(file!=NULL)
	{
		fseek(file,seekpos,0);
		for(i=0;i<size;i++)
		{
			fread(tbl,(sizeof(TableStorageElem)+inf_size),1,file);
			if(!strcmp(tbl->key,key))
			{
				tbl->key[0]=0;
				fseek(file,seekpos+i*(sizeof(TableStorageElem)+inf_size),0);
				fwrite(tbl,(sizeof(TableStorageElem)+inf_size),1,file);
				return 0;
			}
		}
	}
	return 1;
}

char* TableStorage::FindKeyValue(char* key, char* val)
{
	TableStorageElem *tbl = (TableStorageElem*)malloc(sizeof(TableStorageElem)+inf_size);
	int i,k;
	if(file!=NULL)
	{
		fseek(file,seekpos,0);
		for(i=0;i<size;i++)
		{
			fread(tbl,(sizeof(TableStorageElem)+inf_size),1,file);
			if(!strcmp(tbl->key,key))
			{
				for(k=0;k<inf_size;k++)
					*(val+k)=*((char*)(tbl)+sizeof(TableStorageElem)+k);
				return val;
			}
		}
	}
	return 0;
}

