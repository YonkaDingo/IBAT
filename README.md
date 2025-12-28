This repository represents a selection of header files to represent the structure of the IBAT engine.

The most important files are:

***
Shell.hpp - The custom shell for interacting with IBAT through user-registered commands.

Executor.hpp - The top level class which holds a strategy and handles execution of methods.

IStrategy.hpp - The abstract base class of a strategy.

PerSymbol.hpp - A template struct to allow management of abstract types independently across symbols.

Integrators.hpp - A collection of declarative type-safe macros for automatic registration of class members created inside of an IStrategy derivation.
***

This repository is incomplete and intended only as a showcase, the full engine code is proprietary at this time.
