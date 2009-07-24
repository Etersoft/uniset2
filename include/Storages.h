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

#ifndef Storages_H_
#define Storages_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "UniXML.h"

#define key_size 20

struct StorageAttr
{
	int k_size, inf_size,size,block_number;
	int lim, seekpos;
} __attribute__((__packed__));

struct CycleStorageAttr
{
	int size, inf_size, seekpos;
} __attribute__((__packed__));

struct TableStorageElem
{
	char status;
	char key[key_size];
} __attribute__((__packed__));

struct TableBlockStorageElem
{
	int count;
} __attribute__((__packed__));

struct CycleStorageElem
{
	char status;
} __attribute__((__packed__));

class TableStorage
{
	FILE *file;
	int seekpos, inf_size;
	int head;
	public:
		int size;
		TableStorage(const char* name, int inf_sz, int sz, int seek);
		~TableStorage();
		int AddRow(char* key, char* val);
		int DelRow(char* key);
		char* FindKeyValue(char* key, char* val);
};

class TableBlockStorage
{
	int max;
	TableBlockStorageElem** mem;
	public:
		FILE *file;
		int inf_size, k_size, lim,seekpos;
		int size,cur_block,block_size,block_number;
		TableBlockStorage();
		TableBlockStorage(const char* name, int key_sz, int inf_sz, int sz, int block_num, int block_lim, int seek);
		~TableBlockStorage();
	private:
		void filewrite(TableBlockStorageElem* tbl,int seek, bool needflush=true);
		bool CopyToNextBlock();
	public:
		bool Open(const char* name, int inf_sz, int key_sz, int sz, int block_num, int block_lim, int seek);
		bool Create(const char* name, int inf_sz, int key_sz, int sz, int block_num, int block_lim, int seek);
		bool AddRow(char* key, char* val);
		bool DelRow(char* key);
		char* FindKeyValue(char* key, char* val);
};

class CycleStorage
{
	FILE *file;
	int seekpos, inf_size;
	int head,tail;
	void filewrite(CycleStorageElem* jrn,int seek, bool needflush=true);
	public:
		int size, iter;
		CycleStorage();
		CycleStorage(const char* name, int inf_sz, int sz, int seek);
		~CycleStorage();
		bool Open(const char* name, int inf_sz, int sz, int seek);
		bool Create(const char* name, int inf_sz, int sz, int seek);
		bool AddRow(char* str);
		bool DelRow(int row);
		bool DelAllRows(void);
		bool ViewRows(int beg, int num);
		bool ExportToXML(const char* name);
};

#endif
