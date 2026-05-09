# mupdf_dll

对 [MuPDF](https://mupdf.com/) 进行二次封装的 C 语言 DLL 包装层。
将 MuPDF 内部以 `fz_context` / `fz_document` 为核心的 API，重新组织为更稳定、易于跨语言调用（C / C++ / C# / Rust / Python ctypes 等）的扁平接口，数据布局参考 PDFium 风格，便于上层快速集成。

## 功能特性

- 文档生命周期管理：`mupdf_ctx_create` / `mupdf_doc_open` / `mupdf_doc_close` / `mupdf_ctx_destroy`
- 页面信息：`mupdf_doc_page_count`
- 页面渲染：`mupdf_page_render`，支持缩放、旋转、Alpha 通道，输出连续 RGB(A) 字节缓冲区
- 目录（TOC）提取：`mupdf_doc_get_outline`，以 UTF-8 JSON 形式返回，含层级、页码、URI、标志位
- 错误信息查询：`mupdf_last_error`
- 全部以不透明句柄（opaque handle）暴露，调用方无需感知 MuPDF 内部类型

详细 API 见 [mupdf_api.h](mupdf_api.h)。

## 目录结构

```text
mupdf_dll/
├── CMakeLists.txt        # 构建脚本
├── mupdf_api.h           # 对外暴露的 C API 头文件
├── mupdf_api.c           # 包装层实现
├── main.c                # 测试程序（mupdf_test 可执行文件）
├── include/mupdf/        # MuPDF 原始头文件（fitz.h、pdf.h 等）
├── lib/                  # ⚠️ 需要手动放入 libmupdf.lib
└── test/                 # 构建产物输出目录（DLL、可执行文件、头文件）
```

## ⚠️ 构建前置条件

**本仓库不包含 MuPDF 编译产物，构建前必须自行准备 `libmupdf.lib` 并放入 `lib/` 目录。**

```text
lib/
└── libmupdf.lib    ← 必须存在
```

获取方式（任选其一）：

1. 从 [MuPDF 官方源码](https://mupdf.com/releases) 拉取后用 Visual Studio 打开 `platform/win32/mupdf.sln` 编译，得到 `libmupdf.lib`
2. 使用已有的 MuPDF 静态库 / 导入库，重命名或拷贝为 `libmupdf.lib` 后放入 `lib/` 目录

`CMakeLists.txt` 通过 `file(GLOB ...)` 自动扫描 `lib/*.lib`：未找到时 MSVC 编译会因符号缺失而链接失败，请务必先放入。

注意架构、运行库（/MD vs /MT）、Debug/Release 配置需要与本工程保持一致，否则会出现 LNK2019 / LNK2038 等链接错误。

## 构建步骤

使用 CMake（推荐 Visual Studio 2019 / 2022 + MSVC x64）：

```bash
cmake -S . -B build -A x64
cmake --build build --config Release
```

构建成功后，产物会被自动复制到 `test/` 目录：

- `test/mupdf.dll`、`test/mupdf_test.exe`
- `test/dll/mupdf.lib`
- `test/include/mupdf_api.h`

## 运行示例

```bash
cd test
./mupdf_test.exe sample.pdf 1 150 0
```

参数说明：

| 参数 | 含义 |
| --- | --- |
| `sample.pdf` | PDF / XPS / CBZ / EPUB 文档路径 |
| `1` | 页码（从 1 开始） |
| `150` | 缩放百分比（100 = 72 dpi） |
| `0` | 旋转角度（0 / 90 / 180 / 270） |

程序会输出文档目录（JSON）和指定页面的尺寸信息。

## 许可证

MuPDF 采用 AGPL / 商业双许可，本封装层在分发时需遵循 MuPDF 的对应许可条款。请根据实际使用场景自行确认合规性。
