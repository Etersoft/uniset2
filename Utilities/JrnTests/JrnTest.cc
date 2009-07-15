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

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "Storages.h"

char* itoa(int val, int base)
{
	
	static char buf[32] = {0};
	
	int i = 30;
	
	for(; val && i ; --i, val /= base)
	
		buf[i] = "0123456789abcdef"[val % base];
	
	return &buf[i+1];
	
}


void testTable(void)
{
	char *chr=(char*)malloc(20);
	char *val=(char*)malloc(40);
	TableStorage *t;
	t = new TableStorage("table.test", 40, 6000, 0);
	int i;
	printf("testTable\nsize = %d\n",t->size);
	for(i=0;i<t->size+5;i++)
	{
		chr[0]=i;
		val=itoa(i,10);
		if(t->AddRow(chr,val)==1) printf("elem number %d - no space in TableStorage\n",i);
	}
	printf("elements with values=keys added\nvalues from keys 25-59\n");
	for(i=25;i<60;i++)
	{
		chr[0]=i;
		if(t->FindKeyValue(chr,val)!=0) printf("%s, ",val);
	}
	printf("\n");
	for(i=40;i<60;i++)
	{
		chr[0]=i;
		t->DelRow(chr);
	}
	printf("elements with keys from 40 to 60 deleted\n");
	for(i=30;i<50;i++)
	{
		chr[0]=i;
		val=itoa(i+40,10);
		if(t->AddRow(chr,val)==1) printf("elem number %d - no space in TableStorage\n",i);
	}
	printf("elements with keys from 30 to 50 with values=key+40 added\nvalues from keys 25-59\n");
	for(i=25;i<60;i++)
	{
		chr[0]=i;
		if(t->FindKeyValue(chr,val)!=0) printf("%s, ",val);
	}
	printf("\n");
}

void testJournal1(void)
{
	CycleStorage *j;
	int i;
	char *str = (char*)malloc(30);
	printf("journal test 1\n");
	j = new CycleStorage("journal.test",30,1000000,0);
	printf("size = %d\n",j->size);
	for(i=1;i<40000;i++)
	{
		str = itoa(i,10);
		j->AddRow(str);
	}
	printf("\n20 elements from 10-th:\n");
	j->ViewRows(10,20);
	printf("\n20 elements from 10-th after deleting first 20:\n");
	for(i=0;i<20;i++)
	{
		j->DelRow(i);
	}
	j->ViewRows(10,20);
	printf("\nfirst 20 after adding 10 elements\n");
	for(i=1;i<11;i++)
	{
		str = itoa(i,10);
		j->AddRow(str);
	}
	j->ViewRows(0,20);
	printf("\nthe same after reopen:\n");
	j = new CycleStorage("journal.test",30,2000000,0);
	j->ViewRows(0,20);
	printf("\n");
}

void testJournal2(void)
{
	CycleStorage *j;
	int i,k;
	char *str = (char*)malloc(30);
	j = new CycleStorage("journal.test",30,1000000,0);
	printf("journal test 2 - checking number of iterations to find head/tail\n");
	printf("size = %d\n\n",j->size);
	printf("iterations = %d\n",j->iter);
	for(i=0;i<20;i++)
	{
		for(k=0;k<2000;k++)
		{
			str = itoa(k,10);
			j->AddRow(str);
		}
		j = new CycleStorage("journal.test",30,2000000,0);
		printf("iterations = %d\n",j->iter);
	}
	printf("\n");
}


int main(void)
{
	testTable();

	testJournal1();
	testJournal2();

	return 0;
}