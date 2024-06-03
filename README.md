# 2023 级信息类数据结构与程序设计大作业 Debug 辅助程序

## 1. 项目结构

```text
.
├── data
│   ├── codes.txt
│   ├── debuginfo.txt
│   ├── editdistDP.c
│   ├── keepwords.txt
│   └── log
│       └── .gitkeep
├── src
│   └── main.c
└── README.md
```

- `data/`：大作业需要读取的数据、程序的运行路径、日志文件等
- `src/`：debug 程序的源代码

## 2. 使用方法

### 2.1 仓库下载

下载代码仓库到本地，两种方法任选其一：

1. 使用 git clone：

```bash
# https 链接
git clone https://github.com/OnlyAR/2024-ds-project.git

# ssh 链接
git clone git@github.com:OnlyAR/2024-ds-project.git
```

2. 在 [releases](https://github.com/OnlyAR/2024-ds-project/releases) 下载仓库源码压缩包

### 2.2 数据准备

准备好 `data/` 目录下的 `codes.txt` 文件，该文件包含了需要检查的代码片段。默认使用的是课程网站下载的代码片段。也可以进行替换。

### 2.3 编译

需要将 `main.c` 编译到 `data/` 目录下，生成可执行文件。首先进入项目根目录 `2024-ds-project/`，然后执行：

```bash
gcc src/main.c -o data/main -std=c99
```

> 如果你的电脑找不到 `gcc` 命令，可以将 `gcc` 所在目录添加进环境变量 Path 中：[教程](https://blog.csdn.net/weixin_45684731/article/details/132915568)

### 2.4 执行

```bash
# Windows 系统
./data/main.exe

# 其他操作系统
./data/main
```

### 2.5 查看运行结果

运行结束后，可以查看 `data/log/` 中的日志文件，查看运行结果。

日志中包含的信息有：
- `codes.txt` 的文件长度
- 每个程序的：
  - 程序 ID
  - 函数个数
  - 每个函数的：
    - 名字
    - 调用顺序
    - 函数信息流
  - 程序信息流
- 参与比较的程序两两之间相似度（建议**输出错误**的同学检查一下自己参与比较的程序对是否多了或者少了）

> 由于题目没有对 `codes.txt` 的某些奇怪写法做出规范，所以生成的信息流和计算出的相似度可能会有微小差异，但是不影响最终结果的判断。