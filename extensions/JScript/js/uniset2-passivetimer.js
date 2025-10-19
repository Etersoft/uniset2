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
var unisetPassiveTimerModuleVersion = "v1"
// --------------------------------------------------------------------------
/**
 * Passive Timer
 * After setting the timer in the constructor or using setTiming,
 * you can use checkTime to check if the specified time has elapsed
 * If t_msec==0 - the timer will trigger immediately
 */
class PassiveTimer
{
    /**
     * Constructor
     * @param {number} msec - time in milliseconds to set the timer
     */
    constructor(msec = 0)
    {
        this.t_msec = msec;  // timer interval in milliseconds
        this.t_start = Date.now(); // timer start time
        this.t_inner_msec = msec; // timer set time in milliseconds
    }

    /**
     * Check if the specified time has elapsed
     * @returns {boolean} true if time has elapsed
     */
    checkTime()
    {
        if (this.t_msec === 0)
            return true;

        return (Date.now() - this.t_start) >= this.t_msec;
    }

    /**
     * Set timer and start
     * @param {number} msec - time in milliseconds
     * @returns {number} set time
     */
    setTiming(msec)
    {
        this.t_msec = msec;
        this.t_inner_msec = msec;
        this.reset();
        return msec;
    }

    /**
     * Restart timer
     */
    reset()
    {
        this.t_start = Date.now();
    }

    /**
     * Get current timer value
     * @returns {number} time in milliseconds since start
     */
    getCurrent()
    {
        return Date.now() - this.t_start;
    }

    /**
     * Get the interval for which the timer is set
     * @returns {number} interval in milliseconds
     */
    getInterval()
    {
        return this.t_msec;
    }

    /**
     * Stop timer operation
     */
    terminate()
    {
        // In JavaScript version simply reset the timer
        this.reset();
    }

    /**
     * Get remaining time
     * @returns {number} remaining time in milliseconds
     */
    getLeft()
    {
        const elapsed = this.getCurrent();
        return Math.max(0, this.t_msec - elapsed);
    }
}
// --------------------------------------------------------------------------
// Export for use in Node.js or other environments
if (typeof module !== 'undefined' && module.exports)
{
    module.exports = { PassiveTimer };
}
// --------------------------------------------------------------------------
try { if (typeof PassiveTimer === 'function') globalThis.PassiveTimer = PassiveTimer; } catch {}
// --------------------------------------------------------------------------
