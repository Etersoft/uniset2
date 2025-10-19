// uniset2-delaytimer.esm.js — minimal dual wrapper
import * as std from 'std';

try {
  if (typeof load === 'function') {
    load('uniset2-delaytimer.js');
  } else {
    std.loadScript('uniset2-delaytimer.js');
  }
} catch (e) {
  try { if (typeof print === 'function') print('[delaytimer.esm] load error:', e && e.message ? e.message : String(e)); } catch (_) {}
}

// Re-export from globalThis (legacy script must assign these)
export const DelayTimer = globalThis.DelayTimer;
export const unisetDelayTimerModuleVersion = globalThis.unisetDelayTimerModuleVersion;

// Default namespace
const __default = { DelayTimer, unisetDelayTimerModuleVersion };
export default __default;
