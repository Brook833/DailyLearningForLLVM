# 最基本的程序
```c++
; main.ll
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

define i32 @main() {
    ret i32 0
}
```

上面的程序可以看作最简单的C语言代码:
```c++
int main() {
    return 0;
}
```

```c++
clang main.ll -o main
./main
```
使用clang可以直接将main.ll编译成可执行文件main。
运行这个程序后，程序自动退出，并返回0.

# 基本概念
## 注释
```
;main.ll
```
这是一个注释。
在LLVM中，注释以；开头，并一直延申到行尾。
所以在LLVM IR中，并没有像C语言中的/*comment block*/这样的注释块，而全都类似于//commment line 这样的注释行。

## 目标数据分布和平台
```c++
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"
```
第二行和第三行的target datalayout和target triple，则是注明了目标汇编代码的数据分布和平台。LLVM是一个面向多平台的深度定制化编译器后端，而我们LLVM IR的目的，则是让LLVM后端根据IR代码生成相应平台的汇编代码。所以，我们需要在IR代码中指明我们需要生成哪一个平台的代码，也就是target triple字段。类似的，我们还需要定制数据的大小端、对齐形式等需求，所以我们也需要指明target datalayout字段。

```c++
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
```
> + e:小端 (E:大段)
> + m:o符号表中使用Mach-O格式的name mangling(就是把程序中的标识符经过处理得到可执行文件中的符号表中的符号)
> + i64:64将i64类型的变量采用64比特的ABI对齐
> + f80:128将long double类型的变量采用128比特的ABI对齐
> + n8:16:32:64目标CPU的原生整型包含8比特、16比特、32比特和64比特
> + S128:栈以128比特自然对齐

```c++
target triple = "x86_64-apple-macosx10.15.0"
```
> + x86_64目标架构为x86_64架构
> + apple供应商为Apple
> + maccosx10.15.0目标操作系统为macOS 10.15

在一般情况下，我们呢都是想生成当前平台的代码，也就是说不太会改动这两行。因此，我们可以之际额写一个简单的test.c程序，然后使用:
```c++
clang -S -emit-llvm test.c
```
生成LLVM IR代码test.ll，在test.ll中找到target datalaout和triple这两个字段，然后拷贝到我们的代码中即可。

## 主程序
主程序是可执行程序的入口点，所以任何可执行程序都需要main函数才能运行:
```c++
define i32 @main() {
    ret i32 0
}
```
就是这段代码的主程序。关于正式的函数、指令的定义会在后面章节提及。
这里只需要知道，在@main()之后的，就是这个函数的函数体，ret i32 0 就代表C语言中的return 0。因此，如果我们增加代码，就只需要在大括号内，ret i32 0前增加代码即可。
