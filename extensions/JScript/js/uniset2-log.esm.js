// uniset2-log.esm.min.js — minimal dual wrapper for legacy logger
import * as std from 'std';

try {
  if (typeof load === 'function') {
    load('uniset2-log.js');
  } else {
    std.loadScript('uniset2-log.js');
  }
} catch (e) {
  try { if (typeof print === 'function') print('[log.esm] load error:', e && e.message ? e.message : String(e)); } catch (_) {}
}

// Re-export common logging API from globalThis (legacy script must assign these)
export const uinfo  = globalThis.uinfo;
export const uwarn  = globalThis.uwarn;
export const ucrit  = globalThis.ucrit;
export const udebug = globalThis.udebug;

export const uniset_log         = globalThis.uniset_log;
export const uniset_log_create  = globalThis.uniset_log_create;
export const LogLevels          = globalThis.LogLevels;
export const defaultLogger      = globalThis.defaultLogger;
export const unisetLogModuleVersion = globalThis.unisetLogModuleVersion;
export const uniset_internal_log = globalThis.uniset_internal_log;
export const uniset_internal_log_level = globalThis.uniset_internal_log_level;

// Default namespace
const __default = {
  uinfo, uwarn, ucrit, udebug,
  uniset_log, uniset_log_create,
  LogLevels, defaultLogger, unisetLogModuleVersion,
  uniset_internal_log, uniset_internal_log_level
};
export default __default;
