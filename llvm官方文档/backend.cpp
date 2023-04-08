int main() {
  BinopPrecedence['<'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['*'] = 40; // highest.
  fprintf(stderr, "ready> ");
  getNextToken();
  InitializeModuleAndPassManager();
  MainLoop();

  // 构建llvm backend
  // LLVM编译器的目标平台相关组件，包括目标平台信息、目标平台生成器、目标平台机器码、汇编解析器和汇编打印器等
  InitializeAllTargetInfos();
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllAsmPrinters();

  // 获取当前系统的默认目标平台三元组（triple）
  auto TargetTriple = sys::getDefaultTargetTriple();

  /* 这里的TheModule应该修改为遍历全局的vector<module *> 
  for (auto modue : modules) {
    modules->setTargetTriple(TargetTriple);
    ···
    后续代码应该均在此循环中
  } */

  // 为LLVM IR模块（TheModule）设置目标平台三元组（TargetTriple）。
  TheModule->setTargetTriple(TargetTriple);

  // 字符串变量，用于存储错误信息。
  std::string Error;
  // 从LLVM目标平台注册表（target registry）中查找目标平台生成器对象，并将其存储在Target变量中。
  auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);

  // 检查在查找目标平台生成器时是否发生错误，如果发生错误，则将错误信息输出到标准错误流并返回错误码1
  if (!Target) {
    errs() << Error;
    return 1;
  }

  // 设置目标平台生成器的CPU类型
  // 这里将CPU类型设置为"generic"，表示使用通用的CPU类型，适用于大部分平台。
  auto CPU = "generic";
  // 设置目标平台生成器的CPU特性（CPU feature）
  // 这里将CPU特性设置为空字符串，表示不使用任何特殊的CPU特性。
  auto Features = "";

  // 存储目标平台生成器的配置选项
  /* 举例:
  opt.OptLevel = 2; // 配置优化级别为2
  opt.EmitDebugInfo = true; // 生成调试信息
  opt.CodeModel = CodeModel::Small;
  opt.DataLayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128";
  */
  TargetOptions opt;
  // 存储目标平台生成器的重定位模型
  // 初始值为空,在后续的代码中，可以通过将RM作为参数传递给目标平台生成器，来指定重定位模型。
  auto RM = std::optional<Reloc::Model>();
  // 创建一个目标平台生成器（Target Machine）对象，以便后续使用该对象生成目标代码。
  auto TheTargetMachine =
      Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

  // 设置当前的模块对象（TheModule）的数据布局（Data Layout）属性
  TheModule->setDataLayout(TheTargetMachine->createDataLayout());

  // 存储生成的目标代码的输出文件名
  auto Filename = "output.o";
  // 用于存储可能出现的错误信息
  std::error_code EC;
  // 用于将生成的目标代码输出到指定的文件中
  // 考虑是否输出到buffer中。
  raw_fd_ostream dest(Filename, EC, sys::fs::OF_None);

  // 检查输出文件是否成功打开，如果打开失败则输出错误信息并退出程序。
  if (EC) {
    errs() << "Could not open file: " << EC.message();
    return 1;
  }

  // pass管理器
  legacy::PassManager pass;
  // FileType的llvm::CodeGeneratorFileType类型的变量，用于指定生成目标代码的类型
  auto FileType = CGFT_ObjectFile;

  // 向当前的目标平台生成器添加一些优化器（passes），并将生成的目标代码输出到指定的文件中
  if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
    errs() << "TheTargetMachine can't emit a file of this type";
    return 1;
  }

  // pass.run(*TheModule);
  pass.run(*TheModule);
  // 将目标代码缓冲区中的数据刷入到输出文件中
  dest.flush();

  outs() << "Wrote " << Filename << "\n";

  return 0;
}

// 注意，此时我们只是生成了目标文件，并没有生成可执行文件。

/*
链接:
1. 使用llvm::Linker::linkModules函数将多个目标文件链接成一个整体。该函数接受两个参数：目标模块（Destination）和源模块（Source）。在这里，目标模块表示最终生成的可执行文件或动态库，源模块则表示需要被链接的目标文件集合。
bool Linker::linkModules(Module &Dest, std::unique_ptr<Module> Src, unsigned Flags);

2. 设置链接标志（Link Flags）。链接标志是一个位掩码（bitmask），用于指定链接选项。可以使用llvm::Linker::Flags枚举类型来设置链接标志。
enum Flags : unsigned {
  None             = 0,
  OverrideFromSrc  = 1,  // Override symbol tables with those from Src module
  LinkOnlyNeeded   = 2,  // Link only needed symbols
  InternalizeLinkedSymbols = 4, // Mark linked symbols as internal
  ...
};

3. 如果需要，可以使用llvm::Triple类来指定目标平台信息。
Triple(TargetTriple);
*/

请使用LLVM的C++API举例，实现由目标文件到可执行程序的示例

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

int main() {
  LLVMContext Context;
  std::unique_ptr<Module> MainModule = std::make_unique<Module>("Main", Context);

  // 读取目标文件
  std::vector<std::string> ObjectFiles = {"foo.o", "bar.o"};

  // 将目标文件链接到主模块中
  for (auto &ObjFile : ObjectFiles) {
    std::unique_ptr<Module> ObjModule = parseIRFile(ObjFile, Error, Context);
    if (!ObjModule) {
      std::cerr << "Failed to load object file " << ObjFile << "\n";
      return 1;
    }
    if (Linker::linkModules(*MainModule, std::move(ObjModule))) {
      std::cerr << "Failed to link object file " << ObjFile << "\n";
      return 1;
    }
  }

  // 进行优化
  legacy::PassManager Passes;
  Passes.add(createPromoteMemoryToRegisterPass());
  Passes.run(*MainModule);

  // 生成可执行文件
  std::string OutputFilename = "program";
  std::error_code EC;
  raw_fd_ostream OutputFile(OutputFilename, EC, sys::fs::OF_None);
  if (EC) {
    std::cerr << "Failed to open output file " << OutputFilename << "\n";
    return 1;
  }
  if (verifyModule(*MainModule, &errs())) {
    std::cerr << "Module verification failed\n";
    return 1;
  }
  if (WriteBitcodeToFile(*MainModule, OutputFile)) {
    std::cerr << "Failed to write output file " << OutputFilename << "\n";
    return 1;
  }

  std::cout << "Output file generated: " << OutputFilename << "\n";
  return 0;
}

如果不读取ir文件，而是直接使用记录的Module，要怎么做，请举例

如果已经将IR代码转换为LLVM的IR表示（Module对象），可以直接将这些Module对象链接到主Module中，方法与上面类似，只需要使用Module::getFunctionList()方法获取Module中所有函数的引用，然后将它们添加到主Module中即可。

下面是一个简单的示例，演示如何将多个Module链接到主Module中：
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

int main() {
  LLVMContext Context;
  std::unique_ptr<Module> MainModule = std::make_unique<Module>("Main", Context);

  // 读取Module列表
  std::vector<std::unique_ptr<Module>> Modules;
  // 假设有两个Module：foo和bar
  std::unique_ptr<Module> FooModule = ...
  std::unique_ptr<Module> BarModule = ...
  Modules.push_back(std::move(FooModule));
  Modules.push_back(std::move(BarModule));

  // 将Module链接到主模块中
  for (auto &Module : Modules) {
    for (auto &Function : Module->getFunctionList()) {
      if (Function.isDeclaration()) {
        Function.setLinkage(GlobalValue::ExternalLinkage);
      }
      MainModule->getFunctionList().push_back(&Function);
    }
  }

  // 进行优化
  legacy::PassManager Passes;
  Passes.add(createPromoteMemoryToRegisterPass());
  Passes.run(*MainModule);

  // 生成可执行文件
  std::string OutputFilename = "program";
  std::error_code EC;
  raw_fd_ostream OutputFile(OutputFilename, EC, sys::fs::OF_None);
  if (EC) {
    std::cerr << "Failed to open output file " << OutputFilename << "\n";
    return 1;
  }
  if (verifyModule(*MainModule, &errs())) {
    std::cerr << "Module verification failed\n";
    return 1;
  }
  if (WriteBitcodeToFile(*MainModule, OutputFile)) {
    std::cerr << "Failed to write output file " << OutputFilename << "\n";
    return 1;
  }

  std::cout << "Output file generated: " << OutputFilename << "\n";
  return 0;
}
在上面的示例中，我们首先读取了多个Module（假设有两个Module：foo和bar），然后将它们链接到主模块中。在链接过程中，我们将每个函数的链接类型设置为ExternalLinkage，以避免出现重复定义的问题。最后，我们对主模块进行了一些简单的优化，然后将其输出到一个可执行文件中。

需要注意的是，如果多个Module之间存在函数名或变量名相同的情况，可能会导致链接错误，需要进行一些处理来避免这种情况的发生。此外，在将多个Module链接到主模块中时，还需要注意函数调用的符号解析和重定位