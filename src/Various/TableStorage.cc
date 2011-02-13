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
 */
// --------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Storages.h"


TableStorage::TableStorage(const char* name, int inf_sz, int sz, int seek)
{
	file = fopen(name, "r+");
	inf_size=inf_sz;
	int l=-1,r=size,mid;
	size=sz/(sizeof(TableStorageElem)+inf_size);
	TableStorageElem *t = (TableStorageElem*)malloc(sizeof(TableStorageElem)+inf_size);
	if(file==NULL)
	{
		file = fopen(name,"w");
		for(int i=0;i<size;i++) fwrite(t,(sizeof(TableStorageElem)+inf_size),1,file);
		fclose(file);
		file = fopen(name,"r+");
		seekpos=0;
		head=-1;
	}
	else
	{
		seekpos=seek;
		fseek(file,seekpos,0);
		fread(t,(sizeof(TableStorageElem)+inf_size),1,file);
		if(t->status==0)
		{
			head=-1;
		}
		else if((t->status==1)||(t->status==6))
		{
			head=0;
		}
		else if((t->status==2)||(t->status==3))
		{
			while((t->status!=1)&&(t->status!=6)&&(r - l > 1))
			{
				mid = (l+r)/2;
				fseek(file,seekpos+mid*(sizeof(TableStorageElem)+inf_size),0);
				fread(t,(sizeof(TableStorageElem)+inf_size),1,file);
				if((t->status==2)||(t->status==3))
					l = mid;
				else if((t->status==4)||(t->status==5))
					r = mid;
				else
				{
					r=mid;
					break;
				}
			}
			if(r<size)
				head=r;
			else head=size-1;
		}
		else
		{
			while((t->status!=1)&&(t->status!=6)&&(r - l > 1))
			{
				mid = (l+r)/2;
				fseek(file,seekpos+mid*(sizeof(TableStorageElem)+inf_size),0);
				fread(t,(sizeof(TableStorageElem)+inf_size),1,file);
				if((t->status==2)||(t->status==3))
					r = mid;
				else if((t->status==4)||(t->status==5))
					l = mid;
				else
				{
					r=mid;
					break;
				}
			}
			if(r<size)
				head=r;
			else head=size-1;
		}
	}
}

TableStorage::~TableStorage()
{
	fclose(file);
}

int TableStorage::addRow(char* key, char* value)
{
	TableStorageElem *tbl = (TableStorageElem*)malloc(sizeof(TableStorageElem)+inf_size);
	int i,k,j,st;
	if(file!=NULL)
	{
		if(head==-1)
		{
			fseek(file,seekpos,0);
			tbl->status=1;
			strcpy(tbl->key,key);
			for(k=0;k<inf_size;k++)
				*((char*)(tbl)+sizeof(TableStorageElem)+k)=*(value+k);
			fwrite(tbl,(sizeof(TableStorageElem)+inf_size),1,file);
			head=0;
			return 0;
		}
		fseek(file,seekpos+head*(sizeof(TableStorageElem)+inf_size),0);
		j=head;
		for(i=0;i<size;i++)
		{
			fread(tbl,(sizeof(TableStorageElem)+inf_size),1,file);
			if(tbl->status==0) break;
			if(!strcmp(tbl->key,key)&&((tbl->status==2)||(tbl->status==4)||(tbl->status==1)))
			{	
				for(k=0;k<inf_size;k++)
					*((char*)(tbl)+sizeof(TableStorageElem)+k)=*(value+k);
				fseek(file,seekpos+i*(sizeof(TableStorageElem)+inf_size),0);
				fwrite(tbl,(sizeof(TableStorageElem)+inf_size),1,file);
				return 0;
			}
			j++;
			if(j>=size)
			{
				j=0;
				fseek(file,seekpos,0);
			}
		}
		fseek(file,seekpos+j*(sizeof(TableStorageElem)+inf_size),0);
		if(j==head)
		{
			if((tbl->status==2)||(tbl->status==3)) st=2;
			else st=4;

			if(j==0) {
				if(st==2) st=4;
				else st=2;
			}

			tbl->status=st;
			strcpy(tbl->key,key);
			for(k=0;k<inf_size;k++)
				*((char*)(tbl)+sizeof(TableStorageElem)+k)=*(value+k);
			fwrite(tbl,(sizeof(TableStorageElem)+inf_size),1,file);
			j++;
			if(j>=size)
			{
				j=0;
				fseek(file,seekpos,0);
			}

			fread(tbl,(sizeof(TableStorageElem)+inf_size),1,file);
			if((tbl->status==3)||(tbl->status==5)) tbl->status=6;
			else tbl->status=1;
			fseek(file,seekpos+j*(sizeof(TableStorageElem)+inf_size),0);
			fwrite(tbl,(sizeof(TableStorageElem)+inf_size),1,file);
			head++;
			if(head>=size) head=0;
			return 0;
		}
	}
	return 1;
}

int TableStorage::delRow(char* key)
{
	TableStorageElem *tbl = (TableStorageElem*)malloc(sizeof(TableStorageElem)+inf_size);
	int i,j;
	if(file!=NULL)
	{
		fseek(file,seekpos+head*(sizeof(TableStorageElem)+inf_size),0);
		j=head;
		for(i=0;i<size;i++)
		{
			fread(tbl,(sizeof(TableStorageElem)+inf_size),1,file);
			if(!strcmp(tbl->key,key)&&((tbl->status==2)||(tbl->status==4)||(tbl->status==1)))
			{
				//tbl->key[0]=0;
				if(tbl->status==1) tbl->status=6;
				else if(tbl->status==2) tbl->status=3;
				else tbl->status=5;
				fseek(file,seekpos+j*(sizeof(TableStorageElem)+inf_size),0);
				fwrite(tbl,(sizeof(TableStorageElem)+inf_size),1,file);
				return 0;
			}
			j++;
			if(j>=size)
			{
				j=0;
				fseek(file,seekpos,0);
			}
		}
	}
	return 1;
}

char* TableStorage::findKeyValue(char* key, char* val)
{
	TableStorageElem *tbl = (TableStorageElem*)malloc(sizeof(TableStorageElem)+inf_size);
	int i,k;
	if(file!=NULL)
	{
		fseek(file,seekpos,0);
		for(i=0;i<size;i++)
		{
			fread(tbl,(sizeof(TableStorageElem)+inf_size),1,file);
			if(!strcmp(tbl->key,key)&&((tbl->status==2)||(tbl->status==4)||(tbl->status==1)))
			{
				for(k=0;k<inf_size;k++)
					*(val+k)=*((char*)(tbl)+sizeof(TableStorageElem)+k);
				return val;
			}
		}
	}
	return 0;
}

