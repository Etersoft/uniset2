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

#include <Storages.h>
#include <UniXML.h>
#include <UniSetTypes.h>

int seek=0;
int b_size=100000;
int bj_size=1300000;
void testTable1(void)
{
	char *chr=new char[20];
	char *val=new char[40];
	TableStorage *t;
	t = new TableStorage("table.test", 40, 1220, 0);
	int i;
	for(i=0;i<20;i++)
	{
		chr[0]=i;
		sprintf(val,"%d",i);
		t->addRow(chr,val);
	}
	printf("elements with values=keys added:\n");
	for(i=0;i<40;i++)
	{
		chr[0]=i;
		if(t->findKeyValue(chr,val)!=0) printf("%s, ",val);
	}
	printf("\n");
	for(i=9;i<15;i++)
	{
		chr[0]=i;
		t->delRow(chr);
	}
	printf("elements with keys from 9 to 14 deleted\n");
	for(i=9;i<15;i++)
	{
		chr[0]=i;
		sprintf(val,"%d",i+40);
		t->addRow(chr,val);
	}
	printf("elements with keys from 9 to 14 with values=key+40 added, all elements:\n");
	for(i=0;i<40;i++)
	{
		chr[0]=i;
		if(t->findKeyValue(chr,val)!=0) printf("%s, ",val);
	}
	printf("\n");
}

bool testTable2(void)
{
	char *val=new char[40];
	TableBlockStorage t;
	//t = new TableBlockStorage();
	t.create("big_file.test", b_size, 4, 40, 100, 5,28,0);
	seek=t.getByteSize();
	printf("Table size in bytes = %d\n",seek);
	int i;
	for(i=1;i<20;i++)
	{
		if(t.findKeyValue(&i,val)!=0) printf("%s, ",val);
	}
	printf("\n");
	if(t.getCurBlock()!=0)
	{
		delete val;
		return false;
	}
	for(i=1;i<11;i++)
	{
		sprintf(val,"%d",i);
		t.addRow((char*)&i,val);
	}
	if(t.getCurBlock()!=0)
	{
		delete val;
		return false;
	}
	for(i=1;i<20;i++)
	{
		if(t.findKeyValue(&i,val)!=0) printf("%s, ",val);
		if(val[0]==0)
		{
			delete val;
			return false;
		}
	}
	printf("\n");
	if(t.getCurBlock()!=0)
	{
		delete val;
		return false;
	}
	for(i=1;i<8;i++)
	{
		sprintf(val,"%d",i+10);
		t.addRow(&i,val);
	}
	printf("deleteing 8-10 elements\n");
	for(i=8;i<11;i++)
	{
		t.delRow(&i);
	}
	for(i=1;i<20;i++)
	{
		if(t.findKeyValue(&i,val)!=0)
		{
			printf("%s, ",val);
			if((i > 7)&&(i <11))
			{
				delete val;
				return false;
			}
		}
		if((val[0] == 0)&&(i < 8))
		{
			delete val;
			return false;
		}
	}
	printf("\nrewriting 3-10 elements with values=keys+40\n");
	if(t.getCurBlock()!=0)
	{
		delete val;
		return false;
	}
	for(i=3;i<11;i++)
	{
		sprintf(val,"%d",i+40);
		t.addRow(&i,val);
	}
	for(i=1;i<20;i++)
	{
		if(t.findKeyValue(&i,val)!=0) printf("%s, ",val);
		if((UniSetTypes::uni_atoi(val) != i+40) && (i>2) && (i<11))
		{
			delete val;
			return false;
		}
		if((UniSetTypes::uni_atoi(val) != i+10) && (i<3))
		{
			delete val;
			return false;
		}
	}
	if(t.getCurBlock()!=0)
	{
		delete val;
		return false;
	}

	printf("\n");

	strcpy(val,"new block");
	i=9;
	t.addRow(&i,val);
	for(i=1;i<20;i++)
	{
		if(t.findKeyValue((char*)&i,val)!=0) printf("%s, ",val);
	}
	if(t.getCurBlock()!=1)
	{
		delete val;
		return false;
	}
	printf("after reopen:\n");
	t.open("big_file.test", b_size, 4, 40, 100, 5,28,0);
	for(i=1;i<20;i++)
	{
		if(t.findKeyValue(&i,val)!=0) printf("%s, ",val);
	}
	if(t.getCurBlock()!=1)
	{
		delete val;
		return false;
	}
	delete val;
	return true;
}

bool reOpen()
{
	CycleStorage j;
	int i,k=0;
	char *str = new char[30];
	printf("the same after reopen:\n");
	if(!j.open("big_file.test",bj_size,30,33000,seek))
	{
		printf("Reopen file error\n");
		delete str;
		return false;
	}
	for(i=0;i<20;i++)
	{
		if(j.readRow(i,str))
		{
			printf("%s\n",str);
			k++;
		}
	}
	delete str;
	if(k != 10)
		return false;
	return true;
}

bool testJournal1(void)
{
	CycleStorage j("big_file.test",bj_size,30,32000,seek,true);
	int i,k=0;
	char *str = new char[30];
	printf("journal test 1\n");
	for(i=1;i<64001;i++)
	{
		sprintf(str,"%d",i);
		j.addRow(str);
	}
	printf("first 30 elements:\n");
	for(i=0;i<30;i++)
	{
		if(j.readRow(i,str))
		{
			printf("%s\n",str);
			k++;
		}
	}
	if(k < 30)
	{
		delete str;
		return false;
	}
	k = 0;
	printf("size changed to 33000 rows (increased)\n");
	j.setSize(33000);
	TableBlockStorage t("big_file.test", b_size, 4, 40, 100, 5,28,0);
	printf("test of 2 classes working in 1 file together\n");
	char *val = new char[40];
	for(i=1;i<20;i++)
	{
		if(t.findKeyValue((char*)&i,val)!=0) printf("%s, ",val);
		if((UniSetTypes::uni_atoi(val) != i+10) && (i<3))
		{
			delete val;
			delete str;
			return false;
		}
	}
	delete val;

	printf("\nfirst 30 elements after deleting first 20:\n");
	for(i=0;i<20;i++)
	{
		j.delRow(i);
	}
	for(i=0;i<30;i++)
	{
		if(j.readRow(i,str))
		{
			printf("%s\n",str);
			k++;
		}
	}
	if(k != 10)
	{
		delete str;
		return false;
	}
	k = 0;

	printf("first 20 after adding 10 elements\n");
	for(i=10000001;i<10000011;i++)
	{
		sprintf(str,"%d",i);
		j.addRow(str);
	}
	for(i=0;i<20;i++)
	{
		if(j.readRow(i,str))
		{
			printf("%s\n",str);
			k++;
		}
	}
	if(k != 10)
	{
		delete str;
		return false;
	}

	k = 0;

	if(!reOpen()) return false;

	if(!reOpen()) return false;

	printf("size changed back to 32000 rows\n");
	j.setSize(32000);
	for(i=0;i<20;i++)
	{
		if(j.readRow(i,str))
		{
			printf("%s\n",str);
			k++;
		}
	}
	if(k != 10)
	{
		delete str;
		return false;
	}

	k = 0;
	delete str;
	return true;
}

void testJournal2(void)
{
	CycleStorage j("big_file.test",bj_size,30,32000,seek);
	int i,k;
	char *str = new char[30];
	printf("journal test 2 - checking number of iterations to find head/tail\n");
	printf("iterations = %d\n",j.getIter());
	for(i=0;i<20;i++)
	{
		for(k=1000;k<2999;k++)
		{
			sprintf(str,"%d",k);
			j.addRow(str);
		}
		j.open("big_file.test",bj_size,30,32000,seek);
		printf("i=%d, iterations = %d\n", i, j.getIter());
	}
	printf("\n");
	delete str;
}


struct JItem
{
	long id;
	long val[10];
} __attribute__((__packed__));

bool testJournal3()
{
	CycleStorage j("journal3.test",bj_size,sizeof(JItem),10,0,true);
	if( !j.isOpen() )
	{
		printf("create journal3.test failed\n");
		return false;
	}

	printf("Joural size=%d inf_size=%d full_size=%d byte_size=%d\n",
	j.getSize(),
	j.getInfSize(),
	j.getFullSize(),
	j.getByteSize()
	);

	JItem ji;

	printf("write 35 elements:\n");
	for(int i=0;i<35;i++)
	{
		JItem ji;
		ji.id = i;
		for( int k=0; k<10; k++ )
			ji.val[k] = i;

		j.addRow(&ji);
	}

	printf("read first 10 elements:\n");
	
	for( int i=0;i<10;i++)
	{
		if( j.readRow(i,&ji) )
			printf("read i=%d j.id=%d\n",i,ji.id);
		else
			printf("read num=%d FAILED!\n",i);
			
	}

	return true;
}


int main(int args, char **argv)
{
	//testTable1();
	bool ok = true;

	if(testTable2())
		printf("\nTest for TableBlockStorage passed\n\n");
	else
	{
		printf("\nTest for TableBlockStorage failed\n\n");
		ok = false;
	}

	if(testJournal1())
		printf("\nTest1 for CycleStorage passed\n\n");
	else
	{
		printf("\nTest for CycleStorage failed\n\n");
		ok = false;
	}

	if(ok)
	{
		testJournal2();
		printf("TEST PASSED :)\n");
	}
	else
		printf("TEST FAILED :(\n");

	testJournal3();

	return 0;
}