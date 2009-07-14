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
 *  \date   $Date: 2009/07/14 16:59:00 $
 *  \version $Id: Jrn.h,v 1.0 2009/07/14 16:59:00 vpashka Exp $
 */
// --------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>

#include "Storages.h"

TableStorage::TableStorage()
{
	file = fopen("tbl", "r+");
	if(file==NULL)
	{
		file = fopen("tbl","w");
		TableStorageElem *t = new TableStorageElem();
		for(int i=0;i<100;i++) fwrite(t,sizeof(*t),1,file);
		fclose(file);
		file = fopen("tbl","r+");
	}
	size=100;
	seekpos=0;
}

TableStorage::TableStorage(const char* name, int sz, int seek)
{
	file = fopen(name, "r+");
	size=sz/sizeof(TableStorageElem);
	if(file==NULL)
	{
		file = fopen(name,"w");
		TableStorageElem *t = new TableStorageElem();
		for(int i=0;i<size;i++) fwrite(t,sizeof(*t),1,file);
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
	TableStorageElem *tbl = new TableStorageElem();
	int i;
	if(file!=NULL)
	{
		fseek(file,seekpos,0);
		for(i=0;i<size;i++)
		{
			fread(tbl,sizeof(*tbl),1,file);
			if(!strcmp(tbl->key,key))
			{
				strcpy(tbl->value,value);
				fseek(file,seekpos+i*sizeof(*tbl),0);
				fwrite(tbl,sizeof(*tbl),1,file);
				return 0;
			}
		}
		fseek(file,seekpos,0);
		for(i=0;i<size;i++)
		{
			fread(tbl,sizeof(*tbl),1,file);
			if(*(tbl->key)==0)
			{
				strcpy(tbl->key,key);
				strcpy(tbl->value,value);
				fseek(file,seekpos+i*sizeof(*tbl),0);
				fwrite(tbl,sizeof(*tbl),1,file);
				return 0;
			}	
		}
	}
	return 1;
}

int TableStorage::DelRow(char* key)
{
	TableStorageElem *tbl = new TableStorageElem();
	int i;
	if(file!=NULL)
	{
		fseek(file,seekpos,0);
		for(i=0;i<size;i++)
		{
			fread(tbl,(key_size+val_size),1,file);
			if(!strcmp(tbl->key,key))
			{
				tbl->key[0]=0;
				fseek(file,seekpos+i*(key_size+val_size),0);
				fwrite(tbl,(key_size+val_size),1,file);
				return 0;
			}
		}
	}
	return 1;
}

char* TableStorage::FindKeyValue(char* key, char* val)
{
	TableStorageElem *tbl = new TableStorageElem();
	int i;
	if(file!=NULL)
	{
		fseek(file,seekpos,0);
		for(i=0;i<size;i++)
		{
			fread(tbl,(key_size+val_size),1,file);
			if(!strcmp(tbl->key,key))
			{
				strcpy(val,tbl->value);
				return val;
			}
		}
	}
	return 0;
}

