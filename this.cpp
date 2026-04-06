#include <cstdlib>
#include <iostream>
#include <fstream>
#include <filesystem>

#include <this.hpp>
#include <tlex.hpp>
#include <tparse.hpp>
#include <thisvm.hpp>
#include <thisc.hpp>
#include <tinit.hpp>

void open_file(const std::string &filename, std::string &buffer)
{
    std::ifstream inFile(filename);
    if (!inFile)
    {
        std::cerr << "Could not open file: " << filename << std::endl;
        exit(1);
    }
    std::stringstream ss;
    ss << inFile.rdbuf();
    buffer = ss.str();
}

void compile(const std::string &sourceFile, const std::string &outputFile)
{
    std::string sourceCode;
    open_file(sourceFile, sourceCode);
    ThisLexer lexer(sourceCode);
    ThisParser parser(lexer);
    auto ast = parser.thisX_parseProgram();
    ThisCompiler compiler(std::move(ast));
    compiler.compile(outputFile);
}

void virtualize(const std::string &bytecodeFile)
{
    ThisVM vm;
    initstd(vm);
    if (!vm.thisX_load(bytecodeFile))
    {
        std::cerr << "Failed to load bytecode file: " << bytecodeFile << std::endl;
        exit(1);
    }
    vm.thisX_run();
}

int main(int argc, char *argv[])
{
    // assume that they want everything
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h")
        {
            std::cout << "Usage: " << argv[0] << " <source_file.this | source_file.th>" << std::endl;
            std::cout << "Options:\n"
                      << "  --help, -h       Show this help message\n"
                      << "  --compile, -c    Compile only (output to out.tbc or specified output)\n"
                      << "  --vm, -v        Run VM on specified bytecode file (default: out.tbc)\n";
            return 0;
        }
        else if (arg == "--vm" || arg == "-v")
        {
            // run only the vm on what should be a bytecode file
            if (i + 1 >= argc)
            {
                std::cerr << "Expected bytecode (.tbc) file after --vm\n";
                return 1;
            }
            std::string vmArg = (i + 1 < argc) ? argv[i + 1] : "out.tbc";
            virtualize(vmArg);
            return 0;
        }
        else if (arg == "--compile" || arg == "-c")
        {
            // compile only, output to [whatever but probably out].tbc
            if (i + 1 >= argc)
            {
                std::cerr << "Expected source (.this or .th) file after --compile\n";
                return 1;
            }
            else if (!(i + 2 >= argc))
            {
                // use this thing then
                std::string compileArg = argv[i + 1];
                std::string outputArg = argv[i + 2];
                compile(compileArg, outputArg);
                std::cout << "Compiled " << compileArg << " to " << outputArg << '\n';
                return 0;
            }
            else
            {
                std::string compileArg = argv[i + 1];
                compile(compileArg, "out.tbc");
            }
        }
    }
    if (argc < 2) {
        std::cerr << "No input file provided (should be a .th or .this file). Use --help for usage information.\n";
        return 1;
    }

    std::string sourceCode;

    open_file(argv[1], sourceCode);

    compile(argv[1], "out.tbc");

    virtualize("out.tbc");

    // get rid of out.tbc
    std::filesystem::remove("out.tbc");

    return 0;
}