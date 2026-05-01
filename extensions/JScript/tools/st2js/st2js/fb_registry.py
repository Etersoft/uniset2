#!/usr/bin/env python3
"""Registry of standard IEC 61131-3 function blocks for uniset2-iec61131.js.

Maps IEC 61131-3 FB type names to their JavaScript class equivalents,
including constructor arguments, update() parameters, and output properties.
"""

from __future__ import annotations

from dataclasses import dataclass, field


@dataclass(frozen=True)
class FBInfo:
    """Metadata for a standard IEC 61131-3 function block.

    Attributes:
        name: IEC 61131-3 name (e.g. "TON").
        js_class: JavaScript class name in uniset2-iec61131.js (same as name).
        constructor_args: Arguments passed to JS constructor, as (name, type) pairs.
        update_params: Parameters for the update() method, as (name, type) pairs.
        outputs: Readable output properties after update(), as (name, type) pairs.
        library: "iec" for standard IEC 61131-3, "codesys" for Codesys Util.
    """

    name: str
    js_class: str
    constructor_args: list[tuple[str, str]] = field(default_factory=list)
    update_params: list[tuple[str, str]] = field(default_factory=list)
    outputs: list[tuple[str, str]] = field(default_factory=list)
    library: str = "iec"


# Standard FB registry keyed by IEC 61131-3 type name.
_REGISTRY: dict[str, FBInfo] = {}


def _register(info: FBInfo) -> None:
    """Register a function block in the internal registry."""
    _REGISTRY[info.name] = info


# --- Timers (IEC 61131-3, Tables 43-45) ---

_register(FBInfo(
    name="TON",
    js_class="TON",
    constructor_args=[("PT", "TIME")],
    update_params=[("IN", "BOOL")],
    outputs=[("Q", "BOOL"), ("ET", "TIME")],
))

_register(FBInfo(
    name="TOF",
    js_class="TOF",
    constructor_args=[("PT", "TIME")],
    update_params=[("IN", "BOOL")],
    outputs=[("Q", "BOOL"), ("ET", "TIME")],
))

_register(FBInfo(
    name="TP",
    js_class="TP",
    constructor_args=[("PT", "TIME")],
    update_params=[("IN", "BOOL")],
    outputs=[("Q", "BOOL"), ("ET", "TIME")],
))

# --- Counters (IEC 61131-3, Tables 40-42) ---

_register(FBInfo(
    name="CTU",
    js_class="CTU",
    constructor_args=[("PV", "INT")],
    update_params=[("CU", "BOOL"), ("RESET", "BOOL")],
    outputs=[("Q", "BOOL"), ("CV", "INT")],
))

_register(FBInfo(
    name="CTD",
    js_class="CTD",
    constructor_args=[("PV", "INT")],
    update_params=[("CD", "BOOL"), ("LOAD", "BOOL")],
    outputs=[("Q", "BOOL"), ("CV", "INT")],
))

_register(FBInfo(
    name="CTUD",
    js_class="CTUD",
    constructor_args=[("PV", "INT")],
    update_params=[("CU", "BOOL"), ("CD", "BOOL"), ("RESET", "BOOL"), ("LOAD", "BOOL")],
    outputs=[("QU", "BOOL"), ("QD", "BOOL"), ("CV", "INT")],
))

# --- Bistable (IEC 61131-3, Tables 36-37) ---

_register(FBInfo(
    name="RS",
    js_class="RS",
    constructor_args=[],
    update_params=[("SET", "BOOL"), ("RESET1", "BOOL")],
    outputs=[("Q1", "BOOL")],
))

_register(FBInfo(
    name="SR",
    js_class="SR",
    constructor_args=[],
    update_params=[("SET1", "BOOL"), ("RESET", "BOOL")],
    outputs=[("Q1", "BOOL")],
))

# --- Edge detection (IEC 61131-3, Tables 38-39) ---

_register(FBInfo(
    name="R_TRIG",
    js_class="R_TRIG",
    constructor_args=[],
    update_params=[("CLK", "BOOL")],
    outputs=[("Q", "BOOL")],
))

_register(FBInfo(
    name="F_TRIG",
    js_class="F_TRIG",
    constructor_args=[],
    update_params=[("CLK", "BOOL")],
    outputs=[("Q", "BOOL")],
))


# ---------------------------------------------------------------------------
# Codesys Util library FBs (from uniset2-iec61131-codesys.js)
# ---------------------------------------------------------------------------

_register(FBInfo(
    name="BLINK",
    js_class="BLINK",
    constructor_args=[("TIMEHIGH", "TIME"), ("TIMELOW", "TIME")],
    update_params=[("ENABLE", "BOOL")],
    outputs=[("OUT", "BOOL")],
    library="codesys",
))

_register(FBInfo(
    name="HYSTERESIS",
    js_class="HYSTERESIS",
    constructor_args=[],
    update_params=[("IN", "INT"), ("HIGH", "INT"), ("LOW", "INT")],
    outputs=[("OUT", "BOOL")],
    library="codesys",
))

_register(FBInfo(
    name="LIMITALARM",
    js_class="LIMITALARM",
    constructor_args=[],
    update_params=[("IN", "INT"), ("HIGH", "INT"), ("LOW", "INT")],
    outputs=[("O", "BOOL"), ("U", "BOOL"), ("IL", "BOOL")],
    library="codesys",
))

_register(FBInfo(
    name="LIN_TRAFO",
    js_class="LIN_TRAFO",
    constructor_args=[("IN_MIN", "REAL"), ("IN_MAX", "REAL"), ("OUT_MIN", "REAL"), ("OUT_MAX", "REAL")],
    update_params=[("IN", "REAL")],
    outputs=[("OUT", "REAL"), ("ERROR", "BOOL")],
    library="codesys",
))

_register(FBInfo(
    name="RAMP_REAL",
    js_class="RAMP_REAL",
    constructor_args=[("ASCEND", "REAL"), ("DESCEND", "REAL"), ("TIMEBASE", "TIME")],
    update_params=[("IN", "REAL"), ("RESET", "BOOL")],
    outputs=[("OUT", "REAL")],
    library="codesys",
))

_register(FBInfo(
    name="PID",
    js_class="PID",
    constructor_args=[("KP", "REAL"), ("TN", "REAL"), ("TV", "REAL"), ("Y_MIN", "REAL"), ("Y_MAX", "REAL")],
    update_params=[("ACTUAL", "REAL"), ("SET_POINT", "REAL"), ("RESET", "BOOL")],
    outputs=[("Y", "REAL"), ("LIMITS_ACTIVE", "BOOL"), ("OVERFLOW", "BOOL")],
    library="codesys",
))

_register(FBInfo(
    name="DERIVATIVE",
    js_class="DERIVATIVE",
    constructor_args=[],
    update_params=[("IN", "REAL"), ("TM", "DWORD"), ("RESET", "BOOL")],
    outputs=[("OUT", "REAL")],
    library="codesys",
))

_register(FBInfo(
    name="INTEGRAL",
    js_class="INTEGRAL",
    constructor_args=[],
    update_params=[("IN", "REAL"), ("TM", "DWORD"), ("RESET", "BOOL")],
    outputs=[("OUT", "REAL"), ("OVERFLOW", "BOOL")],
    library="codesys",
))


def needs_codesys_library(fb_instances: list) -> bool:
    """Check if any FB instance requires the Codesys library."""
    for inst in fb_instances:
        info = get_fb_info(inst.fb_type if hasattr(inst, 'fb_type') else str(inst))
        if info is not None and info.library == "codesys":
            return True
    return False


def get_fb_info(name: str) -> FBInfo | None:
    """Look up function block info by IEC 61131-3 type name.

    Args:
        name: FB type name (case-sensitive, e.g. "TON").

    Returns:
        FBInfo if known, None otherwise.
    """
    return _REGISTRY.get(name)


def load_lib_types(yaml_path: str) -> int:
    """Load external FB type definitions from a YAML file and register them.

    YAML format:
        function_blocks:
          TypeName:
            inputs:
              ParamName: TYPE
            outputs:
              ParamName: TYPE

    Returns:
        Number of types registered.
    """
    import yaml
    with open(yaml_path) as f:
        data = yaml.safe_load(f)

    count = 0
    for fb_name, fb_def in (data.get("function_blocks") or {}).items():
        inputs = fb_def.get("inputs") or {}
        outputs = fb_def.get("outputs") or {}
        info = FBInfo(
            name=fb_name,
            js_class=fb_name.replace(".", "_"),
            update_params=[(k, v) for k, v in inputs.items()],
            outputs=[(k, v) for k, v in outputs.items()],
            library="external",
        )
        _REGISTRY[fb_name] = info
        count += 1
    return count


def is_standard_fb(name: str) -> bool:
    """Check whether a name refers to a standard IEC 61131-3 function block.

    Args:
        name: FB type name (case-sensitive).

    Returns:
        True if the name is in the standard FB registry.
    """
    return name in _REGISTRY
