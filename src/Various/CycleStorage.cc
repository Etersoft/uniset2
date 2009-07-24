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
 *  \version $Id: Jrn.h,v 1.0 2009/07/15 15:55:00 vpashka Exp $
 */
// --------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Storages.h"

CycleStorage::CycleStorage()
{
	file=NULL;
	size=0;
	inf_size=0;
	seekpos=0;
	head=tail=-1;
}

CycleStorage::CycleStorage(const char* name, int inf_sz, int sz, int seek)
{
	if(!Open(name,inf_sz, sz, seek))
		Create(name,inf_sz, sz, seek);
}

CycleStorage::~CycleStorage()
{
	fclose(file);
}

void CycleStorage::filewrite(CycleStorageElem* jrn,int seek,bool needflush)
{
	fseek(file,seekpos+seek*(sizeof(CycleStorageElem)+inf_size),0);
	fwrite(jrn,(sizeof(CycleStorageElem)+inf_size),1,file);
	if(needflush) fflush(file);
}

bool CycleStorage::Open(const char* name, int inf_sz, int sz, int seek)
{
	if(file!=NULL) fclose(file);
	file = fopen(name, "r+");
	if(file==NULL) return false;

	seekpos=seek;
	CycleStorageAttr *csa = new CycleStorageAttr();
	if(fseek(file,seekpos,0)==0) fread(csa,sizeof(CycleStorageAttr),1,file);
	else return false;

	if((csa->size!=((sz-sizeof(CycleStorageAttr))/(sizeof(CycleStorageElem)+inf_sz)))||(csa->inf_size!=inf_sz)||(csa->seekpos!=seek)) return false;

	size=(sz-sizeof(CycleStorageAttr))/(sizeof(CycleStorageElem)+inf_sz);
	inf_size=inf_sz;
	seekpos=seek;

	CycleStorageElem *jrn = (CycleStorageElem*)malloc(sizeof(CycleStorageElem)+inf_sz);
	int l=-1,r=size,mid;
	iter=0;
	seekpos+=sizeof(CycleStorageAttr);
	fread(jrn,(sizeof(CycleStorageElem)+inf_size),1,file);
	if(jrn->status==0)
	{
		head=-1;
		tail=-1;
	}
	else if((jrn->status==1)||(jrn->status==6))
	{
		head=0;
		while(r - l > 1)
		{
			mid = (l+r)/2;
			fseek(file,seekpos+mid*(sizeof(CycleStorageElem)+inf_size),0);
			fread(jrn,(sizeof(CycleStorageElem)+inf_size),1,file);
			iter++;
			if(jrn->status==0)
				r = mid;
			else l=mid;
		}
		if(r<size)
		{
			fseek(file,seekpos+r*(sizeof(CycleStorageElem)+inf_size),0);
			fread(jrn,(sizeof(CycleStorageElem)+inf_size),1,file);
			tail=r-1;
		}
		else tail=size-1;
	}
	else if((jrn->status==2)||(jrn->status==3))
	{
		while((jrn->status!=1)&&(jrn->status!=6)&&(r - l > 1))
		{
			mid = (l+r)/2;
			fseek(file,seekpos+mid*(sizeof(CycleStorageElem)+inf_size),0);
			fread(jrn,(sizeof(CycleStorageElem)+inf_size),1,file);
			iter++;
			if((jrn->status==2)||(jrn->status==3))
				l = mid;
			else if((jrn->status==4)||(jrn->status==5))
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
		tail=head-1;
		if(tail<0) tail=size-1;
	}
	else
	{
		while((jrn->status!=1)&&(jrn->status!=6)&&(r - l > 1))
		{
			mid = (l+r)/2;
			fseek(file,seekpos+mid*(sizeof(CycleStorageElem)+inf_size),0);
			fread(jrn,(sizeof(CycleStorageElem)+inf_size),1,file);
			iter++;
			if((jrn->status==2)||(jrn->status==3))
				r = mid;
			else if((jrn->status==4)||(jrn->status==5))
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
		tail=head-1;
		if(tail<0) tail=size-1;
	}
	return true;
}

bool CycleStorage::Create(const char* name, int inf_sz, int sz, int seek)
{
	if(file!=NULL) fclose(file);
	file = fopen(name, "r+");
	if(file==NULL)
	{
		FILE*f=fopen(name,"w");
		fclose(f);
		file = fopen(name, "r+");
	}

	CycleStorageElem *jrn = (CycleStorageElem*)malloc(sizeof(CycleStorageElem)+inf_sz);
	size=(sz-sizeof(CycleStorageAttr))/(sizeof(CycleStorageElem)+inf_sz);
	iter=0;
	inf_size=inf_sz;
	seekpos=seek;

	CycleStorageAttr *csa = new CycleStorageAttr();
	csa->inf_size=inf_size;
	csa->size=size;
	csa->seekpos=seekpos;

	fseek(file,seekpos,0);
	fwrite(csa,sizeof(CycleStorageAttr),1,file);
	fflush(file);
	seekpos+=sizeof(CycleStorageAttr);

	for(int i=0;i<size;i++)
	{
		filewrite(jrn,i,false);
	}
	fflush(file);
	
	int emp = sz-size*(sizeof(CycleStorageElem)+inf_size)-sizeof(CycleStorageAttr);
	if(emp>0)
	{
		char* empty= new char[emp];
		fwrite(empty,emp,1,file);
		fflush(file);
	}

	head=tail=-1;
	return true;
}

bool CycleStorage::AddRow(char* str)
{
	if(file==NULL) return false;
	CycleStorageElem *jrn = (CycleStorageElem*)malloc(sizeof(CycleStorageElem)+inf_size);
	int i,k;
	if(head==-1)
	{
		jrn->status=1;
		memcpy((void*)((char*)jrn+sizeof(CycleStorageElem)),(void*)str,inf_size);
		filewrite(jrn,0);
		head=0;
		tail=0;
		return true;
	}
	if(head==tail)
	{
		jrn->status=2;
		memcpy((void*)((char*)jrn+sizeof(CycleStorageElem)),(void*)str,inf_size);
		filewrite(jrn,1);
		tail=1;
		return true;
	}
	fseek(file,seekpos+tail*(sizeof(CycleStorageElem)+inf_size),0);
	fread(jrn,(sizeof(CycleStorageElem)+inf_size),1,file);
	if((jrn->status==2)||(jrn->status==3))
		i=2;
	else i=4;
	if(tail==size-1)
	{
		fseek(file,seekpos,0);
		tail=0;
		if(i==2) i=4;
		else i=2;
	}
	else tail++;
	fread(jrn,(sizeof(CycleStorageElem)+inf_size),1,file);
	if(jrn->status==0)
	{
		jrn->status=2;
		memcpy((void*)((char*)jrn+sizeof(CycleStorageElem)),(void*)str,inf_size);
		filewrite(jrn,tail);
		return true;
	}
	else
	{
		head++;
		if(head>=size) head=0;
		jrn->status=i;
		memcpy((void*)((char*)jrn+sizeof(CycleStorageElem)),(void*)str,inf_size);
		filewrite(jrn,tail);
		fseek(file,seekpos+head*(sizeof(CycleStorageElem)+inf_size),0);
		fread(jrn,(sizeof(CycleStorageElem)+inf_size),1,file);
		if((jrn->status==3)||(jrn->status==5)) jrn->status=6;
		else jrn->status=1;
		filewrite(jrn,head);
		return true;
	}
}

bool CycleStorage::DelRow(int row)
{
	CycleStorageElem *jrn = (CycleStorageElem*)malloc(sizeof(CycleStorageElem)+inf_size);
	int i=(head+row)%size,j;
	if( row >= size ) return false;
	if(file==NULL) return false;
	fseek(file,seekpos+i*(sizeof(CycleStorageElem)+inf_size),0);
	fread(jrn,(sizeof(CycleStorageElem)+inf_size),1,file);
	if(jrn->status==1)
	{
		jrn->status=6;
		filewrite(jrn,i);
		return true;
	}
	if(jrn->status==2) j=3;
	else if(jrn->status==4) j=5;
	else return false;
	jrn->status=j;
	filewrite(jrn,i);
	return true;
}

bool CycleStorage::DelAllRows()
{
	if(file==NULL) return false;
	CycleStorageElem *jrn = (CycleStorageElem*)malloc(sizeof(CycleStorageElem)+inf_size);
	int i,j;
	fseek(file,seekpos,0);

	for(i=0;i<size;i++)
	{
		fread(jrn,(sizeof(CycleStorageElem)+inf_size),1,file);
		if(jrn->status!=0)
		{
			jrn->status=0;
			filewrite(jrn,i,false);
		}
	}
	fflush(file);
	head=tail=-1;
	return true;
}

bool CycleStorage::ViewRows(int beg, int num)
{
	CycleStorageElem *jrn = (CycleStorageElem*)malloc(sizeof(CycleStorageElem)+inf_size);
	int i,j=(head+beg)%size,n=num,k;
	if(num==0) n=size;
	if(num>size) n=size;
	if(file==NULL) return false;
	fseek(file,seekpos+j*(sizeof(CycleStorageElem)+inf_size),0);
	for(i=0;i<n;i++)
	{
		if(j==size)
		{
			j=0;
			fseek(file,seekpos,0);
		}
		fread(jrn,(sizeof(CycleStorageElem)+inf_size),1,file);
		if((jrn->status!=0)&&(jrn->status!=3)&&(jrn->status!=5)&&(jrn->status!=6))
		{
			for(k=0;k<inf_size;k++)
				printf("%c",((char*)(jrn)+sizeof(CycleStorageElem))[k]);
			printf("\n");
		}
		j++;
	}
	return true;
}

bool CycleStorage::ExportToXML(const char* name)
{
	CycleStorageElem *jrn = (CycleStorageElem*)malloc(sizeof(CycleStorageElem)+inf_size);
	int i,j=head;
	if(file==NULL) return false;
	UniXML* f = new UniXML();
	f->newDoc("!");
	fseek(file,seekpos+j*(sizeof(CycleStorageElem)+inf_size),0);
	for(i=0;i<size;i++)
	{
		if(j==size)
		{
			j=0;
			fseek(file,seekpos,0);
		}
		fread(jrn,(sizeof(CycleStorageElem)+inf_size),1,file);
		if((jrn->status!=0)&&(jrn->status!=3)&&(jrn->status!=5)&&(jrn->status!=6))
		{
			f->createNext(f->cur,"!",((char*)(jrn)+sizeof(CycleStorageElem)));
		}
		j++;
	}
	f->save(name);
	return true;
}