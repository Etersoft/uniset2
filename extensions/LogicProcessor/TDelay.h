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
#ifndef TDelay_H_
#define TDelay_H_
// --------------------------------------------------------------------------
#include "PassiveTimer.h"
#include "Element.h"
// ---------------------------------------------------------------------------
// "ON" delay element
// Сбрасывается без задержки.. а срабатывает с задержкой.
class TDelay:
	public Element
{

	public:
		TDelay( Element::ElementID id, timeout_t delayMS = 0, size_t inCount = 0 );
		virtual ~TDelay();

		virtual void tick() override;
		virtual void setIn( size_t num, bool state ) override;
		virtual bool getOut() const override;
		virtual std::string getType() const override
		{
			return "Delay";
		}

		void setDelay( timeout_t timeMS );
		inline timeout_t getDelay() const
		{
			return delay;
		}

	protected:
		TDelay(): myout(false), delay(0) {};

		bool myout;
		PassiveTimer pt;
		timeout_t delay;

	private:
};
// ---------------------------------------------------------------------------
#endif
// ---------------------------------------------------------------------------

