# dmstack

dmstack 是一个用于采集和解析进程调用栈的命令行工具。在 Linux 环境下，它可以采集目标进程的线程栈，并结合本地二进制文件或调试符号进行符号解析；同时也支持从已有的栈文本文件中读取内容并重新格式化输出。

## 功能特性

- 采集目标进程所有线程的调用栈。
- 基于 `libunwind`、`binutils/bfd` 和 `elfutils/libelf` 解析并符号化栈帧。
- 支持对原始栈文件进行重新格式化、过滤、聚合和紧凑输出。
- 支持优先输出指定线程，并按最小栈深度过滤结果。
- 支持指定二进制文件路径和独立调试符号路径。

## 依赖

### Linux

- CMake 3.8 或更高版本
- 支持 C++14 的编译器
- GNU Make
- 当前 Linux 版本支持 x86 和 ARM 两种平台

Linux 构建直接使用系统安装的动态库（`.so`）。需要安装以下开发包：

- binutils
- elfutils 
- libunwind 



Debian / Ubuntu：

```bash
sudo apt-get update
sudo apt-get install -y cmake g++ make binutils-dev libelf-dev libunwind-dev
```



### Windows

- CMake 3.8 或更高版本
- Visual Studio C++ 编译工具链，或其他可被 CMake 识别的 C++14 编译器
- Ninja

 Windows 目标主要用于通过 `--input` 读取并格式化已有栈文本。

## 构建

### Linux

当前 Linux 支持 x86 和 ARM 两种平台。CMake 会根据当前机器架构选择对应的 libunwind 架构库名称，例如 `unwind-x86_64` 或 `unwind-aarch64`。

Debug 构建：

```bash
cmake -S . -B build_debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build_debug
```

Release 构建：

```bash
cmake -S . -B build_release -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build_release
```

生成的可执行文件位于：

```bash
build_release/dmstack/dmstack
```

### Windows

Windows 可通过 CMake 和 Ninja 直接构建：

```powershell
cmake -S . -B out/build/x64-release -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build out/build/x64-release
```

生成的可执行文件位于：

```powershell
out/build/x64-release/dmstack/dmstack.exe
```

## 使用

采集并解析某个进程所有线程的调用栈：

```bash
dmstack <pid>
```

仅采集原始地址，不做符号解析：

```bash
dmstack --no-parse <pid>
```

从文件读取栈内容并重新格式化：

```bash
dmstack --input stack.txt
```

聚合相同的调用栈：

```bash
dmstack --aggregate <pid>
```

使用紧凑输出、隐藏线程 ID，并只输出深度大于 10 的调用栈：

```bash
dmstack --format-compact --hide-thread 1 --min-depth 10 <pid>
```

### 常用参数

| 参数 | 说明 |
| --- | --- |
| `-v`, `--version` | 显示版本信息。 |
| `--help` | 显示帮助信息。 |
| `-f`, `--format-compact` | 使用紧凑格式输出，每个线程一行，栈帧以逗号分隔。 |
| `-l`, `--log-level <LEVEL>` | 设置日志级别，支持 `DEBUG`、`INFO`、`WARN`、`ERROR`。 |
| `--input <FILE>` | 从文件读取原始栈内容。 |
| `-a`, `--aggregate` | 聚合相同调用栈，并输出出现次数。 |
| `-r`, `--reverse` | 反转聚合结果排序，仅在启用 `--aggregate` 时生效。 |
| `-b`, `--binary-path <PATH>` | 指定二进制文件或可执行文件搜索路径。 |
| `-s`, `--symbol-path <PATH>` | 指定独立调试符号文件搜索路径。 |
| `--search-system-symbols` | 搜索系统调试符号路径，例如 `/usr/lib/.build-id` 和 `/usr/lib/debug`。 |
| `-t`, `--thread-only` | 仅处理单个选中或当前线程。 |
| `-i`, `--min-depth <N>` | 仅输出栈深度大于 `N` 的线程。 |
| `-c`, `--priority-threads <PATTERNS>` | 优先输出匹配的线程，多个模式用空格分隔。 |
| `-n`, `--no-parse` | 仅输出地址，不解析符号、路径或文件行号。 |
| `-h`, `--hide-thread <0|1|2>` | 隐藏线程头字段。`0` 表示不隐藏，`1` 表示隐藏线程 ID，`2` 表示全部隐藏。 |

## 项目结构

```text
.
|-- CMakeLists.txt
|-- dmstack/
|   |-- dmstack.cpp
|   |-- config.*
|   |-- bfd/
|   |-- log/
|   |-- unwind/
|   `-- utils/
```

## 注意事项

- Linux 下采集目标进程调用栈时，可能需要具备访问目标进程的权限。

  使用命令授予权限：

  ```
  sudo setcap cap_sys_ptrace+ep dmstack
  ```

- 为获得更完整的符号解析结果，建议提供目标程序的二进制路径和匹配的调试符号。

- Linux 构建使用系统安装的动态库，需要确保 `libbfd.so`、`libiberty`、`libelf.so`、`libunwind*.so`  可被编译器和动态链接器找到。

## 许可证

本项目使用 GPL-3.0 许可证，详见 [LICENSE](LICENSE)。
