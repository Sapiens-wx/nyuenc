# nyuenc

## Overview

`nyuenc` is a run-length encoding (RLE) tool written in C. It uses a thread pool to read and encode files in parallel.

The program concatenates input files in command-line order, encodes consecutive identical bytes as a byte value followed by its repetition count, and writes the result to standard output. The count occupies one byte, so each encoded pair occupies two bytes. For example, `aaabb` is encoded as the hexadecimal bytes `61 03 62 02`. No separators are inserted between files, and matching bytes at file boundaries participate in run merging.

## Building

Requires a Linux/POSIX environment, GCC, GNU Make, and support for POSIX threads and semaphores. Windows users can run these commands in WSL or a Linux container. Starting from the parent `labs` directory:

```bash
cd nyuenc
make
```

This creates the `nyuenc` executable in the current directory. The makefile uses C99, enables thread support with `-pthread`, and applies `-O2` optimization.

To remove build artifacts:

```bash
make clean
```

## Usage

```text
./nyuenc [-j threads] file [file ...]
```

- `file`: An input file path. Multiple files are processed in the order provided.
- `-j threads`: The number of worker threads, defaulting to `1`. Supply a positive integer.
- Output is binary data; use `>` to save it to a file.

Encode a file using the default single worker thread:

```bash
./nyuenc input.txt > output.rle
```

Encode multiple files using four worker threads:

```bash
./nyuenc -j 4 first.txt second.txt > output.rle
```

The following example demonstrates the output format. Run these commands in an external shell such as Bash:

```bash
printf 'aaabb' > input.txt
./nyuenc input.txt > output.rle
od -An -tx1 output.rle
# Expected output: 61 03 62 02
```

The program implements encoding only; it provides no decoding command and does not read from standard input. If no files are supplied or all input files are empty, it produces no encoded output.

Provide existing, readable files, with at most 128 input files. The program loads all input into memory. When encoding in parallel, the thread count should not exceed the total number of input bytes. The current implementation uses a one-byte count and does not handle runs longer than 255 bytes. Some cases involving merging across chunk boundaries also have limitations, so verify the encoded results for your intended data.
