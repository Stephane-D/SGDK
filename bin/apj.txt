APJ packer v1.00 by Stephane Dallongeville (Copyright 2020)

This tool is basically a (dirty) Java port from the original C# version of CAP compressor (ApLib based) from Svendahl (https://github.com/svendahl/cap). Many thanks to him for letting me to use it !
It has been slighly modified for the purpose of integration with SGDK.
There is no licence attached to original version but the (current) Java version obey to SGDK (https://github.com/Stephane-D/SGDK) licenses.

Usage
-----
Pack:     java -jar apj.jar p <input_file> <output_file>
Unpack:   java -jar apj.jar u <input_file> <output_file>

Using an extra parameter after <output_file> acts as a 'silent mode' switch.
