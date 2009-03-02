/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Vitaly Lipatov
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

/*! \file
 *  \author Vitaly Lipatov <lav>
 *  \date   $Date: 2007/01/02 22:30:48 $
 *  \version $Id: TextFileIndex.h,v 1.7 2007/01/02 22:30:48 vpashka Exp $
 *  \par
 * Базовый класс получения строки по её индексу
 */

#include <string>

class TextFileIndex: public TextIndex
{
	public:
		virtual ~TextFileIndex(){}

		// Получить строку по коду
		virtual std::string getText(int id);
		// При инициализации указывается название файла для считывания
		TextFileIndex(std::string filename);
};
