/*
 * HVAC process imitator — two-zone temperature simulation
 */
var imitator = (function() {
    var outdoor = 10.0;
    var zone1 = 18.0;   // office — needs heating
    var zone2 = 28.0;   // server room — needs cooling
    var cycle = 0;
    var enabled = false;

    function update() {
        cycle++;

        // Read outputs (directly from globals — works without SharedMemory)
        var heater = !!globalThis.out_DO_Zone1Heater_C;
        var cooler = !!globalThis.out_DO_Zone2Cooler_C;

        // Outdoor: slow sine wave 5-15°C
        outdoor = 10.0 + 5.0 * Math.sin(cycle * 0.005);

        // Zone 1 (office): heating effect
        if (heater) {
            zone1 += 0.3;
        } else {
            zone1 -= 0.05 * (zone1 - outdoor);
        }
        if (zone1 < 0) zone1 = 0;
        if (zone1 > 50) zone1 = 50;

        // Zone 2 (server): cooling effect + heat from servers
        zone2 += 0.1;  // servers generate heat
        if (cooler) {
            zone2 -= 0.4;
        } else {
            zone2 += 0.02 * (outdoor - zone2) * 0.1;
        }
        if (zone2 < 10) zone2 = 10;
        if (zone2 > 50) zone2 = 50;

        // Write sensors (directly to globals — works without SharedMemory)
        globalThis.in_AI_OutdoorTemp_S = Math.round(outdoor * 100);
        globalThis.in_AI_Zone1Temp_S = Math.round(zone1 * 100);
        globalThis.in_AI_Zone2Temp_S = Math.round(zone2 * 100);

        // Enable at cycle 5
        if (cycle === 5 && !enabled) {
            globalThis.in_DI_Enable_S = 1;
            enabled = true;
        } else if (cycle === 7) {
            globalThis.in_DI_Enable_S = 0;
        }
    }

    return { update: update };
})();
