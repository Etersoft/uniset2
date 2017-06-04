/*
 * Copyright (c) 2015 Pavel Vainerman.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 2.1.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Lesser Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
//--------------------------------------------------------------------------
#ifndef TA2D_H_
#define TA2D_H_
// --------------------------------------------------------------------------
#include "Element.h"
// --------------------------------------------------------------------------
namespace uniset
{
	// ---------------------------------------------------------------------------
	// "A2D"(analog to discrete)
	// Преобразование аналогового датчика в дискретный по заданному значению. (Value=XXX --> True).
	// Может быть один вход и много выходов.
	class TA2D:
		public Element
	{

		public:
			TA2D( Element::ElementID id, long filterValue = 1 );
			virtual ~TA2D();

			/*! num игнорируется, т.к. элемент с одним входом */
			virtual void setIn( size_t num, long value ) override;

			virtual long getOut() const override;
			virtual std::string getType() const override
			{
				return "A2D";
			}

			void setFilterValue( long value );

		protected:
			TA2D(): myout(false) {};

			bool myout;
			long fvalue = { 1 };

		private:
	};
	// --------------------------------------------------------------------------
} // end of namespace uniset
// ---------------------------------------------------------------------------
#endif
// ---------------------------------------------------------------------------
