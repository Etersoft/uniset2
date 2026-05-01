"""Tests for function block code generation (codegen.py).

Tests cover:
- IRFBInstance emits correct constructor: new TON(pt), new CTU(pv), new RS()
- IRFBCall emits correct update() calls with args in correct order
- IRFieldAccess on FB emits obj.field
- All 10 standard FBs generate correct constructor and update() calls
- FUNCTION_BLOCK declaration emits JS class with constructor and execute()
"""
from __future__ import annotations

import pytest

from st2js.ir import (
    IECType,
    IRAssignment,
    IRFBCall,
    IRFBInstance,
    IRFieldAccess,
    IRFunctionBlock,
    IRIfElse,
    IRLiteral,
    IRProgram,
    IRVarRef,
    IRVariable,
)
from st2js.mapping import SensorEntry, SensorMapping, MappingOptions


def _make_program(
    name: str = "Main",
    inputs: list | None = None,
    outputs: list | None = None,
    locals_: list | None = None,
    fb_instances: list | None = None,
    body: list | None = None,
    function_blocks: list | None = None,
) -> IRProgram:
    """Helper to create an IRProgram with defaults."""
    return IRProgram(
        name=name,
        inputs=inputs or [],
        outputs=outputs or [],
        locals=locals_ or [],
        fb_instances=fb_instances or [],
        body=body or [],
        function_blocks=function_blocks or [],
    )


def _make_mapping(
    inputs: list[SensorEntry] | None = None,
    outputs: list[SensorEntry] | None = None,
) -> SensorMapping:
    """Helper to create a SensorMapping with defaults."""
    return SensorMapping(
        inputs=inputs or [],
        outputs=outputs or [],
        options=MappingOptions(),
    )


class TestFBInstanceCodegen:
    """Test IRFBInstance generates correct 'const name = new FBType(args);' declarations."""

    def test_ton_instance_with_pt(self):
        from st2js.codegen import generate

        program = _make_program(
            fb_instances=[
                IRFBInstance(
                    name="myTimer",
                    fb_type="TON",
                    constructor_args={"PT": IRLiteral(value=3000, iec_type=IECType.TIME)},
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        assert "const myTimer = new TON(3000);" in js

    def test_tof_instance_with_pt(self):
        from st2js.codegen import generate

        program = _make_program(
            fb_instances=[
                IRFBInstance(
                    name="myTimer",
                    fb_type="TOF",
                    constructor_args={"PT": IRLiteral(value=5000, iec_type=IECType.TIME)},
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        assert "const myTimer = new TOF(5000);" in js

    def test_tp_instance_with_pt(self):
        from st2js.codegen import generate

        program = _make_program(
            fb_instances=[
                IRFBInstance(
                    name="myPulse",
                    fb_type="TP",
                    constructor_args={"PT": IRLiteral(value=1000, iec_type=IECType.TIME)},
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        assert "const myPulse = new TP(1000);" in js

    def test_ctu_instance_with_pv(self):
        from st2js.codegen import generate

        program = _make_program(
            fb_instances=[
                IRFBInstance(
                    name="myCounter",
                    fb_type="CTU",
                    constructor_args={"PV": IRLiteral(value=10, iec_type=IECType.INT)},
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        assert "const myCounter = new CTU(10);" in js

    def test_ctd_instance_with_pv(self):
        from st2js.codegen import generate

        program = _make_program(
            fb_instances=[
                IRFBInstance(
                    name="myCounter",
                    fb_type="CTD",
                    constructor_args={"PV": IRLiteral(value=100, iec_type=IECType.INT)},
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        assert "const myCounter = new CTD(100);" in js

    def test_ctud_instance_with_pv(self):
        from st2js.codegen import generate

        program = _make_program(
            fb_instances=[
                IRFBInstance(
                    name="myCounter",
                    fb_type="CTUD",
                    constructor_args={"PV": IRLiteral(value=50, iec_type=IECType.INT)},
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        assert "const myCounter = new CTUD(50);" in js

    def test_rs_instance_no_args(self):
        from st2js.codegen import generate

        program = _make_program(
            fb_instances=[
                IRFBInstance(name="myLatch", fb_type="RS"),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        assert "const myLatch = new RS();" in js

    def test_sr_instance_no_args(self):
        from st2js.codegen import generate

        program = _make_program(
            fb_instances=[
                IRFBInstance(name="myLatch", fb_type="SR"),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        assert "const myLatch = new SR();" in js

    def test_r_trig_instance_no_args(self):
        from st2js.codegen import generate

        program = _make_program(
            fb_instances=[
                IRFBInstance(name="myTrigger", fb_type="R_TRIG"),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        assert "const myTrigger = new R_TRIG();" in js

    def test_f_trig_instance_no_args(self):
        from st2js.codegen import generate

        program = _make_program(
            fb_instances=[
                IRFBInstance(name="myTrigger", fb_type="F_TRIG"),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        assert "const myTrigger = new F_TRIG();" in js

    def test_fb_instances_before_on_step(self):
        """FB instance declarations should appear before uniset_on_step."""
        from st2js.codegen import generate

        program = _make_program(
            fb_instances=[
                IRFBInstance(name="myTimer", fb_type="TON",
                             constructor_args={"PT": IRLiteral(value=1000, iec_type=IECType.TIME)}),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        fb_pos = js.index("const myTimer")
        on_step_pos = js.index("function uniset_on_step()")
        assert fb_pos < on_step_pos

    def test_timer_instance_without_pt_uses_zero(self):
        """If PT not provided in constructor_args, default to 0."""
        from st2js.codegen import generate

        program = _make_program(
            fb_instances=[
                IRFBInstance(name="myTimer", fb_type="TON"),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        assert "const myTimer = new TON(0);" in js


class TestFBCallCodegen:
    """Test IRFBCall emits correct update() calls."""

    def test_ton_update_call(self):
        from st2js.codegen import generate

        program = _make_program(
            inputs=[
                IRVariable(name="Enable", iec_type=IECType.BOOL, is_input=True),
            ],
            fb_instances=[
                IRFBInstance(name="myTimer", fb_type="TON",
                             constructor_args={"PT": IRLiteral(value=3000, iec_type=IECType.TIME)}),
            ],
            body=[
                IRFBCall(
                    instance="myTimer",
                    arguments={"IN": IRVarRef(name="Enable")},
                ),
            ],
        )
        mapping = _make_mapping(
            inputs=[SensorEntry(st_name="Enable", sensor_name="DI_Enable_S")],
        )
        js = generate(program, mapping)
        assert "myTimer.update(in_DI_Enable_S);" in js

    def test_ctu_update_call_arg_order(self):
        """CTU.update(CU, RESET) - order must match fb_registry."""
        from st2js.codegen import generate

        program = _make_program(
            inputs=[
                IRVariable(name="Pulse", iec_type=IECType.BOOL, is_input=True),
                IRVariable(name="ResetCmd", iec_type=IECType.BOOL, is_input=True),
            ],
            fb_instances=[
                IRFBInstance(name="myCounter", fb_type="CTU",
                             constructor_args={"PV": IRLiteral(value=10, iec_type=IECType.INT)}),
            ],
            body=[
                IRFBCall(
                    instance="myCounter",
                    arguments={
                        "CU": IRVarRef(name="Pulse"),
                        "RESET": IRVarRef(name="ResetCmd"),
                    },
                ),
            ],
        )
        mapping = _make_mapping(
            inputs=[
                SensorEntry(st_name="Pulse", sensor_name="DI_Pulse_S"),
                SensorEntry(st_name="ResetCmd", sensor_name="DI_Reset_S"),
            ],
        )
        js = generate(program, mapping)
        assert "myCounter.update(in_DI_Pulse_S, in_DI_Reset_S);" in js

    def test_ctud_update_call_four_args(self):
        """CTUD.update(CU, CD, RESET, LOAD) - all 4 args in order."""
        from st2js.codegen import generate

        program = _make_program(
            locals_=[
                IRVariable(name="cu", iec_type=IECType.BOOL, is_local=True),
                IRVariable(name="cd", iec_type=IECType.BOOL, is_local=True),
                IRVariable(name="rst", iec_type=IECType.BOOL, is_local=True),
                IRVariable(name="ld", iec_type=IECType.BOOL, is_local=True),
            ],
            fb_instances=[
                IRFBInstance(name="myCtud", fb_type="CTUD",
                             constructor_args={"PV": IRLiteral(value=100, iec_type=IECType.INT)}),
            ],
            body=[
                IRFBCall(
                    instance="myCtud",
                    arguments={
                        "CU": IRVarRef(name="cu"),
                        "CD": IRVarRef(name="cd"),
                        "RESET": IRVarRef(name="rst"),
                        "LOAD": IRVarRef(name="ld"),
                    },
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        assert "myCtud.update(cu, cd, rst, ld);" in js

    def test_rs_update_call(self):
        from st2js.codegen import generate

        program = _make_program(
            locals_=[
                IRVariable(name="s", iec_type=IECType.BOOL, is_local=True),
                IRVariable(name="r", iec_type=IECType.BOOL, is_local=True),
            ],
            fb_instances=[
                IRFBInstance(name="myLatch", fb_type="RS"),
            ],
            body=[
                IRFBCall(
                    instance="myLatch",
                    arguments={
                        "SET": IRVarRef(name="s"),
                        "RESET1": IRVarRef(name="r"),
                    },
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        assert "myLatch.update(s, r);" in js

    def test_sr_update_call(self):
        from st2js.codegen import generate

        program = _make_program(
            locals_=[
                IRVariable(name="s", iec_type=IECType.BOOL, is_local=True),
                IRVariable(name="r", iec_type=IECType.BOOL, is_local=True),
            ],
            fb_instances=[
                IRFBInstance(name="myLatch", fb_type="SR"),
            ],
            body=[
                IRFBCall(
                    instance="myLatch",
                    arguments={
                        "SET1": IRVarRef(name="s"),
                        "RESET": IRVarRef(name="r"),
                    },
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        assert "myLatch.update(s, r);" in js

    def test_r_trig_update_call(self):
        from st2js.codegen import generate

        program = _make_program(
            locals_=[
                IRVariable(name="clk", iec_type=IECType.BOOL, is_local=True),
            ],
            fb_instances=[
                IRFBInstance(name="myTrigger", fb_type="R_TRIG"),
            ],
            body=[
                IRFBCall(
                    instance="myTrigger",
                    arguments={"CLK": IRVarRef(name="clk")},
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        assert "myTrigger.update(clk);" in js

    def test_f_trig_update_call(self):
        from st2js.codegen import generate

        program = _make_program(
            locals_=[
                IRVariable(name="clk", iec_type=IECType.BOOL, is_local=True),
            ],
            fb_instances=[
                IRFBInstance(name="myTrigger", fb_type="F_TRIG"),
            ],
            body=[
                IRFBCall(
                    instance="myTrigger",
                    arguments={"CLK": IRVarRef(name="clk")},
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        assert "myTrigger.update(clk);" in js


class TestFBFieldAccessCodegen:
    """Test IRFieldAccess on FB emits obj.field."""

    def test_timer_q_access(self):
        from st2js.codegen import generate

        program = _make_program(
            locals_=[
                IRVariable(name="result", iec_type=IECType.BOOL, is_local=True),
            ],
            fb_instances=[
                IRFBInstance(name="myTimer", fb_type="TON",
                             constructor_args={"PT": IRLiteral(value=1000, iec_type=IECType.TIME)}),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="result"),
                    value=IRFieldAccess(
                        obj=IRVarRef(name="myTimer"),
                        field="Q",
                    ),
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        assert "result = myTimer.Q;" in js

    def test_counter_cv_access(self):
        from st2js.codegen import generate

        program = _make_program(
            locals_=[
                IRVariable(name="cnt", iec_type=IECType.INT, is_local=True),
            ],
            fb_instances=[
                IRFBInstance(name="myCounter", fb_type="CTU",
                             constructor_args={"PV": IRLiteral(value=10, iec_type=IECType.INT)}),
            ],
            body=[
                IRAssignment(
                    target=IRVarRef(name="cnt"),
                    value=IRFieldAccess(
                        obj=IRVarRef(name="myCounter"),
                        field="CV",
                    ),
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        assert "cnt = myCounter.CV;" in js


class TestFunctionBlockDeclarationCodegen:
    """Test FUNCTION_BLOCK -> JS class emission (Task 7.1)."""

    def test_empty_fb_emits_class(self):
        from st2js.codegen import generate

        program = _make_program(
            function_blocks=[
                IRFunctionBlock(
                    name="MyFB",
                    inputs=[],
                    outputs=[],
                    locals=[],
                    fb_instances=[],
                    body=[],
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        assert "class MyFB {" in js
        assert "constructor()" in js
        assert "execute()" in js

    def test_fb_with_local_vars_in_constructor(self):
        from st2js.codegen import generate

        program = _make_program(
            function_blocks=[
                IRFunctionBlock(
                    name="MyFB",
                    inputs=[],
                    outputs=[],
                    locals=[
                        IRVariable(name="counter", iec_type=IECType.INT, initial_value=0, is_local=True),
                        IRVariable(name="flag", iec_type=IECType.BOOL, is_local=True),
                    ],
                    fb_instances=[],
                    body=[],
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        assert "this.counter = 0;" in js
        assert "this.flag = false;" in js

    def test_fb_with_body_in_execute(self):
        from st2js.codegen import generate

        program = _make_program(
            function_blocks=[
                IRFunctionBlock(
                    name="MyFB",
                    inputs=[],
                    outputs=[],
                    locals=[
                        IRVariable(name="counter", iec_type=IECType.INT, initial_value=0, is_local=True),
                    ],
                    fb_instances=[],
                    body=[
                        IRAssignment(
                            target=IRVarRef(name="counter"),
                            value=IRLiteral(value=42, iec_type=IECType.INT),
                        ),
                    ],
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        # Body should be inside execute() method
        assert "execute()" in js
        # The assignment should reference this.counter
        assert "this.counter = 42;" in js

    def test_fb_class_appears_before_on_step(self):
        from st2js.codegen import generate

        program = _make_program(
            function_blocks=[
                IRFunctionBlock(
                    name="MyFB",
                    inputs=[],
                    outputs=[],
                    locals=[],
                    fb_instances=[],
                    body=[],
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        class_pos = js.index("class MyFB")
        on_step_pos = js.index("function uniset_on_step()")
        assert class_pos < on_step_pos

    def test_fb_inputs_and_outputs_in_constructor(self):
        """FB inputs and outputs become constructor properties."""
        from st2js.codegen import generate

        program = _make_program(
            function_blocks=[
                IRFunctionBlock(
                    name="MyFB",
                    inputs=[
                        IRVariable(name="SetPoint", iec_type=IECType.REAL, is_input=True),
                    ],
                    outputs=[
                        IRVariable(name="Active", iec_type=IECType.BOOL, is_output=True),
                    ],
                    locals=[],
                    fb_instances=[],
                    body=[],
                ),
            ],
        )
        mapping = _make_mapping()
        js = generate(program, mapping)
        assert "this.SetPoint = 0.0;" in js
        assert "this.Active = false;" in js
