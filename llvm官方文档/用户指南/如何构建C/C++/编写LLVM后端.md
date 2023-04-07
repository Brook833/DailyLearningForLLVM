## 受众
本文档的受众为需要编写LLVM后端以生成特定硬件或软件目标代码的人。

## 介绍
本文档描述了编写编译器后端的技术，将LLVM中间表示(IR)转换为指定机器或其他语言的代码。针对特定机器的代码可以采用汇编代码或二进制代码的形式(可用于JIT编译器)。

LLVM的后端具有独立于目标的代码生成器，可为多种类型的目标CPU创建输出，包括X86、PowerPC、ARM和SPARC。后端还可用于生成针对Cell处理器的SPU或GPU的代码，以支持计算内核的执行。

本文档重点介绍下载的LLVM版本中llvm/lib/Target子目录中找到的现有示例。特别是，本文档侧重于为SPARC目标创建静态编译器的示例(产生文本汇编代码)，因为SPARC具有相对标准的特征，如RISC指令集和简单的调用约定。

## 阅读前
在阅读本文档前,必须阅读以下基本文件:
> + LLVM语言参考手册————LLVM汇编语言的参考手册。
> + LLVM目标独立代码生成器————指导用于将LLVM内部表示转换为指定目标的机器代码的组件(类和代码生成算法)。特别注意代码生成阶段的描述:指令选择、调度和形成、基于SSA的优化、寄存器分配、入口/出口代码插入、后期机器代码优化和代码发射。
> + TableGen概述————描述管理特定于领域信息以支持LLVM代码生成的TableGen(tblgen)应用程序的文档。TableGen处理来自目标文件描述(.td后缀)的输入并生成可用于代码生成的C++代码。
> + LLVM Pass————汇编printer是FunctionPass，以及几个SelectionDAG处理步骤
> + 要按照文档中的SPARC示例，需要参考SPARC Architecture Manual, Version 8的副本。有关ARM指令集的详细信息，请参阅ARM Architecture Reference Manual。关于GNU Assembler格式（GAS）的更多信息，请参阅Using As，特别是对于汇编printer。 "Using As"包含目标机器相关特性的列表。

## 基本步骤
要为LLVM编写一个编译器后端，将LLVM IR转换为指定目标(机器或其他语言)的代码，请按照以下步骤进行:
> + 1.创建一个TargetMachine类的子类，描述目标机器的特性。
> + 2.描述目标的寄存器集。使用TalbeGen从特定于目标的RegisterInfo和寄存器类的代码。您还应编写TargetRegisterInfo的子类的额外代码，该类表示用于寄存器分配的类寄存器文件数据，并描述寄存器之间的交互。
> + 3.描述目标的指令集。使用TalbeGen从特定于目标的TargetInstrFormats.td和TargetInstrInfo类的子类的额外代码，以表示目标机器支持的机器指令。
> + 4.描述从指令的DAG表示到本机目标指令的选择和转换的过程。使用TableGen生成代码来匹配模式并基于目标特定版本的TargetInstrInfo.td中的其他信息选择指令。