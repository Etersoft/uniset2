/*
 * Copyright (c) 2025 Pavel Vainerman.
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
// --------------------------------------------------------------------------
var unisetDelayTimerModuleVersion = "v1"
// --------------------------------------------------------------------------
load("uniset2-passivetimer.js")
// --------------------------------------------------------------------------
/**
 * Timer implementing activation and deactivation delay for signals.
 * The check(state) function is called for verification, where state is the current signal state,
 * and check() returns the signal with applied delay.
 * For state to change, it must persist for at least the specified time.
 *
 * Example:
 * const dt = new DelayTimer(200, 50);
 * if( dt.check(true) ) ...
 */
class DelayTimer
{
    constructor(on_msec = 0, off_msec = 0)
    {
        this.pt = new PassiveTimer();
        this.prevState = false;
        this.state = false;
        this.onDelay = on_msec;
        this.offDelay = off_msec;
        this.waiting_on = false;
        this.waiting_off = false;
    }

    /**
     * Set timer parameters
     * @param {number} on_msec - activation delay in milliseconds
     * @param {number} off_msec - deactivation delay in milliseconds
     */
    set(on_msec, off_msec)
    {
        this.onDelay = on_msec;
        this.offDelay = off_msec;
        this.waiting_on = false;
        this.waiting_off = false;
        this.state = false;
    }

    /**
     * Reset timer
     */
    reset()
    {
        this.pt.reset();
        this.prevState = false;
        this.waiting_on = false;
        this.waiting_off = false;
        this.state = false;
    }

    /**
     * Check state with delay processing
     * @param {boolean} st - current state
     * @returns {boolean} state with applied delays
     */
    check(st)
    {
        this.prevState = st;

        if (this.waiting_off)
        {
            if (this.pt.checkTime())
            {
                this.waiting_off = false;

                if (!st)
                    this.state = false;

                return this.state;
            }
            else if (st)
            {
                this.waiting_off = false;
            }

            return this.state;
        }

        if (this.waiting_on)
        {
            if (this.pt.checkTime())
            {
                this.waiting_on = false;

                if (st)
                    this.state = true;

                return this.state;
            }
            else if (!st)
            {
                this.waiting_on = false;
            }

            return this.state;
        }

        if (this.state !== st)
        {
            this.waiting_on = false;
            this.waiting_off = false;

            if (st)
            {
                if (this.onDelay <= 0)
                {
                    this.pt.setTiming(0);
                    this.state = st;
                    return st;
                }

                this.pt.setTiming(this.onDelay);
                this.waiting_on = true;
            }
            else
            {
                if (this.offDelay <= 0)
                {
                    this.pt.setTiming(0);
                    this.state = st;
                    return st;
                }

                this.pt.setTiming(this.offDelay);
                this.waiting_off = true;
            }
        }

        return this.state;
    }

    /**
     * Get current state (uses previous value)
     * @returns {boolean} current state
     */
    get()
    {
        return this.check(this.prevState);
    }

    /**
     * Get activation delay
     * @returns {number} activation delay in milliseconds
     */
    getOnDelay()
    {
        return this.onDelay;
    }

    /**
     * Get deactivation delay
     * @returns {number} deactivation delay in milliseconds
     */
    getOffDelay()
    {
        return this.offDelay;
    }

    /**
     * Get current timer time
     * @returns {number} current time in milliseconds
     */
    getCurrent()
    {
        return this.pt.getCurrent();
    }

    /**
     * Check if activation is pending
     * @returns {boolean} true if activation is pending
     */
    isWaitingOn()
    {
        return !this.get() && this.waiting_on;
    }

    /**
     * Check if deactivation is pending
     * @returns {boolean} true if deactivation is pending
     */
    isWaitingOff()
    {
        return this.get() && this.waiting_off;
    }

    /**
     * Check if any state change is pending
     * @returns {boolean} true if state change is pending
     */
    isWaiting()
    {
        this.check(this.prevState);
        return (this.waiting_off || this.waiting_on);
    }
}

// --------------------------------------------------------------------------
// Export for use in Node.js or other environments
if (typeof module !== 'undefined' && module.exports)
{
    module.exports = { DelayTimer };
}
// --------------------------------------------------------------------------
try { if (typeof DelayTimer === 'function') globalThis.DelayTimer = DelayTimer; } catch {}
// --------------------------------------------------------------------------
