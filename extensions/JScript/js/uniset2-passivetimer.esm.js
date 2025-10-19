// uniset2-passivetimer.esm.js — minimal dual wrapper (no delaytimer deps)
import * as std from 'std';

try {
  if (typeof load === 'function') {
    load('uniset2-passivetimer.js');
  } else {
    std.loadScript('uniset2-passivetimer.js');
  }
} catch (e) {
  try { if (typeof print === 'function') print('[passivetimer.esm] load error:', e && e.message ? e.message : String(e)); } catch (_) {}
}

// Re-export from globalThis (legacy script must assign these)
export const PassiveTimer = globalThis.PassiveTimer;
export const unisetPassiveTimerModuleVersion = globalThis.unisetPassiveTimerModuleVersion;

// Default namespace
const __default = { PassiveTimer, unisetPassiveTimerModuleVersion };
export default __default;
