/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Ivan Donchevskiy
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

CycleStorage::CycleStorage()
{
	CycleStorageElem *jrn = new CycleStorageElem();
	file = fopen("jrn", "r+");
	size=100;
	seekpos=0;
	int l=-1,r=size,mid;
	iter=0;
	if(file==NULL)
	{
		file = fopen("jrn","w");
		CycleStorageElem *t = new CycleStorageElem();
		for(int i=0;i<100;i++) fwrite(t,sizeof(*t),1,file);
		fclose(file);
		file = fopen("jrn","r+");
		head=-1;
		tail=-1;
	}
	else
	{
		fseek(file,seekpos,0);
		fread(jrn,sizeof(*jrn),1,file);
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
				fseek(file,seekpos+mid*sizeof(*jrn),0);
				fread(jrn,sizeof(*jrn),1,file);
				iter++;
				if(jrn->status==0)
					r = mid;
				else l=mid;
			}
			if(r<size)
			{
				fseek(file,seekpos+r*sizeof(*jrn),0);
				fread(jrn,sizeof(*jrn),1,file);
				tail=r-1;
			}
			else tail=size-1;
		}
		else if((jrn->status==2)||(jrn->status==3))
		{
			while((jrn->status!=1)&&(jrn->status!=6)&&(r - l > 1))
			{
				mid = (l+r)/2;
				fseek(file,seekpos+mid*sizeof(*jrn),0);
				fread(jrn,sizeof(*jrn),1,file);
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
				fseek(file,seekpos+mid*sizeof(*jrn),0);
				fread(jrn,sizeof(*jrn),1,file);
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
	}
}

CycleStorage::CycleStorage(const char* name, int sz, int seek)
{
	CycleStorageElem *jrn = new CycleStorageElem();
	file = fopen(name, "r+");
	size=sz/sizeof(CycleStorageElem);
	int l=-1,r=size,mid;
	iter=0;
	if(file==NULL)
	{
		file = fopen(name,"w");
		CycleStorageElem *t = new CycleStorageElem();
		for(int i=0;i<size;i++) fwrite(t,sizeof(*t),1,file);
		fclose(file);
		file = fopen(name,"r+");
		seekpos=0;
		head=-1;
		tail=-1;
	}
	else 
	{
		seekpos=seek;
		fseek(file,seekpos,0);
		fread(jrn,sizeof(*jrn),1,file);
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
				fseek(file,seekpos+mid*sizeof(*jrn),0);
				fread(jrn,sizeof(*jrn),1,file);
				iter++;
				if(jrn->status==0)
					r = mid;
				else l=mid;
			}
			if(r<size)
			{
				fseek(file,seekpos+r*sizeof(*jrn),0);
				fread(jrn,sizeof(*jrn),1,file);
				tail=r-1;
			}
			else tail=size-1;
		}
		else if((jrn->status==2)||(jrn->status==3))
		{
			while((jrn->status!=1)&&(jrn->status!=6)&&(r - l > 1))
			{
				mid = (l+r)/2;
				fseek(file,seekpos+mid*sizeof(*jrn),0);
				fread(jrn,sizeof(*jrn),1,file);
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
				fseek(file,seekpos+mid*sizeof(*jrn),0);
				fread(jrn,sizeof(*jrn),1,file);
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
	}
}

CycleStorage::~CycleStorage()
{
	fclose(file);
}

int CycleStorage::AddRow(char* str)
{
	CycleStorageElem *jrn = new CycleStorageElem();
	int i;
	if(file!=NULL)
	{
		if(head==-1)
		{
			fseek(file,seekpos,0);
			strcpy(jrn->str,str);
			jrn->status=1;
			fwrite(jrn,sizeof(*jrn),1,file);
			head=0;
			tail=0;
			return 0;
		}
		if(head==tail)
		{
			fseek(file,seekpos+sizeof(*jrn),0);
			strcpy(jrn->str,str);
			jrn->status=2;
			fwrite(jrn,sizeof(*jrn),1,file);
			tail=1;
			return 0;
		}
		fseek(file,seekpos+tail*sizeof(*jrn),0);
		fread(jrn,sizeof(*jrn),1,file);
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
		fread(jrn,sizeof(*jrn),1,file);
		if(jrn->status==0)
		{
			fseek(file,seekpos+tail*sizeof(*jrn),0);
			strcpy(jrn->str,str);
			jrn->status=2;
			fwrite(jrn,sizeof(*jrn),1,file);
			return 0;
		}
		else
		{
			head++;
			if(head>=size) head=0;
			fseek(file,seekpos+tail*sizeof(*jrn),0);
			strcpy(jrn->str,str);
			jrn->status=i;
			if(tail==0) fseek(file,seekpos,0);
			fwrite(jrn,sizeof(*jrn),1,file);
			fseek(file,seekpos+head*sizeof(*jrn),0);
			fread(jrn,sizeof(*jrn),1,file);
			if((jrn->status==3)||(jrn->status==5)) jrn->status=6;
			else jrn->status=1;
			fseek(file,seekpos+head*sizeof(*jrn),0);
			fwrite(jrn,sizeof(*jrn),1,file);
			return 0;
		}
	}
	return 1;
}

int CycleStorage::DelRow(int row)
{
	CycleStorageElem *jrn = new CycleStorageElem(); 
	int i=(head+row)%size,j;
	if( row >= size ) return 1;
	if(file!=NULL)
	{
		fseek(file,seekpos+i*sizeof(*jrn),0);
		fread(jrn,sizeof(*jrn),1,file);
		if(jrn->status==1)
		{
			jrn->status=6;
			fseek(file,seekpos+i*sizeof(*jrn),0);
			fwrite(jrn,sizeof(*jrn),1,file);
			return 0;
		}
		if(jrn->status==2) j=3;
		else if(jrn->status==4) j=5;
		else return 1;
		fseek(file,seekpos+i*sizeof(*jrn),0);
		jrn->status=j;
		fwrite(jrn,sizeof(*jrn),1,file);
	}
	return 1;
}

int CycleStorage::DelAllRows()
{
	CycleStorageElem *jrn = new CycleStorageElem(); 
	int i;
	if(file!=NULL)
	{
		fseek(file,seekpos,0);
		if((tail>head)&&(tail!=size-1))
		{
			i=1;
			while(i!=0)
			{
				fread(jrn,sizeof(*jrn),1,file);
				jrn->status=0;
				fseek(file,seekpos+i*sizeof(*jrn),0);
				fwrite(jrn,sizeof(*jrn),1,file);
				i=jrn->status;
			}
		}
		for(i=0;i<size;i++)
		{
			fread(jrn,sizeof(*jrn),1,file);
			if(jrn->status!=0)
			{
				jrn->status=0;
				fseek(file,seekpos+i*sizeof(*jrn),0);
				fwrite(jrn,sizeof(*jrn),1,file);
			}
		}
	}
	return 1;
}

int CycleStorage::ViewRows(int beg, int num)
{
	CycleStorageElem *jrn = new CycleStorageElem(); 
	int i,j=(head+beg)%size,n=num;
	if(num==0) n=size;
	if(num>size) n=size;
	if(file!=NULL)
	{
		fseek(file,seekpos+j*sizeof(*jrn),0);
		for(i=0;i<n;i++)
		{
			if(j==size)
			{
				j=0;
				fseek(file,seekpos,0);
			}
			fread(jrn,sizeof(*jrn),1,file);
			if((jrn->status!=0)&&(jrn->status!=3)&&(jrn->status!=5)&&(jrn->status!=6))
			{
				printf("%s\n",jrn->str);
			}
			j++;
		}
		return 0;
	}
	return 1;
}

