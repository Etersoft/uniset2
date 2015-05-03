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
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "UniXML.h"

int main()
{

	UniXML* f = new UniXML();
	f->newDoc("journal");
	xmlNode* cur, *beg;
	beg = cur = f->createChild(f->cur, "0", "");
	char* ch = new char[10];

	for(int j = 1; j < 30; j++)
	{
		sprintf(ch, "%d", j);
		cur = f->createNext(cur, ch, "");
		f->setProp(cur, "name", ch);
	}

	for(int j = 0; j < 10; j++)
	{
		sprintf(ch, "%d", j);
		cur = f->createChild(cur, ch, "");
		f->setProp(cur, "name", ch);

		for(int i = 0; i < 10; i++)
		{
			sprintf(ch, "%d", i * j);
			cur = f->createNext(cur, ch, "");
			f->setProp(cur, "name", ch);
		}
	}

	bool testPassed = true;

	if(f->extFindNode(beg, 9, 30, "72", "72") != 0)
		printf("correct, \"72\" has width 30 and depth 9\n");
	else testPassed = false;

	if(f->extFindNode(beg, 9, 29, "72", "72") == 0)
		printf("correct, \"72\" has width more than 29\n");
	else testPassed = false;

	if(f->extFindNode(beg, 8, 30, "72", "72") == 0)
		printf("correct, \"72\" has depth more than 8\n");
	else testPassed = false;

	printf("If test passed it is 1: %d\n", testPassed);
	return 0;
}