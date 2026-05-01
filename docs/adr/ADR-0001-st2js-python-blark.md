# ADR-0001: Python + blark for ST-to-JS Converter

## Status

Proposed

## Context

The UniSet2 JScript extension now includes `uniset2-iec61131.js` with 10 standard IEC 61131-3 function blocks. To enable PLC engineers to write control logic in IEC 61131-3 Structured Text (ST) and deploy it on the UniSet JScript runtime, we need a converter tool (`st2js`) that parses ST source and emits JavaScript compatible with the `uniset_on_step()` cyclic execution model.

Key constraints:
- IEC 61131-3 ST grammar is complex (PROGRAM, FUNCTION_BLOCK, VAR sections, control flow, type system)
- The converter is a standalone offline build tool, not a runtime component
- UniSet2 is a C++ project with autotools build system; adding Python tooling is a new direction
- The generated JS must integrate with QuickJS engine (ES2020 subset, no Node.js APIs)

## Decision

Use **Python** as the implementation language with the **blark** library (lark-parser based) for ST parsing.

### Decision Details

| Item | Content |
|------|---------|
| **Decision** | Implement st2js as a Python package using blark for ST parsing and a custom code generator for JS emission |
| **Why now** | The IEC 61131-3 FB library is complete; the converter is the next step to make it usable by PLC engineers |
| **Why this** | blark provides a production-quality IEC 61131-3 grammar with Python dataclass AST, eliminating months of grammar development; Python enables rapid development of the AST-to-JS transformation pipeline |
| **Known unknowns** | blark targets Beckhoff TwinCAT dialect -- pure IEC 61131-3 subset compatibility needs validation; some grammar rules may need local patching |
| **Kill criteria** | If blark's grammar fundamentally cannot parse pure IEC 61131-3 constructs (PROGRAM, standard FB calls) and patching requires rewriting >30% of the grammar |

## Rationale

### Options Considered

1. **Custom lark grammar (Python)**
   - Pros: Full control over grammar rules; no Beckhoff-specific baggage; Python ecosystem for tooling
   - Cons: IEC 61131-3 grammar is ~2000 rules; estimated 4-6 weeks for grammar alone; high risk of subtle parsing errors; no community validation

2. **C++ with custom parser**
   - Pros: Same language as UniSet2 core; no new runtime dependency; integrates with autotools build
   - Cons: No mature C++ IEC 61131-3 parser exists; C++ is significantly slower for AST manipulation/code generation tooling; estimated 8-12 weeks; harder to iterate on grammar issues
   - Rejected: Development cost is prohibitive for an offline build tool

3. **JavaScript/Node.js converter**
   - Pros: Same target language; could potentially run in QuickJS itself
   - Cons: No IEC 61131-3 parser in JS ecosystem; QuickJS lacks Node.js standard library (no filesystem, no YAML parsing); would need Node.js as build dependency anyway
   - Rejected: No parser availability; QuickJS limitations make self-hosting impractical

4. **Python + blark (Selected)**
   - Pros: Production-quality IEC 61131-3 grammar (lark Earley parser); Python dataclass AST with round-trip capability; active maintenance (last release Feb 2025); rich Python ecosystem for YAML config, CLI, testing; rapid development cycle (estimated 2-3 weeks)
   - Cons: New Python dependency in a C++ project; blark includes Beckhoff-specific extensions that must be filtered; blark is a relatively niche library

### Comparison

| Evaluation Axis | Custom lark | C++ parser | JS/Node.js | Python + blark |
|----------------|-------------|------------|------------|----------------|
| Grammar readiness | 4-6 weeks | 8-12 weeks | N/A | Ready (patch only) |
| Implementation effort | 6-8 weeks | 10-14 weeks | N/A | 2-3 weeks |
| IEC 61131-3 coverage | Medium | Medium | None | High |
| Maintenance burden | High | High | N/A | Low (upstream) |
| New dependencies | Python, lark | None | Node.js | Python, blark |
| Ecosystem fit | Fair | Good | Poor | Fair |

## Consequences

### Positive Consequences

- Leverages a validated IEC 61131-3 grammar, drastically reducing parser development time
- Python dataclass AST enables clean visitor/transformer pattern for code generation
- Python ecosystem provides mature tools for CLI (argparse), config (PyYAML), and testing (pytest)
- Offline tool -- Python dependency does not affect UniSet2 runtime or C++ build

### Negative Consequences

- Introduces Python as a build-time dependency for users who want ST conversion (Python is not currently required by UniSet2 core)
- blark may include Beckhoff-specific grammar extensions that produce confusing errors on pure IEC 61131-3 input
- Version coupling: blark updates could change AST dataclass structure

### Neutral Consequences

- The tool lives in `extensions/JScript/tools/st2js/` as a standalone Python package, isolated from the C++ build system
- Python is already present in the project for `pyUniSet2` bindings (`wrappers/python/`), so the dependency is not entirely foreign
- The tool can be installed via `pip install -e .` or run directly with `python -m st2js`

## Implementation Guidance

- Wrap blark parsing behind a thin adapter to isolate the rest of the tool from blark's internal AST structure changes
- Filter or reject Beckhoff-specific constructs at parse time with clear error messages pointing to pure IEC 61131-3 equivalents
- Use dependency injection for the parser interface to allow future replacement if blark proves inadequate
- Keep the tool as a standalone Python package with its own `pyproject.toml` and test suite

## Related Information

- [blark GitHub repository](https://github.com/klauer/blark) -- IEC 61131-3 parsing tools
- [blark on PyPI](https://pypi.org/project/blark/) -- Package distribution
- [lark-parser](https://github.com/lark-parser/lark) -- Underlying parsing library
- [IEC 61131-3:2013](https://webstore.iec.ch/en/publication/4552) -- Standard specification
- `extensions/JScript/js/uniset2-iec61131.js` -- Target JS FB library
- `extensions/JScript/IEC61131.md` -- FB documentation
- `wrappers/python/` -- Existing Python presence in the project
