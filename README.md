# icarus-fv
Aiger Formal Verification Target and PLI for Icarus Verilog

## Dependencies
- Icarus Verilog

## Installation
1. run `make` from main directory
2. Move entire contents of `build` directory into the IVL directory.
   For a 64-bit Debian system the path is `/usr/lib/x86_64-linux-gnu/ivl/`

## Using the Aiger target
Build a Verilog design using the aiger target by running
`iverilog -o aiger_file.aig -t aig my_verilog_src.v'
where `aiger_file.aig` is the targetted AIGER output file
and `my_verilog_src.v` is the Verilog source to be compiled.

## Using the Aiger target PLI
This target supports four PLI's.
All take two arguments, a predicate (probably most useful in form of a `wire`) and
a string description.

- `$aig_bad(p,"My Bad State");`
- `$aig_constraint(p,"My Constraint");`
- `$aig_fairness(p,"My Fairness");`
- `$aig_justice(p,"My Justice");`

See `example` directory for usage examples.

## Limitations
This target currently has some notable limitations.

1. Vectors/Array Types are not supported. Vectors should be converted to
   single signals before compiling.
2. If Then Else constructs have a serious bug. Avoid by using ternary
   statements.
3. Few LPM functions are supported. Mathematical functions will be
   ignored.
