nsl
===

Net Stalker Library - C++ library for low latency environment transfer

# Library description

The library is capable of synchronization of objects containing continuously changing values through network. 
It is targeted to environments with many changes that happen almost all the time, f.e. computer games.
The latency of the transfer is minimal - it is close to one way delay.

The library consists of 2 modules - client and server. The objects containing the values are created on server 
and then synchronized to connected clients. On clients, the values are retrieved already interpolated.

Tutorial and description of all library functions is under construction.

# Compilation

There are 3 possible things to compile.
The library itself, the examples and the tests.

## NSL

In \NetStalkerLibrary there is a Visual Studio project and CMake file. Compiled binaries will appear in \build.

## Examples

You need first to compile NSL. Then compile the examples the same way. If launched through Visual Studio, the examples 
will be already linked to the NSL compilation, if you haven't changed any paths.

## Tests

You have to compile Google Test framework first. Follow the instructions on 
[the project homepage](https://code.google.com/p/googletest/wiki/Primer#Setting_up_a_New_Test_Project). 
Then compile the NSLTest project. If you compiled the framework through Visual Studio and launched the test project 
in Visual Studio as well, it will be already linked.
