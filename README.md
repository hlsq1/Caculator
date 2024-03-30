# 计算器

## 计算机实现的主要功能

命令格式：
calculator -S [-A | -D] [-a | -I] [-o output] source
calculator -R [-A | -D] source

选项-S -a 借助 flex+bison 产生抽象语法树 AST

选项-S -I 借助 flex+bison 产生线性 IR

选项-o output 可把输出的抽象语法树或线性 IR 保存到文件中

选项-R 可直接运行得出计算器结果

选项-A 指定时可通过 antlr4 进行词法与语法分析
选项-D 指定时可通过递归下降分析法实现语法分析，词法分析手动实现
不指定-A 或-D 选项时默认采用 Flex/Bison 实现词法与语法分析

## 源代码构成

frontend 目录下的源代码实现编译器的前端，有三种：flex/bison、antlr4、递归下降分析法

backend 目录下的源代码实现编译器的后端，有两种：解释执行或转换成 ARM32 汇编

DragonIR 目录下的源代码描述规定的线性 IR

common 目录下的源代码描述符号表或共通的代码

utils 目录下的源代码实现个别函数的操作系统差异

tests 目录下放置了一些简单的测试用例代码

thirdparty 目录下放置了一些第三方的工具，如 antlr4.12.0

## 程序构建

请使用VSCode + WSL/Container/SSH + Ubuntu 22.04进行编译与程序构建。

在使用前请使用命令安装clang-format与clang-tidy，这样该项目可使用根文件夹下的.clang-format和.clang-tidy进行代码分析。
大家也可自行修改.clang-format和.clang-tidy的内容。

安装命令格式如下：

```shell
sudo apt install -y clang-format clang-tidy
```

方式一使用VScode的cmake插件直接先选择clang编译器后进行程序的构建。

方式二可使用如下的命令进行构建。

```shell
# cmake根据CMakeLists.txt进行配置与检查，这里使用clang编译器并且是Debug模式
cmake -B cmake-build-debug -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++
# cmake，其中--parallel说明是并行编译
cmake --build cmake-build-debug --parallel
```

## 计算器使用方法

在Ubuntu 22.04平台上运行。支持的命令如下所示：

```shell

./cmake-build-debug/calculator -S -a test.txt

./cmake-build-debug/calculator -S -a -A test.txt

./cmake-build-debug/calculator -S -a -D test.txt

./cmake-build-debug/calculator -S -a -o test.pdf test.txt

./cmake-build-debug/calculator -S -a -A -o test.pdf test.txt

./cmake-build-debug/calculator -S -a -D -o test.pdf test.txt

./cmake-build-debug/calculator -S -I test.txt

./cmake-build-debug/calculator -S -I -A test.txt

./cmake-build-debug/calculator -S -I -D test.txt

./cmake-build-debug/calculator -S -I -o ir.txt test.txt

./cmake-build-debug/calculator -S -I -A -o ir.txt test.txt

./cmake-build-debug/calculator -S -I -D -o ir.txt test.txt

./cmake-build-debug/calculator -R test.txt

./cmake-build-debug/calculator -R -A test.txt

./cmake-build-debug/calculator -R -D test.txt
```

## 工具

计算器实验的所有工具在实验一中已经安装，这里不需要再次安装。这里主要介绍一些功能。

### Flex 与 Bison

#### Windows

主要给 Visual Studio 用

win_flex -o calculator_lex.cpp --wincompat calculator.l

win_bison -o calculator.cpp calculator.y

#### MinGW、Linux or Mac

flex -o calculator_lex.cpp --header-file=calculator_lex.h calculator.l

bison -o calculator_yacc.cpp --header=calculator_yacc.h -d calculator.y

请注意 bison 的--header 在某些平台上可能是--defines，要根据情况调整指定。

### Antlr 4.12.0

要确认java15 以上版本的 JDK，否则编译不会通过。默认已经安装了JDK 17的版本。

由于cmake的bug可能会导致适配不到15以上的版本，请删除旧版的JDK。

编写 g4 文件然后通过 antlr 生成 C++代码，用 Visitor 模式。

```shell
java -jar tools/antlr-4.12.0-complete.jar -Dlanguage=Cpp -no-listener -visitor -o frontend/antlr4 frontend/antlr4/Calculator.g4
```

C++使用 antlr 时需要使用 antlr 的头文件和库，在 msys2 下可通过如下命令安装 antlr 4.12.0 版即可。

```shell
pacman -U https://mirrors.ustc.edu.cn/msys2/mingw/mingw64/mingw-w64-x86_64-antlr4-runtime-cpp-4.12.0-1-any.pkg.tar.zst
```

### Graphviz

借助该工具提供的C语言API实现抽象语法树的绘制。

### doxygen

借助该工具分析代码中的注释，产生详细分析的文档。这要求注释要满足一定的格式。具体可参考实验文档。

### texlive

把doxygen生成的文档转换成pdf格式。

## 根据注释生成文档

请按照计算器实验的文档要求编写注释，可通过doxygen工具生成网页版的文档，借助latex可生成pdf格式的文档。

请在本实验以及后续的实验按照格式进行注释。

### 生成网页版文档

```shell
doxygen Doxygen.config
```

需要时可自行修改配置文件 Doxygen.config。

### 生成 pdf格式的文档

```shell
cd latex
make
```

## 计算器运行

tests 目录下存放了一些简单的测试用例。其中 test1.c 是 test1.txt 的 C 语言版本实现，用于运行的对比。

由于 qemu 的用户模式在 Window 系统下不支持，因此要么在真实的开发板上运行，或者用 Linux 系统下的 qemu 来运行。

后面介绍如用 Linux 系统下的 qemu 来运行。

### 计算器调试运行

由于自定的gdb或者lldb调试器对C++的STL模版库提供的类如string、map等的显示不够友好，因此请大家确保安装
vadimcn.vscode-lldb插件，也可以更新最新的代码后vscode会提示安装推荐插件后自动安装。
如安装不上请手动下载后安装，网址如下：
<https://github.com/vadimcn/codelldb/releases/>

调试运行配置可参考.vscode/launch.json中的配置。

### 生成 ARM32 的汇编

```shell
# 通过计算器翻译 test1.txt 成 ARM32 汇编
./cmake-build-debug/calculator -S -o tests/test1.s tests/test1.txt
# 把 test1-1.c 通过 arm 版的交叉编译器 gcc 翻译成汇编
arm-linux-gnueabihf-gcc -S -include tests/std.h -o tests/test1-1.s tests/test1-1.c
```

如果不指定--include tests/std.h，编译会提示函数 putint 没有声明的警告信息。

test1-1.c 是脚本型语言 test1.txt 的 C 语言表达。

### 生成可执行程序

通过 gcc 的 arm 交叉编译器对生成的汇编进行编译，生成可执行程序。

```shell
# 通过 ARM gcc 编译器把汇编程序翻译成可执行程序，目标平台 ARM32
arm-linux-gnueabihf-gcc -static -g -o tests/test1 tests/std.c tests/test1.s
# 通过 ARM gcc 编译器把汇编程序翻译成可执行程序，目标平台 ARM32
arm-linux-gnueabihf-gcc -static -g -o tests/test1-1 tests/std.c tests/test1-1.s
```

有以下几个点需要注意：

1. 这里必须用-static 进行静态编译，不依赖动态库，否则后续通过 qemu-arm-static 运行时会提示动态库找不到的错误
2. 生成的汇编中包含了 内置 putint 等函数的调用，用来进行数据的输出或输出等，因此在通过 arm-linux-gnueabihf-gcc 进行交叉编译时，需要和 std.c 一起进行编译链接才可以。
3. 可通过网址<https://godbolt.org/>输入 C 语言源代码后查看各种目标后端的汇编。下图是选择 ARM GCC 11.4.0 的源代码与汇编对应。
![godbolt 效果图](doc/figures/godbolt-test1-1-arm32-gcc.png)
4. 这些内置函数的具体实现放在了 tests/std.c 中，其原型在 tests/std.h 中，很简单，请自行查阅与理解。

### 运行可执行程序

借助用户模式的 qemu 来运行，arm 架构可使用 qemu-arm-static 命令。

```shell
qemu-arm-static tests/test1
qemu-arm-static tests/test1-1
```

这里可比较运行的结果，如果两者不一致，则计算器程序有问题。

## qemu 的用户模式

qemu 的用户模式下可直接运行交叉编译的用户态程序。这种模式只在 Linux 和 BSD 系统下支持，Windows 下不支持。

因此，为便于后端开发与调试，请用 Linux 系统进行程序的模拟运行与调试。

## qemu 用户程序调试

### 安装 gdb 调试器

该软件 gdb-multiarch 在前面工具安装时已经安装。如没有，则通过下面的命令进行安装。

```shell
sudo apt-get install -y gdb-multiarch
```

### 启动具有 gdbserver 功能的 qemu

假定通过交叉编译出的程序为 tests/test1，执行的命令如下：

```shell
# 启动 gdb server，监视的端口号为 1234
qemu-arm-static -g 1234 tests/test1
```

其中-g 指定远程调试的端口，这里指定端口号为 1234，这样 qemu 会开启 gdb 的远程调试服务。

### 启动 gdb 作为客户端远程调试

建议通过 vscode 的调试，选择 Qemu Debug 进行调试，可开启图形化调试界面。
可根据需要修改相关的配置，如 miDebuggerServerAddress、program 等选项。

也可以在命令行终端上启动 gdb 进行远程调试，需要指定远程机器的主机与端口。
注意这里的 gdb 要支持目标 CPU 的 gdb-multiarch，而不是本地的 gdb。

```shell
gdb-multiarch tests/test1
# 输入如下的命令，远程连接 qemu 的 gdb server
target remote localhost:1234
# 在 main 函数入口设置断点
b main
# 继续程序的运行
c
# 之后可使用 gdb 的其它命令进行单步运行与调试
```

在调试完毕后前面启动的 qemu-arm-static 程序会自动退出。因此，要想重新调试，请启动第一步的 qemu-arm-static 程序。

## 计算器强化

理解基于 Flex+Bison 的 LR 分析法、基于 ANTLR4 的 LL 分析法以及递归下降分析法等实现词法与语法分析功能。

目前三者都实现了无符号整数的识别以及加减运算运算、赋值运算等，函数定义与调用没有全部支持。

### 后端强化

目前支持把脚本转换成 ARM32 的汇编语言，然后借助交叉编译与 Qemu 运行。同时后端可提供了模拟解释执行的方式，但没有实现函数调用的功能支持。
请强化模拟解释执行，实现函数调用等功能。

### 递归下降分析法下文法强化

目前递归下降分析法分析的语法没有支持全面，请支持函数的定义、函数调用功能，使得三者都实现同样的功能。

### 强化运算符和整数的支持

工具请从 Flex/Bison 和 ANTLR4 中选择其中一种，优先选择 ANTLR4。若其中还有部分文法不支持，也需要支持。

请追加如下的功能支持：

1. 无符号整数支持 8 进制和 16 进制，八进制以 0 开头，16 进制以 0x 或 0X 开头，与 C 语言保持一致
2. 支持单目运算符求负运算-
3. 支持乘法*、除法/、求余%运算
4. ARM32 后端和模拟解析执行支持

### 支持浮点数与相关运算（可选）

目前只支持了整数的相关运算，请引入浮点数，可以进行整数与浮点数的混合运算。

具体要求有：

1. 词法要支持浮点数的检测
2. 抽象语法树要区分整数与实数运算
3. 要求整数的结果为整数、浮点数的运算结果是浮点数、混合运算的结果为浮点数
4. 根据源操作数的类型自行推导变量的类型
5. ARM32 后端和模拟解析执行支持
6. 除法/按 C 语言方式解释，两个整数求商（整数），非两个整数按浮点数求解得浮点数
7. 求余只对两个整数有效
