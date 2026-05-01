/*
 * Thermostat process imitator
 *
 * Simulates the physical process:
 * - Temperature rises/falls depending on heater state
 * - Enable button press at start
 * - Periodic reset to test alarm recovery
 *
 * Loaded from main-plc.js, called from uniset_on_step().
 */

var imitator = (function() {

    // Process model parameters
    var ambientTemp = 15.0;       // ambient temperature (°C)
    var heaterPower = 0.5;        // heating rate (°C per cycle when heater ON)
    var coolingRate = 0.05;       // cooling rate (°C per cycle, natural loss)
    var currentTemp = ambientTemp; // simulated temperature

    // Scenario state
    var cycleCount = 0;
    var enableSent = false;
    var resetScheduled = false;
    var resetCycle = 0;

    function update()
    {
        cycleCount++;

        // --- Step 1: Read heater output to affect temperature ---
        var heaterOn = !!globalThis.out_DO_Heater_C;

        // --- Step 2: Simulate temperature physics ---
        if (heaterOn)
        {
            currentTemp += heaterPower;
        }
        else
        {
            // Cool down towards ambient
            currentTemp -= coolingRate * (currentTemp - ambientTemp);
        }

        // Clamp to realistic range
        if (currentTemp < -10) currentTemp = -10;
        if (currentTemp > 120) currentTemp = 120;

        // --- Step 3: Write simulated temperature to sensor ---
        // Scale: AI_Temperature_S = temperature * 100
        globalThis.in_AI_Temperature_S = Math.round(currentTemp * 100);

        // --- Step 4: Scenario automation ---

        // Enable at cycle 10 (single pulse)
        if (cycleCount === 10 && !enableSent)
        {
            globalThis.in_DI_Enable_S = 1;
            enableSent = true;
        }
        else if (cycleCount === 12 && enableSent)
        {
            globalThis.in_DI_Enable_S = 0;
        }

        // If alarm triggered — schedule reset after 30 cycles
        var alarmHigh = !!globalThis.out_DO_AlarmHigh_C;
        if (alarmHigh && !resetScheduled)
        {
            resetScheduled = true;
            resetCycle = cycleCount + 30;
        }

        // Execute scheduled reset
        if (resetScheduled && cycleCount === resetCycle)
        {
            globalThis.in_DI_Reset_S = 1;
        }
        else if (resetScheduled && cycleCount === resetCycle + 2)
        {
            globalThis.in_DI_Reset_S = 0;
            resetScheduled = false;
            currentTemp = ambientTemp;
        }
    }

    return { update: update };

})();
