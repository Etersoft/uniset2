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
#include "UniXML.h"

void testTable1(void)
{
	char *chr=(char*)malloc(20);
	char *val=(char*)malloc(40);
	TableStorage *t;
	t = new TableStorage("table.test", 40, 1220, 0);
	int i;
	printf("testTable\nsize = %d\n",t->size);
	for(i=0;i<t->size;i++)
	{
		chr[0]=i;
		sprintf(val,"%d",i);
		t->AddRow(chr,val);
	}
	printf("elements with values=keys added:\n");
	for(i=0;i<40;i++)
	{
		chr[0]=i;
		if(t->FindKeyValue(chr,val)!=0) printf("%s, ",val);
	}
	printf("\n");
	for(i=9;i<15;i++)
	{
		chr[0]=i;
		t->DelRow(chr);
	}
	printf("elements with keys from 9 to 14 deleted\n");
	for(i=9;i<15;i++)
	{
		chr[0]=i;
		sprintf(val,"%d",i+40);
		t->AddRow(chr,val);
	}
	printf("elements with keys from 9 to 14 with values=key+40 added, all elements:\n");
	for(i=0;i<40;i++)
	{
		chr[0]=i;
		if(t->FindKeyValue(chr,val)!=0) printf("%s, ",val);
	}
	printf("\n");
}

void testTable2(void)
{
	char *val=new char[40];
	TableBlockStorage *t;
	t = new TableBlockStorage("big_file.test", 4, 40, 20000, 5,28,0,true);
	int i;
	for(i=1;i<20;i++)
	{
		if(t->FindKeyValue(&i,val)!=0) printf("%s, ",val);
	}
	printf("\ncurrent block = %d\n",t->cur_block);
	for(i=1;i<11;i++)
	{
		sprintf(val,"%d",i);
		t->AddRow((char*)&i,val);
	}
	printf("current block = %d, elements with values=keys added:\n",t->cur_block);
	for(i=1;i<20;i++)
	{
		if(t->FindKeyValue(&i,val)!=0) printf("%s, ",val);
	}
	printf("\ncurrent block = %d, rewriting first 7 with values=keys+10\n",t->cur_block);
	for(i=1;i<8;i++)
	{
		sprintf(val,"%d",i+10);
		t->AddRow(&i,val);
	}
	printf("deleteing 8-10 elements\n");
	for(i=8;i<11;i++)
	{
		t->DelRow(&i);
	}
	for(i=1;i<20;i++)
	{
		if(t->FindKeyValue(&i,val)!=0) printf("%s, ",val);
	}
	printf("\ncurrent block = %d, rewriting 3-10 elements with values=keys+40\n",t->cur_block);
	for(i=3;i<11;i++)
	{
		sprintf(val,"%d",i+40);
		t->AddRow(&i,val);
	}
	for(i=1;i<20;i++)
	{
		if(t->FindKeyValue(&i,val)!=0) printf("%s, ",val);
	}
	printf("\ncurrent block = %d\n",t->cur_block);

	strcpy(val,"new block");
	i=9;
	t->AddRow(&i,val);
	for(i=1;i<20;i++)
	{
		if(t->FindKeyValue((char*)&i,val)!=0) printf("%s, ",val);
	}
	printf("\ncurrent block = %d\n",t->cur_block);
	printf("after reopen:\n");
	t->Open("big_file.test", 4, 40, 20000, 5,28,0);
	for(i=1;i<20;i++)
	{
		if(t->FindKeyValue(&i,val)!=0) printf("%s, ",val);
	}
	printf("\ncurrent block = %d\n",t->cur_block);
	delete t;
}

void testJournal1(void)
{
	CycleStorage *j;
	int i;
	char *str = new char[30];
	printf("journal test 1\n");
	j = new CycleStorage("big_file.test",30,1000000,20000);
	printf("size = %d\n",j->size);
	for(i=1;i<33000;i++)
	{
		sprintf(str,"%d",i);
		j->AddRow(str);
	}
	printf("\nfirst 30 elements:\n");
	j->ViewRows(0,30);

	printf("test of 2 classes working in 1 file together\n");
	TableBlockStorage *t = new TableBlockStorage("big_file.test", 4, 40, 20000, 5,28,0);
	char *val = new char[40];
	for(i=1;i<20;i++)
	{
		if(t->FindKeyValue((char*)&i,val)!=0) printf("%s, ",val);
	}
	printf("\ncurrent block = %d\n\n",t->cur_block);

	printf("\nfirst 30 elements after deleting first 20:\n");
	for(i=0;i<20;i++)
	{
		j->DelRow(i);
	}
	j->ViewRows(0,30);
	printf("\nfirst 20 after adding 10 elements\n");
	for(i=10001;i<10011;i++)
	{
		sprintf(str,"%d",i);
		j->AddRow(str);
	}
	j->ViewRows(0,20);
	printf("\nthe same after reopen:\n");
	delete j;
	j = new CycleStorage();
	j->Open("big_file.test",30,1000000,20000);
	j->ViewRows(0,20);
	printf("\n");
	j->ExportToXML("Xml.xml");
	delete t;
	delete j;
}

void testJournal2(void)
{
	CycleStorage *j;
	int i,k;
	char *str = (char*)malloc(30);
	j = new CycleStorage("big_file.test",30,1000000,20000);
	printf("journal test 2 - checking number of iterations to find head/tail\n");
	printf("size = %d\n\n",j->size);
	printf("iterations = %d\n",j->iter);
	//j->ViewRows(10,20);
	for(i=0;i<20;i++)
	{
		for(k=1000;k<3000;k++)
		{
			sprintf(str,"%d",k);
			j->AddRow(str);
		}
		j->Open("big_file.test",30,1000000,20000);
		printf("iterations = %d\n",j->iter);
	}
	printf("\n");
	delete j;
}


int main(int args, char **argv)
{
	//testTable1();
	testTable2();

	testJournal1();
	testJournal2();

	return 0;
}