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

char* itoa(int val, int base)
{
	
	static char buf[32] = {0};
	
	int i = 30;
	
	for(; val && i ; --i, val /= base)
	
		buf[i] = "0123456789abcdef"[val % base];
	
	return &buf[i+1];
	
}


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
		val=itoa(i,10);
		t->AddRow(chr,val);
	}
	//printf("elements with values=keys added\nvalues from keys 25-59\n");
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
	//printf("elements with keys from 40 to 60 deleted\n");
	printf("elements with keys from 9 to 14 deleted\n");
	for(i=9;i<15;i++)
	{
		chr[0]=i;
		val=itoa(i+40,10);
		t->AddRow(chr,val);
	}
	//printf("elements with keys from 30 to 50 with values=key+40 added\nvalues from keys 25-59\n");
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
	char *chr=(char*)malloc(20);
	char *val=(char*)malloc(40);
	TableBlockStorage *t;
	t = new TableBlockStorage("blocktable.test", 40, 20000, 5,40);
	int i;
	printf("testTable\nsize = %d\n",t->block_size);
	for(i=1;i<16;i++)
	{
		chr[0]=i;
		val=itoa(i,10);
		t->AddRow(chr,val);
	}
	printf("current block = %d, elements with values=keys added:\n",t->cur_block);
	for(i=1;i<16;i++)
	{
		chr[0]=i;
		if(t->FindKeyValue(chr,val)!=0) printf("%s, ",val);
	}
	printf("\ncurrent block = %d\n",t->cur_block);
	for(i=1;i<16;i++)
	{
		chr[0]=i;
		val=itoa(i+10,10);
		t->AddRow(chr,val);
	}
	for(i=1;i<16;i++)
	{
		chr[0]=i;
		if(t->FindKeyValue(chr,val)!=0) printf("%s, ",val);
	}
	printf("\ncurrent block = %d\n",t->cur_block);
	for(i=20;i<30;i++)
	{
		chr[0]=i;
		val=itoa(i,10);
		t->AddRow(chr,val);
	}
	for(i=1;i<40;i++)
	{
		chr[0]=i;
		if(t->FindKeyValue(chr,val)!=0) printf("%s, ",val);
	}
	printf("\ncurrent block = %d\n",t->cur_block);

	chr[0]=30;
	strcpy(val,"new block");
	t->AddRow(chr,val);
	for(i=1;i<40;i++)
	{
		chr[0]=i;
		if(t->FindKeyValue(chr,val)!=0) printf("%s, ",val);
	}
	printf("\ncurrent block = %d\n",t->cur_block);
	/*for(i=9;i<15;i++)
	{
		chr[0]=i;
		t->DelRow(chr);
	}
	//printf("elements with keys from 40 to 60 deleted\n");
	printf("elements with keys from 9 to 14 deleted\n");
	for(i=9;i<15;i++)
	{
		chr[0]=i;
		val=itoa(i+40,10);
		t->AddRow(chr,val);
	}
	//printf("elements with keys from 30 to 50 with values=key+40 added\nvalues from keys 25-59\n");
	printf("elements with keys from 9 to 14 with values=key+40 added, all elements:\n");
	for(i=0;i<40;i++)
	{
		chr[0]=i;
		if(t->FindKeyValue(chr,val)!=0) printf("%s, ",val);
	}
	printf("\n");*/
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
	j->ExportToXML("Xml.xml");
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
		for(k=1000;k<3000;k++)
		{
			str = itoa(k,10);
			j->AddRow(str);
		}
		j = new CycleStorage("journal.test",30,2000000,0);
		printf("iterations = %d\n",j->iter);
	}
	printf("\n");
}


int main(int args, char **argv)
{
	/*if(args<2)
	{
		printf("Correct usage: jrntest [command] [param]\n");
		printf(" commands:\n  add - add row with text = param\n  delete - delete row with number = param\n  view - view param2  from row = param1 \n  toxml - export journal to XML file with name = param\n  del_all - delete all journal\n  q or quit - exit\n\n");
		return 0;
	}
	CycleStorage *j = new CycleStorage(argv[1],99,1000,0);
	printf("commands:\nadd\ndelete\nview\ntoxml\ndel_all\nenter q or quit to exit\n\n");
	char* com=new char[8];
	char* str=new char[99];
	int num,count;
	strcpy(com,"qwerty");
	while(strcmp(com,"q")&&strcmp(com,"quit"))
	{
		scanf("%s",com);
		if(!strcmp(com,"add"))
		{
			printf("string to add: ");
			if(scanf("%99s",str)>0)
			{
				j->AddRow(str);
				printf("\n");
			}
		}
		else if(!strcmp(com,"delete"))
		{
			printf("number of string to delete: ");
			if(scanf("%d",&num)>0)
			{
				j->DelRow(num);
				printf("\n");
			}
		}
		else if(!strcmp(com,"view"))
		{
			printf("start number and count (0 0 for all): ");
			if(scanf("%d%d",&num,&count)>1)
			{
				j->ViewRows(num,count);
				printf("\n");
			}
		}
		else if(!strcmp(com,"del_all"))
		{
			printf("are you sure? (y/n) ");
			if(scanf("%s",str)>0)
				if(!strcmp(str,"y"))
				{
					j->DelAllRows();
					printf("\n");
				}
		}
		else if(!strcmp(com,"toxml"))
		{
			printf("enter file name: ");
			if(scanf("%25s",str)>0)
			{
				j->ExportToXML(str);
				printf("\n");
			}
		}
		else printf("commands:\nadd\ndelete\nview\ntoxml\ndel_all\nenter q or quit to exit\n\n");
	}*/
	testTable1();
	testTable2();

	testJournal1();
	testJournal2();

	return 0;
}