This example demonstrates how PH8 can be compiled and linked directly into your application as 
a single source file, `amalgam.c`. 

Amalgamation combines the PH8 engine and its dependencies into one translation unit, 
simplifying integration and deployment while enabling the compiler to perform more 
aggressive optimizations, including cross-module inlining, dead code elimination, constant propagation, etc.

This is the preferred way of using PH8 in your application