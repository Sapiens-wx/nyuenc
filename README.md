# nyuenc

## 项目介绍

`nyuenc` 是用 C 编写的游程编码（Run-Length Encoding，RLE）工具，支持通过线程池并行读取和编码文件。

程序按命令行顺序将输入文件拼接为一段字节数据，将连续相同的字节编码为“字节值 + 重复次数”，并将结果写入标准输出。重复次数占一个字节，因此每组编码占两个字节。例如，输入 `aaabb` 的编码为十六进制字节 `61 03 62 02`。多个文件之间不添加分隔符，文件边界上相同的字节也参与游程合并。

## 编译

需要 Linux/POSIX 环境、GCC、GNU Make，以及 POSIX 线程和信号量支持。Windows 用户可在 WSL 或 Linux 容器内执行以下命令。以下命令从本仓库的 `labs` 目录开始：

```bash
cd nyuenc
make
```

生成当前目录下的可执行文件 `nyuenc`。makefile 使用 C99，通过 `-pthread` 启用线程支持，并使用 `-O2` 优化。

清理编译产物：

```bash
make clean
```

## 使用

```text
./nyuenc [-j threads] file [file ...]
```

- `file`：输入文件路径，可以提供多个文件，按给定顺序处理。
- `-j threads`：工作线程数量，默认为 `1`；应提供正整数。
- 输出是二进制数据，通常使用 `>` 保存为文件。

使用默认的一个工作线程编码文件：

```bash
./nyuenc input.txt > output.rle
```

使用四个工作线程编码多个文件：

```bash
./nyuenc -j 4 first.txt second.txt > output.rle
```

下面的示例展示输出格式，命令在 Bash 等外部 Shell 中执行：

```bash
printf 'aaabb' > input.txt
./nyuenc input.txt > output.rle
od -An -tx1 output.rle
# 预期输出：61 03 62 02
```

当前程序只实现编码，不提供解码命令，也不从标准输入读取数据。未提供文件或输入文件全部为空时，不输出编码数据。

使用时应提供存在且可读的文件，文件数量不超过 128。程序会将全部输入读入内存；并行编码时，线程数应不超过输入的总字节数。当前实现的计数使用一个字节，未处理超过 255 次的连续重复，部分跨分块合并情形也存在限制，因此应先验证目标数据的编码结果。
