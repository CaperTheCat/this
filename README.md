# [This]

"If you don't know how to code in [This], what are you doing??"

A kinda-fast, dynamically-typed language written in C++ that is nowhere close to complete. It's open for contributions and whatnot, but don't be surprised if you see some absolutely horrible code here.
Lovingly called "Python with curly braces," the language is stylized as [This] to avoid confusion with just the generic word. Why is it called [This]? Bcuz it's funny \:P

## How Does It Work?
Pretty simple. First, you create a `*.this` or `*.th` file. Then, you can run the `this` executable which runs through your file through a compiler (thisc) and a virtual machine (thisvm). The compiler translates the source file to some bytecode (.tbc) that can be run by the virtual machine. There's a couple of other arguments you can find out by using the --help argument.

## Building
> [This] is not distributed in binary. It is dealt in source and must be built.

[This] is built via CMake. Download a valid C/C++ compiler like `gcc` (just follow a tutorial on YouTube or something) and download CMake itself, and then go ahead and run the CMakeLists.txt file and build it.

## Hello, World!
Wanna run "Hello World" in This? Well, this code snippet is all you need:

```
import io

io.print("Hello World!")
```

Have fun with it and make cool stuff. All of it is licensed under MIT, so play around however you like.
