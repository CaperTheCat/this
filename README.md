# [This]

"If you don't know how to code in [This], what are you doing??"

A kinda-fast, dynamically-typed language written in C++ that is nowhere close to complete. It's open for contributions and whatnot, but don't be surprised if you see some absolutely horrible code here.
Lovingly called "Python with curly braces," the language is stylized as [This] to avoid confusion with just the generic word. Why is it called [This]? Bcuz it's funny :P

## How Does It Work?
Pretty simple. First, you create a `*.this` file. Then, you run the compiler (`thisc`) to get an out.tbc file. That out.tbc file is a ThisByteCode file filled with some low-level bytecode instructions.
You then run the virtual machine (`thisvm`) to read that out.tbc file and run it.

## Hello, World!
Wanna run "Hello World" in This? Well, this code snippet is all you need:

```
import io

io.print("Hello World!")
```
Though, as of [This] ALPHA 0.1.0, that "import io" isn't very necessary... for now. Still working on that.

Have fun with it and make cool stuff. All of it is licensed under MIT, so play around however you like.