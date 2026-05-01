#!/usr/bin/env python3
"""Transform blark AST into st2js IR dataclasses.

Walks the blark typed AST (produced by blark.parse_source_code + transform)
and produces an IRProgram containing variables, statements, and expressions
in the st2js internal representation.
"""
from __future__ import annotations

from st2js.errors import UnsupportedError
from st2js.fb_registry import get_fb_info, is_standard_fb
from st2js.ir import (
    IECType,
    IRArrayAccess,
    IRAssignment,
    IRBinaryOp,
    IRCase,
    IRContinueStatement,
    IRExitStatement,
    IRExpression,
    IRFBCall,
    IRFBInstance,
    IRFieldAccess,
    IRForLoop,
    IRFunctionBlock,
    IRFunctionCall,
    IRIfElse,
    IRLiteral,
    IRProgram,
    IRProgramCall,
    IRRepeatLoop,
    IRReturnStatement,
    IRStatement,
    IRStructField,
    IRTypeCoercion,
    IRUnaryOp,
    IRVarRef,
    IRVariable,
    IRWhileLoop,
)
from st2js.parser import GVLBlock, ParseResult

# blark AST node types (imported for isinstance checks)
from blark.transform import (
    ArraySpecification,
    ArrayTypeInitialization,
    AssignmentStatement,
    BinaryOperation,
    BitString,
    Boolean,
    CaseStatement,
    DataTypeDeclaration,
    Duration,
    FieldSelector,
    ForStatement,
    Function,
    FunctionBlock,
    GlobalVariableDeclarations,
    EnumeratedTypeDeclaration,
    EnumeratedValue,
    FunctionCall,
    FunctionCallStatement,
    IfStatement,
    InitializedStructure,
    ContinueStatement,
    ExitStatement,
    InputDeclarations,
    InputParameterAssignment,
    Integer,
    MultiElementVariable,
    OutputDeclarations,
    ParenthesizedExpression,
    Program,
    Real,
    RepeatStatement,
    ReturnStatement,
    SimpleVariable,
    StatementList,
    String,
    StructureTypeDeclaration,
    PartialSubrange,
    SubscriptList,
    UnaryOperation,
    VariableDeclarations,
    WhileStatement,
)


# Mapping from ST type name strings to IECType enum values
_TYPE_MAP: dict[str, IECType] = {
    "BOOL": IECType.BOOL,
    # Signed integers
    "SINT": IECType.SINT,
    "INT": IECType.INT,
    "DINT": IECType.DINT,
    "LINT": IECType.LINT,
    # Unsigned integers
    "USINT": IECType.USINT,
    "UINT": IECType.UINT,
    "UDINT": IECType.UDINT,
    "ULINT": IECType.ULINT,
    # Bit-string types
    "BYTE": IECType.BYTE,
    "WORD": IECType.WORD,
    "DWORD": IECType.DWORD,
    "LWORD": IECType.LWORD,
    # Floating point
    "REAL": IECType.REAL,
    "LREAL": IECType.LREAL,
    # Other
    "TIME": IECType.TIME,
    "STRING": IECType.STRING,
    "WSTRING": IECType.WSTRING,
    # Codesys-specific types
    "JSONVAR": IECType.STRING,  # JSON variable — maps to JS string/object
}


def _resolve_type_name(type_name: str) -> IECType | None:
    """Resolve a type name string to an IECType, handling parameterized types.

    Handles STRING[n], WSTRING[n], etc. by stripping the length suffix.
    """
    result = _TYPE_MAP.get(type_name)
    if result is not None:
        return result
    # Handle STRING[n] / WSTRING[n] -- strip bracket suffix
    base = type_name.split("[")[0].strip()
    if base != type_name:
        return _TYPE_MAP.get(base)
    return None


def transform(parse_result: ParseResult, program_filter: list[str] | None = None,
              main_program: str = "Main") -> IRProgram:
    """Transform a ParseResult (blark AST) into an IRProgram.

    Collects all PROGRAM declarations from the source. The main program
    (default "Main") becomes the entry point for uniset_on_step().
    Other programs are stored in secondary_programs and emitted as
    standalone JS functions callable from the main program.

    Args:
        parse_result: The result from parse_st() containing the blark typed AST.
        program_filter: If set, only convert programs whose names are in this list.
            If None, all programs are converted.
        main_program: Name of the main program (entry point). Case-insensitive.

    Returns:
        An IRProgram representing the main PROGRAM, with secondary programs
        and FUNCTION_BLOCK declarations attached.

    Raises:
        UnsupportedError: If no matching PROGRAM declaration found.
    """
    tree = parse_result.tree

    # Collect PROGRAM names first (for distinguishing program calls from FB calls)
    program_names = {
        item.name for item in tree.items if isinstance(item, Program)
    }

    transformer = ASTTransformer(parse_result.filename, program_names=program_names)

    # Register TYPE..ENUM first (structs may reference enums)
    for item in tree.items:
        if isinstance(item, DataTypeDeclaration):
            decl = item.declaration
            if isinstance(decl, EnumeratedTypeDeclaration):
                transformer.register_enum_type(str(decl.name))

    # Register TYPE..STRUCT in two passes:
    # 1) Register names only (so cross-references resolve)
    # 2) Full registration with field types
    struct_decls = [
        item.declaration for item in tree.items
        if isinstance(item, DataTypeDeclaration)
        and isinstance(item.declaration, StructureTypeDeclaration)
    ]
    for decl in struct_decls:
        transformer._struct_types[str(decl.name)] = []
    for decl in struct_decls:
        transformer.register_struct_type(decl)

    # Register user-defined FUNCTION_BLOCK names before processing anything
    # that might reference them as variable types.
    for item in tree.items:
        if isinstance(item, FunctionBlock):
            transformer.register_user_fb_type(str(item.name))

    # Collect global variables from named GVL blocks
    # Each named GVL becomes a single STRUCT-like variable (GVL_Name.VarName in ST)
    global_vars: list[IRVariable] = []
    global_fb_instances: list[IRFBInstance] = []
    gvl_field_map: dict[str, tuple[str, IECType]] = {}
    for gvl in parse_result.gvl_blocks:
        gvars, gfbs = transformer.transform_gvl_block(gvl)
        global_vars.extend(gvars)
        global_fb_instances.extend(gfbs)
        # Build flat name -> (gvl_name, type) map for unqualified access
        if gvl.name:
            for var_name, type_str, _ in gvl.variables:
                iec_type = _resolve_type_name(type_str) or IECType.INT
                gvl_field_map[var_name] = (gvl.name, iec_type)

    # Collect FUNCTION_BLOCK declarations
    function_blocks: list[IRFunctionBlock] = []
    for item in tree.items:
        if isinstance(item, FunctionBlock):
            function_blocks.append(transformer.transform_function_block(item))

    # Collect standalone FUNCTION declarations (treat as FB with execute())
    for item in tree.items:
        if isinstance(item, Function):
            try:
                function_blocks.append(transformer.transform_function_block(item))
            except Exception:
                pass  # Skip functions that can't be transformed

    # Collect all PROGRAM declarations
    all_programs: list[IRProgram] = []
    for item in tree.items:
        if isinstance(item, Program):
            prog = transformer.transform_program(item)
            all_programs.append(prog)

    # Check for standalone FUNCTION declarations (no PROGRAM/FB)
    has_functions = any(isinstance(item, Function) for item in tree.items)

    if not all_programs:
        if function_blocks or has_functions:
            # FB-only or FUNCTION-only file: create synthetic empty PROGRAM wrapper
            all_programs = [IRProgram(
                name="Main",
                inputs=[], outputs=[], locals=[],
                fb_instances=global_fb_instances, body=[],
                function_blocks=[],
                globals=global_vars,
            )]
        else:
            raise UnsupportedError(
                message="No PROGRAM, FUNCTION_BLOCK or FUNCTION declaration found in source",
                file=parse_result.filename,
            )

    # Apply program filter
    if program_filter is not None:
        filter_lower = {n.lower() for n in program_filter}
        all_programs = [p for p in all_programs if p.name.lower() in filter_lower]
        if not all_programs:
            raise UnsupportedError(
                message=f"No PROGRAM matching --programs filter found "
                        f"(requested: {', '.join(program_filter)})",
                file=parse_result.filename,
            )

    # Find main program (case-insensitive)
    main_lower = main_program.lower()
    main_prog = None
    secondary: list[IRProgram] = []
    for prog in all_programs:
        if prog.name.lower() == main_lower:
            main_prog = prog
        else:
            secondary.append(prog)

    # If no "Main" found, use the first program as main
    if main_prog is None:
        main_prog = all_programs[0]
        secondary = all_programs[1:]

    main_prog.function_blocks = function_blocks
    main_prog.secondary_programs = secondary
    # Inject GVL globals and FB instances into main program
    main_prog.globals = global_vars
    main_prog.gvl_field_map = gvl_field_map
    main_prog.fb_instances = global_fb_instances + main_prog.fb_instances
    return main_prog


# Prefixes that indicate I/O variables mapped to hardware
_INPUT_PREFIXES = ('MBI_', 'RDI_', 'RAI_')
_OUTPUT_PREFIXES = ('MBO_', 'RDO_', 'RAO_')


def resolve_io_by_prefix(program: IRProgram) -> None:
    """Find undeclared variables in program body and classify them as
    inputs/outputs based on naming convention (MBI_*, RDI_* = inputs,
    MBO_*, RDO_* = outputs).

    Mutates program in-place: adds new IRVariable entries to
    program.inputs and program.outputs.
    """
    # Build set of all known names
    known: set[str] = set()
    for var in program.inputs + program.outputs + program.locals + program.globals:
        known.add(var.name)
        if var.struct_fields:
            for f in var.struct_fields:
                known.add(f.name)
    for fb in program.fb_instances:
        known.add(fb.name)
    # GVL field aliases
    known.update(program.gvl_field_map.keys())
    # Secondary program names (they are IIFE consts)
    for sec in program.secondary_programs:
        known.add(sec.name)

    # Walk IR tree collecting all IRVarRef names
    used: set[str] = set()
    _collect_var_refs(program.body, used)
    for sec in program.secondary_programs:
        _collect_var_refs(sec.body, used)

    # Classify undeclared by prefix
    new_inputs: list[IRVariable] = []
    new_outputs: list[IRVariable] = []
    for name in sorted(used - known):
        if any(name.startswith(p) for p in _INPUT_PREFIXES):
            new_inputs.append(IRVariable(name=name, iec_type=IECType.INT, is_input=True))
        elif any(name.startswith(p) for p in _OUTPUT_PREFIXES):
            new_outputs.append(IRVariable(name=name, iec_type=IECType.INT, is_output=True))

    program.inputs.extend(new_inputs)
    program.outputs.extend(new_outputs)


def _collect_var_refs(stmts: list | None, out: set[str]) -> None:
    """Recursively collect all IRVarRef.name from a statement list."""
    if not stmts:
        return
    for stmt in stmts:
        if isinstance(stmt, IRAssignment):
            _collect_expr_refs(stmt.target, out)
            _collect_expr_refs(stmt.value, out)
        elif isinstance(stmt, IRIfElse):
            _collect_expr_refs(stmt.condition, out)
            _collect_var_refs(stmt.then_body, out)
            _collect_var_refs(stmt.else_body, out)
            for elif_ in stmt.elsif_branches:
                _collect_expr_refs(elif_[0], out)
                _collect_var_refs(elif_[1], out)
        elif isinstance(stmt, IRCase):
            _collect_expr_refs(stmt.selector, out)
            for _, body in stmt.branches:
                _collect_var_refs(body, out)
            _collect_var_refs(stmt.else_body, out)
        elif isinstance(stmt, IRForLoop):
            out.add(stmt.var)
            _collect_expr_refs(stmt.start, out)
            _collect_expr_refs(stmt.end, out)
            if stmt.step:
                _collect_expr_refs(stmt.step, out)
            _collect_var_refs(stmt.body, out)
        elif isinstance(stmt, IRWhileLoop):
            _collect_expr_refs(stmt.condition, out)
            _collect_var_refs(stmt.body, out)
        elif isinstance(stmt, IRRepeatLoop):
            _collect_expr_refs(stmt.condition, out)
            _collect_var_refs(stmt.body, out)
        elif isinstance(stmt, IRFBCall):
            for arg in stmt.arguments.values():
                _collect_expr_refs(arg, out)


def _collect_expr_refs(expr, out: set[str]) -> None:
    """Recursively collect IRVarRef names from an expression."""
    if isinstance(expr, IRVarRef):
        out.add(expr.name)
    elif isinstance(expr, IRBinaryOp):
        _collect_expr_refs(expr.left, out)
        _collect_expr_refs(expr.right, out)
    elif isinstance(expr, IRUnaryOp):
        _collect_expr_refs(expr.operand, out)
    elif isinstance(expr, IRFieldAccess):
        _collect_expr_refs(expr.obj, out)
    elif isinstance(expr, IRArrayAccess):
        _collect_expr_refs(expr.array, out)
        _collect_expr_refs(expr.index, out)
    elif isinstance(expr, IRFunctionCall):
        for arg in expr.arguments:
            _collect_expr_refs(arg, out)
    elif isinstance(expr, IRTypeCoercion):
        _collect_expr_refs(expr.expr, out)


class ASTTransformer:
    """Walks blark AST nodes and produces IR dataclasses.

    Handles PROGRAM and FUNCTION_BLOCK declarations, VAR sections
    (INPUT/OUTPUT/local), FB instance detection, FB call statements,
    field access (obj.field), assignment, IF/ELSIF/ELSE,
    CASE, FOR, WHILE, REPEAT, binary/unary expressions,
    variable references, literals, and Duration (TIME) literals.
    """

    def __init__(self, filename: str = "<stdin>", program_names: set[str] | None = None):
        self.filename = filename
        # Registered STRUCT type definitions: type_name -> list of IRStructField
        self._struct_types: dict[str, list[IRStructField]] = {}
        # Known PROGRAM names (for distinguishing program calls from FB calls)
        self._program_names: set[str] = program_names or set()
        # User-defined FUNCTION_BLOCK names (treated as FB instances in variables)
        self._user_fb_types: set[str] = set()
        # User-defined ENUM type names (treated as INT variables)
        self._enum_types: set[str] = set()

    def register_user_fb_type(self, name: str) -> None:
        """Register a user-defined FUNCTION_BLOCK type name."""
        self._user_fb_types.add(name)

    def register_enum_type(self, name: str) -> None:
        """Register a user-defined ENUM type name."""
        self._enum_types.add(name)

    def register_struct_type(self, decl: StructureTypeDeclaration) -> None:
        """Register a STRUCT type declaration for later use in variable resolution."""
        type_name = str(decl.name)
        fields: list[IRStructField] = []
        for elem in decl.declarations:
            init = elem.init
            try:
                if isinstance(init, InitializedStructure):
                    field_type_name = str(init.name)
                elif isinstance(init.spec, ArraySpecification):
                    field_type_name = "ARRAY"
                else:
                    field_type_name = str(init.spec.type)
            except (AttributeError, TypeError):
                field_type_name = "UNKNOWN"
            field_iec_type = _resolve_type_name(field_type_name)
            if field_iec_type is None:
                # Check if it's a known enum, struct, or array
                if field_type_name in self._enum_types:
                    field_iec_type = IECType.STRING
                elif field_type_name in self._struct_types or field_type_name == "ARRAY":
                    field_iec_type = IECType.INT  # nested struct/array → opaque INT in JS
                else:
                    import warnings
                    warnings.warn(
                        f"Unknown struct field type '{field_type_name}' in STRUCT '{type_name}', "
                        f"treated as INT",
                        stacklevel=2,
                    )
                    field_iec_type = IECType.INT
            initial_value = self._extract_initial_value(init.value)
            for declared_var in elem.variables:
                field_name = str(declared_var.variable.name)
                fields.append(IRStructField(
                    name=field_name,
                    iec_type=field_iec_type,
                    initial_value=initial_value,
                ))
        self._struct_types[type_name] = fields

    def transform_gvl_block(
        self, gvl: GVLBlock
    ) -> tuple[list[IRVariable], list[IRFBInstance]]:
        """Transform a named GVL block into a single STRUCT-like IRVariable.

        Each named GVL becomes one variable: `let GVL_Name = { field1: val, ... };`
        FB-typed variables within the GVL are emitted as separate FB instances.
        """
        fields: list[IRStructField] = []
        fb_instances: list[IRFBInstance] = []

        for var_name, type_str, init_str in gvl.variables:
            # Handle ARRAY type: "ARRAY [1..20] OF BOOL" -> array field
            if type_str.startswith('ARRAY'):
                import re
                m = re.match(r'ARRAY\s*\[(\d+)\.\.(\d+)\]\s*OF\s+(\w+)', type_str)
                if m:
                    lower = int(m.group(1))
                    upper = int(m.group(2))
                    base_type_str = m.group(3)
                    base_iec = _resolve_type_name(base_type_str) or IECType.INT
                    size = upper - lower + 1
                    default = {
                        IECType.BOOL: "false", IECType.REAL: "0.0",
                        IECType.LREAL: "0.0", IECType.STRING: '""',
                    }.get(base_iec, "0")
                    # Store JS array literal as initial_value string
                    array_literal = "[" + ", ".join([default] * size) + "]"
                    fields.append(IRStructField(
                        name=var_name,
                        iec_type=IECType.INT,  # placeholder type
                        initial_value=f"__raw:{array_literal}",
                    ))
                    continue

            # Check if this is a known FB type — add as property of GVL object
            if is_standard_fb(type_str) or type_str in self._user_fb_types:
                js_type = type_str.replace('.', '_')
                fields.append(IRStructField(
                    name=var_name,
                    iec_type=IECType.INT,
                    initial_value=f"__raw:new {js_type}()",
                ))
                continue

            # Check if this is an unknown external type (dotted name like OwenTypes.TRG_Buzzer)
            iec_type = _resolve_type_name(type_str)
            if iec_type is None and '.' in type_str:
                import warnings
                warnings.warn(
                    f"Unknown type '{type_str}' in GVL '{gvl.name}' treated as FB instance",
                    stacklevel=2,
                )
                js_type = type_str.replace('.', '_')
                fields.append(IRStructField(
                    name=var_name,
                    iec_type=IECType.INT,
                    initial_value=f"__raw:new {js_type}()",
                ))
                continue

            if iec_type is None:
                # Unknown non-dotted type — check structs
                if type_str in self._struct_types:
                    fields.append(IRStructField(
                        name=var_name,
                        iec_type=IECType.STRUCT,
                        initial_value=None,
                    ))
                else:
                    iec_type = IECType.INT  # fallback

            if iec_type is not None:
                initial_value = self._parse_init_str(init_str, iec_type) if init_str else None
                fields.append(IRStructField(
                    name=var_name,
                    iec_type=iec_type,
                    initial_value=initial_value,
                ))

        variables: list[IRVariable] = []
        if fields and gvl.name:
            variables.append(IRVariable(
                name=gvl.name,
                iec_type=IECType.STRUCT,
                struct_fields=fields,
                is_local=False,
            ))
        elif fields:
            # Unnamed GVL — emit as individual variables
            for f in fields:
                variables.append(IRVariable(
                    name=f.name,
                    iec_type=f.iec_type,
                    initial_value=f.initial_value,
                    is_local=False,
                ))

        return variables, fb_instances

    def _parse_init_str(self, init_str: str, iec_type: IECType):
        """Parse an initial value string into a Python value."""
        if not init_str:
            return None
        s = init_str.strip()
        if iec_type == IECType.BOOL:
            return s.upper() == 'TRUE'
        if iec_type == IECType.REAL or iec_type == IECType.LREAL:
            try:
                return float(s)
            except ValueError:
                return 0.0
        if iec_type == IECType.STRING:
            return s.strip("'\"")
        try:
            return int(s)
        except ValueError:
            return 0

    def transform_program(self, node: Program) -> IRProgram:
        """Transform a blark Program node into an IRProgram."""
        inputs: list[IRVariable] = []
        outputs: list[IRVariable] = []
        locals_: list[IRVariable] = []
        fb_instances: list[IRFBInstance] = []

        for decl in node.declarations:
            if isinstance(decl, InputDeclarations):
                inputs.extend(self._transform_var_items(decl.items, is_input=True))
            elif isinstance(decl, OutputDeclarations):
                outputs.extend(self._transform_var_items(decl.items, is_output=True))
            elif isinstance(decl, VariableDeclarations):
                vars_, fbs = self._transform_var_items_with_fb(decl.items, is_local=True)
                locals_.extend(vars_)
                fb_instances.extend(fbs)

        body = self._transform_statement_list(node.body) if node.body else []

        # Post-process: extract constructor args from FB calls and store in FB instances
        self._extract_constructor_args(fb_instances, body)

        return IRProgram(
            name=str(node.name),
            inputs=inputs,
            outputs=outputs,
            locals=locals_,
            fb_instances=fb_instances,
            body=body,
            function_blocks=[],
        )

    def transform_function_block(self, node: FunctionBlock) -> IRFunctionBlock:
        """Transform a blark FunctionBlock node into an IRFunctionBlock."""
        inputs: list[IRVariable] = []
        outputs: list[IRVariable] = []
        locals_: list[IRVariable] = []
        fb_instances: list[IRFBInstance] = []

        for decl in node.declarations:
            if isinstance(decl, InputDeclarations):
                inputs.extend(self._transform_var_items(decl.items, is_input=True))
            elif isinstance(decl, OutputDeclarations):
                outputs.extend(self._transform_var_items(decl.items, is_output=True))
            elif isinstance(decl, VariableDeclarations):
                vars_, fbs = self._transform_var_items_with_fb(decl.items, is_local=True)
                locals_.extend(vars_)
                fb_instances.extend(fbs)

        body: list[IRStatement] = []
        if node.body is not None:
            if isinstance(node.body, StatementList):
                body = self._transform_statement_list(node.body)
            elif hasattr(node.body, 'statements'):
                body = self._transform_statement_list(node.body)

        return IRFunctionBlock(
            name=str(node.name),
            inputs=inputs,
            outputs=outputs,
            locals=locals_,
            fb_instances=fb_instances,
            body=body,
        )

    def _transform_var_items_with_fb(
        self,
        items: list,
        is_local: bool = False,
    ) -> tuple[list[IRVariable], list[IRFBInstance]]:
        """Transform VAR items, separating regular variables from FB instances.

        Returns:
            A tuple of (regular_variables, fb_instances).
        """
        variables: list[IRVariable] = []
        fb_instances: list[IRFBInstance] = []

        for item in items:
            init = item.init

            # Handle ARRAY declarations
            if isinstance(init, ArrayTypeInitialization):
                variables.extend(self._transform_array_var(
                    item, init, is_local=is_local,
                ))
                continue

            # Handle InitializedStructure: e.g. ST_TypeName := (field := value, ...)
            if isinstance(init, InitializedStructure):
                type_name = str(init.name)
                # Treat as struct or FB instance depending on what we know
                if is_standard_fb(type_name) or type_name in self._user_fb_types:
                    for declared_var in item.variables:
                        var_name = str(declared_var.variable.name)
                        fb_instances.append(IRFBInstance(name=var_name, fb_type=type_name))
                elif type_name in self._struct_types:
                    for declared_var in item.variables:
                        var_name = str(declared_var.variable.name)
                        fields_copy = [
                            IRStructField(name=f.name, iec_type=f.iec_type, initial_value=f.initial_value)
                            for f in self._struct_types[type_name]
                        ]
                        variables.append(IRVariable(
                            name=var_name, iec_type=IECType.STRUCT,
                            struct_type_name=type_name, struct_fields=fields_copy,
                            is_local=is_local,
                        ))
                else:
                    # Unknown initialized struct type — treat as FB
                    import warnings
                    warnings.warn(f"Unknown initialized type '{type_name}' treated as FB instance")
                    for declared_var in item.variables:
                        var_name = str(declared_var.variable.name)
                        fb_instances.append(IRFBInstance(name=var_name, fb_type=type_name))
                continue

            type_name = str(init.spec.type)

            # Check if this is a known enum type -> treat as STRING variable
            if type_name in self._enum_types:
                for declared_var in item.variables:
                    var_name = str(declared_var.variable.name)
                    variables.append(IRVariable(
                        name=var_name,
                        iec_type=IECType.STRING,
                        initial_value="",
                        is_local=is_local,
                    ))
                continue

            # Check if this is a known FB type (standard or user-defined)
            if is_standard_fb(type_name) or type_name in self._user_fb_types:
                for declared_var in item.variables:
                    var_name = str(declared_var.variable.name)
                    fb_instances.append(IRFBInstance(
                        name=var_name,
                        fb_type=type_name,
                    ))
            elif type_name in self._struct_types:
                # Known STRUCT type
                for declared_var in item.variables:
                    var_name = str(declared_var.variable.name)
                    fields_copy = [
                        IRStructField(name=f.name, iec_type=f.iec_type, initial_value=f.initial_value)
                        for f in self._struct_types[type_name]
                    ]
                    variables.append(IRVariable(
                        name=var_name,
                        iec_type=IECType.STRUCT,
                        struct_type_name=type_name,
                        struct_fields=fields_copy,
                        is_local=is_local,
                    ))
            else:
                iec_type = _resolve_type_name(type_name)
                if iec_type is None:
                    # Unknown type -- assume it is an external/library FB
                    # (e.g. Codesys library FBs like BLINK, LIMITALARM, etc.)
                    import warnings
                    warnings.warn(
                        f"Unknown type '{type_name}' treated as FB instance "
                        f"(not in _TYPE_MAP or known struct/FB types)",
                        stacklevel=2,
                    )
                    for declared_var in item.variables:
                        var_name = str(declared_var.variable.name)
                        fb_instances.append(IRFBInstance(
                            name=var_name,
                            fb_type=type_name,
                        ))
                    continue

                initial_value = self._extract_initial_value(init.value)

                for declared_var in item.variables:
                    var_name = str(declared_var.variable.name)
                    variables.append(IRVariable(
                        name=var_name,
                        iec_type=iec_type,
                        initial_value=initial_value,
                        is_local=is_local,
                    ))

        return variables, fb_instances

    def _transform_var_items(
        self,
        items: list,
        is_input: bool = False,
        is_output: bool = False,
        is_local: bool = False,
    ) -> list[IRVariable]:
        """Transform a list of VariableOneInitDeclaration into IRVariables."""
        result: list[IRVariable] = []
        for item in items:
            # Each item has .variables (list of DeclaredVariable) and .init (TypeInitialization)
            init = item.init

            # Handle ARRAY declarations
            if isinstance(init, ArrayTypeInitialization):
                result.extend(self._transform_array_var(
                    item, init, is_input=is_input, is_output=is_output, is_local=is_local,
                ))
                continue

            # Handle InitializedStructure
            if isinstance(init, InitializedStructure):
                type_name = str(init.name)
                if type_name in self._struct_types:
                    for declared_var in item.variables:
                        var_name = str(declared_var.variable.name)
                        fields_copy = [
                            IRStructField(name=f.name, iec_type=f.iec_type, initial_value=f.initial_value)
                            for f in self._struct_types[type_name]
                        ]
                        result.append(IRVariable(
                            name=var_name, iec_type=IECType.STRUCT,
                            struct_type_name=type_name, struct_fields=fields_copy,
                            is_input=is_input, is_output=is_output, is_local=is_local,
                        ))
                else:
                    for declared_var in item.variables:
                        var_name = str(declared_var.variable.name)
                        result.append(IRVariable(
                            name=var_name, iec_type=IECType.INT,
                            is_input=is_input, is_output=is_output, is_local=is_local,
                        ))
                continue

            type_name = str(init.spec.type)
            iec_type = _resolve_type_name(type_name)

            # Check if it is a known ENUM type -> treat as STRING
            if iec_type is None and type_name in self._enum_types:
                for declared_var in item.variables:
                    var_name = str(declared_var.variable.name)
                    result.append(IRVariable(
                        name=var_name,
                        iec_type=IECType.STRING,
                        initial_value="",
                        is_input=is_input,
                        is_output=is_output,
                        is_local=is_local,
                    ))
                continue

            # Check if it is a known STRUCT type
            if iec_type is None and type_name in self._struct_types:
                for declared_var in item.variables:
                    var_name = str(declared_var.variable.name)
                    fields_copy = [
                        IRStructField(name=f.name, iec_type=f.iec_type, initial_value=f.initial_value)
                        for f in self._struct_types[type_name]
                    ]
                    result.append(IRVariable(
                        name=var_name,
                        iec_type=IECType.STRUCT,
                        struct_type_name=type_name,
                        struct_fields=fields_copy,
                        is_input=is_input,
                        is_output=is_output,
                        is_local=is_local,
                    ))
                continue

            if iec_type is None:
                # Unknown type in a non-FB context (VAR_INPUT/VAR_OUTPUT) --
                # treat as opaque FB/struct type; emit as plain object.
                import warnings
                warnings.warn(
                    f"Unknown type '{type_name}' in VAR section treated as "
                    f"FB_INSTANCE placeholder",
                    stacklevel=2,
                )
                for declared_var in item.variables:
                    var_name = str(declared_var.variable.name)
                    result.append(IRVariable(
                        name=var_name,
                        iec_type=IECType.STRUCT,
                        struct_type_name=type_name,
                        struct_fields=[],
                        is_input=is_input,
                        is_output=is_output,
                        is_local=is_local,
                    ))
                continue

            initial_value = self._extract_initial_value(init.value)

            for declared_var in item.variables:
                var_name = str(declared_var.variable.name)
                result.append(IRVariable(
                    name=var_name,
                    iec_type=iec_type,
                    initial_value=initial_value,
                    is_input=is_input,
                    is_output=is_output,
                    is_local=is_local,
                ))
        return result

    def _transform_array_var(
        self,
        item,
        init: ArrayTypeInitialization,
        is_input: bool = False,
        is_output: bool = False,
        is_local: bool = False,
    ) -> list[IRVariable]:
        """Transform an ARRAY variable declaration."""
        spec = init.spec  # ArraySpecification
        # Get element type — may be SimpleType (.type_name) or nested ArraySpecification
        try:
            elem_type_name = str(spec.type.type_name)
        except AttributeError:
            elem_type_name = str(spec.type) if spec.type else "INT"
        elem_iec_type = _resolve_type_name(elem_type_name)
        if elem_iec_type is None:
            # Check if the array element type is a known struct
            if elem_type_name in self._struct_types:
                elem_iec_type = IECType.STRUCT
            else:
                # Treat as opaque struct (empty fields)
                import warnings
                warnings.warn(
                    f"Unknown array element type '{elem_type_name}' treated "
                    f"as STRUCT",
                    stacklevel=2,
                )
                elem_iec_type = IECType.STRUCT

        # Get bounds from first subrange
        subrange = spec.subranges[0]

        def _eval_bound(node):
            """Evaluate a subrange bound (may be Integer or UnaryOperation like -1)."""
            if hasattr(node, 'value'):
                return int(str(node.value))
            if isinstance(node, UnaryOperation):
                val = _eval_bound(node.expr)
                return -val if str(node.op) == '-' else val
            return int(str(node))

        try:
            lower = _eval_bound(subrange.start)
            upper = _eval_bound(subrange.stop)
        except (ValueError, AttributeError, TypeError):
            # Non-constant bounds (e.g. SIZEOF(...)) — use defaults
            import warnings
            warnings.warn(
                f"Cannot evaluate array bounds (compile-time expression), using default size 100",
                stacklevel=2,
            )
            lower = 0
            upper = 99
        size = upper - lower + 1

        result: list[IRVariable] = []
        for declared_var in item.variables:
            var_name = str(declared_var.variable.name)
            result.append(IRVariable(
                name=var_name,
                iec_type=IECType.ARRAY,
                array_element_type=elem_iec_type,
                array_size=size,
                array_lower_bound=lower,
                is_input=is_input,
                is_output=is_output,
                is_local=is_local,
            ))
        return result

    def _extract_initial_value(self, value) -> object | None:
        """Extract a Python value from a blark initial value node."""
        if value is None:
            return None
        if isinstance(value, Integer):
            return int(str(value.value))
        if isinstance(value, Real):
            return float(str(value.value))
        if isinstance(value, Boolean):
            return str(value.value).upper() == "TRUE"
        if isinstance(value, String):
            # Strip ST single quotes: 'hello' -> hello
            raw = str(value.value)
            if raw.startswith("'") and raw.endswith("'"):
                raw = raw[1:-1]
            return raw
        if isinstance(value, Duration):
            return self._transform_duration(value).value
        return None

    def _transform_statement_list(self, stmt_list: StatementList) -> list[IRStatement]:
        """Transform a blark StatementList into a list of IR statements."""
        result: list[IRStatement] = []
        for stmt in stmt_list.statements:
            result.append(self._transform_statement(stmt))
        return result

    def _transform_statement(self, stmt) -> IRStatement:
        """Transform a single blark statement into an IR statement."""
        if isinstance(stmt, AssignmentStatement):
            return self._transform_assignment(stmt)
        if isinstance(stmt, FunctionCallStatement):
            call_name = str(stmt.name.name) if isinstance(stmt.name, SimpleVariable) else str(stmt.name)
            if call_name in self._program_names:
                return IRProgramCall(program_name=call_name)
            # Check for action call: Parent.Action() -> Parent_Action
            qualified = call_name.replace(".", "_")
            if qualified != call_name and qualified in self._program_names:
                return IRProgramCall(program_name=qualified)
            return self._transform_fb_call(stmt)
        if isinstance(stmt, IfStatement):
            return self._transform_if(stmt)
        if isinstance(stmt, CaseStatement):
            return self._transform_case(stmt)
        if isinstance(stmt, ForStatement):
            return self._transform_for(stmt)
        if isinstance(stmt, WhileStatement):
            return self._transform_while(stmt)
        if isinstance(stmt, RepeatStatement):
            return self._transform_repeat(stmt)
        if isinstance(stmt, ExitStatement):
            return IRExitStatement()
        if isinstance(stmt, ContinueStatement):
            return IRContinueStatement()
        if isinstance(stmt, ReturnStatement):
            return IRReturnStatement()
        raise UnsupportedError(
            message=f"Unsupported statement type: {type(stmt).__name__}",
            file=self.filename,
        )

    def _transform_fb_call(self, stmt: FunctionCallStatement) -> IRFBCall:
        """Transform a blark FunctionCallStatement into an IRFBCall.

        Extracts the instance name and named parameters from the call.
        """
        instance_name = str(stmt.name.name) if isinstance(stmt.name, SimpleVariable) else str(stmt.name)

        arguments: dict[str, IRExpression] = {}
        for param in stmt.parameters:
            if isinstance(param, InputParameterAssignment) and param.name is not None:
                param_name = str(param.name.name) if isinstance(param.name, SimpleVariable) else str(param.name)
                param_value = self._transform_expression(param.value)
                arguments[param_name] = param_value

        return IRFBCall(instance=instance_name, arguments=arguments)

    def _extract_constructor_args(
        self,
        fb_instances: list[IRFBInstance],
        body: list[IRStatement],
    ) -> None:
        """Extract constructor arguments from FB calls and store in FB instances.

        For standard FBs, constructor args (PT for timers, PV for counters) are
        specified as named parameters in the first FB call. These are extracted
        from the IRFBCall arguments and stored in the IRFBInstance constructor_args.
        The constructor args are also removed from the IRFBCall arguments so that
        only update() params remain.
        """
        fb_map = {fb.name: fb for fb in fb_instances}

        for stmt in body:
            if not isinstance(stmt, IRFBCall):
                continue

            fb_inst = fb_map.get(stmt.instance)
            if fb_inst is None:
                continue

            fb_info = get_fb_info(fb_inst.fb_type)
            if fb_info is None:
                continue

            # Extract constructor args from the call arguments
            constructor_param_names = {name for name, _ in fb_info.constructor_args}
            for param_name in list(stmt.arguments.keys()):
                if param_name in constructor_param_names:
                    fb_inst.constructor_args[param_name] = stmt.arguments.pop(param_name)

    def _transform_assignment(self, stmt: AssignmentStatement) -> IRAssignment:
        """Transform a blark AssignmentStatement into an IRAssignment."""
        # Assignment target is the first variable in the list
        target = self._transform_expression(stmt.variables[0])
        value = self._transform_expression(stmt.expression)
        return IRAssignment(target=target, value=value)

    def _transform_if(self, stmt: IfStatement) -> IRIfElse:
        """Transform a blark IfStatement into an IRIfElse."""
        condition = self._transform_expression(stmt.if_expression)
        then_body = self._transform_statement_list(stmt.statements)

        elsif_branches: list[tuple[IRExpression, list[IRStatement]]] = []
        for elif_clause in stmt.else_ifs:
            elif_cond = self._transform_expression(elif_clause.if_expression)
            elif_body = self._transform_statement_list(elif_clause.statements)
            elsif_branches.append((elif_cond, elif_body))

        else_body: list[IRStatement] | None = None
        if stmt.else_clause is not None:
            else_body = self._transform_statement_list(stmt.else_clause.statements)

        return IRIfElse(
            condition=condition,
            then_body=then_body,
            elsif_branches=elsif_branches,
            else_body=else_body,
        )

    def _transform_case(self, stmt: CaseStatement) -> IRCase:
        """Transform a blark CaseStatement into an IRCase."""
        selector = self._transform_expression(stmt.expression)

        branches: list[tuple[list[IRExpression], list[IRStatement]]] = []
        for case_element in stmt.cases:
            case_values = []
            for match in case_element.matches:
                if isinstance(match, PartialSubrange):
                    # CASE range: 1..5 → expand to individual values
                    try:
                        start = int(str(match.start.value)) if hasattr(match.start, 'value') else int(str(match.start))
                        stop = int(str(match.stop.value)) if hasattr(match.stop, 'value') else int(str(match.stop))
                        for v in range(start, stop + 1):
                            case_values.append(IRLiteral(value=v, iec_type=IECType.INT))
                    except (ValueError, AttributeError):
                        # Non-constant range — emit start as fallback
                        case_values.append(self._transform_expression(match.start))
                else:
                    case_values.append(self._transform_expression(match))
            case_body = (
                self._transform_statement_list(case_element.statements)
                if case_element.statements
                else []
            )
            branches.append((case_values, case_body))

        else_body: list[IRStatement] | None = None
        if stmt.else_clause is not None:
            else_body = self._transform_statement_list(stmt.else_clause.statements)

        return IRCase(
            selector=selector,
            branches=branches,
            else_body=else_body,
        )

    def _transform_for(self, stmt: ForStatement) -> IRForLoop:
        """Transform a blark ForStatement into an IRForLoop."""
        var_name = str(stmt.control.name)
        start = self._transform_expression(stmt.from_)
        end = self._transform_expression(stmt.to)
        step = self._transform_expression(stmt.step) if stmt.step is not None else None
        body = self._transform_statement_list(stmt.statements)

        return IRForLoop(
            var=var_name,
            start=start,
            end=end,
            step=step,
            body=body,
        )

    def _transform_while(self, stmt: WhileStatement) -> IRWhileLoop:
        """Transform a blark WhileStatement into an IRWhileLoop."""
        condition = self._transform_expression(stmt.expression)
        body = self._transform_statement_list(stmt.statements)

        return IRWhileLoop(
            condition=condition,
            body=body,
        )

    def _transform_repeat(self, stmt: RepeatStatement) -> IRRepeatLoop:
        """Transform a blark RepeatStatement into an IRRepeatLoop."""
        condition = self._transform_expression(stmt.expression)
        body = self._transform_statement_list(stmt.statements)

        return IRRepeatLoop(
            condition=condition,
            body=body,
        )

    def _transform_expression(self, expr) -> IRExpression:
        """Transform a blark expression node into an IR expression."""
        if isinstance(expr, SimpleVariable):
            return IRVarRef(name=str(expr.name))

        if isinstance(expr, MultiElementVariable):
            return self._transform_multi_element_variable(expr)

        if isinstance(expr, Integer):
            val_str = str(expr.value)
            try:
                val = int(val_str)
            except ValueError:
                try:
                    val = int(val_str, 16)  # Hex: 16#2D → "2D"
                except ValueError:
                    val = 0
            return IRLiteral(value=val, iec_type=IECType.INT)

        if isinstance(expr, BitString):
            # BYTE#16#2D, WORD#16#FFFF etc.
            try:
                val = int(str(expr.value), int(str(expr.base)) if expr.base else 16)
            except (ValueError, TypeError):
                val = 0
            return IRLiteral(value=val, iec_type=IECType.INT)

        if isinstance(expr, Real):
            return IRLiteral(value=float(str(expr.value)), iec_type=IECType.REAL)

        if isinstance(expr, Boolean):
            val = str(expr.value).upper() == "TRUE"
            return IRLiteral(value=val, iec_type=IECType.BOOL)

        if isinstance(expr, Duration):
            return self._transform_duration(expr)

        if isinstance(expr, String):
            # Strip ST single quotes: 'hello' -> hello
            raw = str(expr.value)
            if raw.startswith("'") and raw.endswith("'"):
                raw = raw[1:-1]
            return IRLiteral(value=raw, iec_type=IECType.STRING)

        if isinstance(expr, BinaryOperation):
            left = self._transform_expression(expr.left)
            right = self._transform_expression(expr.right)
            op = str(expr.op)
            return IRBinaryOp(op=op, left=left, right=right)

        if isinstance(expr, UnaryOperation):
            operand = self._transform_expression(expr.expr)
            op = str(expr.op)
            return IRUnaryOp(op=op, operand=operand)

        if isinstance(expr, ParenthesizedExpression):
            return self._transform_expression(expr.expr)

        if isinstance(expr, FunctionCall):
            return self._transform_function_call_expr(expr)

        if isinstance(expr, EnumeratedValue):
            # Emit as "TYPE.VALUE" string literal for enum comparisons
            type_name = str(expr.type_name) if expr.type_name else ""
            name = str(expr.name)
            if type_name:
                return IRLiteral(value=f"{type_name}.{name}", iec_type=IECType.STRING)
            return IRLiteral(value=name, iec_type=IECType.STRING)

        raise UnsupportedError(
            message=f"Unsupported expression type: {type(expr).__name__}",
            file=self.filename,
        )

    def _transform_function_call_expr(self, expr: FunctionCall) -> IRFunctionCall:
        """Transform a blark FunctionCall (expression) into an IRFunctionCall."""
        func_name = str(expr.name)
        arguments: list[tuple[str | None, IRExpression]] = []
        if expr.parameters:
            for param in expr.parameters:
                if isinstance(param, InputParameterAssignment):
                    param_name = str(param.name.name) if isinstance(param.name, SimpleVariable) else str(param.name)
                    value = self._transform_expression(param.value)
                    arguments.append((param_name, value))
                else:
                    # Positional parameter
                    value = self._transform_expression(param)
                    arguments.append((None, value))
        return IRFunctionCall(name=func_name, arguments=arguments)

    def _transform_multi_element_variable(self, expr: MultiElementVariable) -> IRExpression:
        """Transform a multi-element variable (e.g. myTimer.Q, arr[i]) into field/array access."""
        obj = IRVarRef(name=str(expr.name.name))

        # Chain accesses: a.b.c -> IRFieldAccess(IRFieldAccess(a, b), c)
        # a[i] -> IRArrayAccess(a, i)
        result: IRExpression = obj
        for element in expr.elements:
            if isinstance(element, FieldSelector):
                field_name = str(element.field.name) if hasattr(element.field, 'name') else str(element.field)
                result = IRFieldAccess(obj=result, field=field_name)
            elif isinstance(element, SubscriptList):
                # Array access: arr[index]
                index = self._transform_expression(element.subscripts[0])
                result = IRArrayAccess(array=result, index=index)
            else:
                raise UnsupportedError(
                    message=f"Unsupported multi-element access: {type(element).__name__}",
                    file=self.filename,
                )
        return result

    def _transform_duration(self, expr: Duration) -> IRLiteral:
        """Transform a blark Duration literal into an IRLiteral with millisecond value."""
        ms = 0
        if expr.days is not None:
            ms += int(str(expr.days)) * 86_400_000
        if expr.hours is not None:
            ms += int(str(expr.hours)) * 3_600_000
        if expr.minutes is not None:
            ms += int(str(expr.minutes)) * 60_000
        if expr.seconds is not None:
            ms += int(float(str(expr.seconds)) * 1000)
        if expr.milliseconds is not None:
            ms += int(str(expr.milliseconds))
        if expr.negative:
            ms = -ms
        return IRLiteral(value=ms, iec_type=IECType.TIME)
