# QMC2-Decode

## Downstream updates
### Usage
>qmc-decode \<input\> \<output\> [\<ekey\>]

When `input` is a directory, it decrypts in batch mode, but only supports `qmcflac` and `qmc0` formats (due to `mflac0`
and `mgg1` need `ekey`)

when `output` is a directory, the output filename is automatically assigned from the extension of the input file.

`ekey` is a 704- or 364-byte Base64-encoded cipher text

### Supported file formats:

- qmcflac (to flac)
- qmc0 (to mp3)
- mflac0 with STag or QTag file trailing mark (to flac; EKey is required when decrypting STag)
- mgg1 with STag or QTag file trailing mark (to ogg; EKey is required when decrypting STag)

# Original README

<small><a href="#english">Click here for English readme</a></small>

用于解密 `mflac` / `mgg1` 文件的小工具。

## 构建

同仓库的 submodule 一同克隆，然后在终端构建：

```sh
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

注：

1. 你需要安装 CMake 以及一个支持的 C++ 编译器 (如：g++, MSVC)。
2. 编译好的二进制文件可以在 `build/QMC2-decoder` 目录下找到。
3. 你也可以使用 Visual Studio 2022 进行构建，文件位置在 `out` 目录下。

## 可执行文件下载

你可以在 [Release][latest_release] 找到
Linux x86_64、Win (32 位) 及 Win (64 位) 的二进制文件下载。

同时亦提供转译 WASM 的版本 (需要现代浏览器)：[QMC2-WASM][qmc2_wasm]

注：Windows 版本需要最新的 [VC++ x64 运行时][vs2022_runtime]。

## 使用方式

```sh
./QMC2-decoder "encrypted_file.mflac" "decrypted.flac"
```

注：如果解密失败，可以尝试将文件名中的非 ASCII 字符去掉后尝试。
Linux 因为使用 UTF-8 编码因而没有该问题。

## 支持的加密格式

- 文件末端为 `'QTag'` 字样的 `mgg1` / `mflac0` 文件；或许也支持其它的该结尾的文件。
- 文件末端为 `0x?? 0x01 0x00 0x00` 的 `mflac` 与 `mgg` 文件；
- 文件末端为 `0x?? 0x02 0x00 0x00` 的 `mgg` 文件；

## Node 包

安装：

```sh
npm i @jixun/qmc2-crypto
```

引入后使用即可，默认使用的是不依赖 `wasm` 的版本。

详细请参见[包的说明][npm_readme]以及[使用文档/用例][npm_usage]。

## 致谢

- [2021/08/26 MGG/MFLAC 研究进展][research] by @ix64 & @Akarinnnnn
- [unlock-music 项目][unlock-music]
- 使用 Visual Studio 2022 进行开发

---

# English

Decryptor for files with `mflac` / `mgg1` extension.

## Build

Clone with submodules, then:

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

Note:

1. CMake and a supported C++ compiler (i.e. g++, MSVC) is required.
2. Binary will be built at `build/QMC-decoder` folder.
3. You can also build with Visual Studio 2022, where the binary will be found at `out`.

## Pre-built binaries

You can find Linux (x86_64), Win (32-bit) and Win (64-bit) binaries
at [Release][latest_release] section.

WASM transpiled version is also available (modern browser required):
[QMC2-WASM][qmc2_wasm]

Note: Windows binary requires latest [VC++ x64 Runtime][vs2022_runtime].

## Usage

```sh
./QMC2-decoder "encrypted_file.mflac" "decrypted.flac"
```

If decryption fails in Windows, please try again with non-ASCII
characters in the file path. Linux uses UTF-8 encoding so is safe.

## Supported format

- Ending with characters `'QTag'`, with extension `mgg1` / `mflac0`;  
  might also work for other extensions with this ending.
- Ending with `0x?? 0x01 0x00 0x00` with extension `mflac` & `mgg`;
- Ending with `0x?? 0x02 0x00 0x00` with extension `mgg`;

## Node Package

Install:

```sh
npm i @jixun/qmc2-crypto
```

It uses the `legacy` build by default that does not require `WASM`.

See [package readme][npm_readme] and [usage document][npm_usage] for details.

## Credits

- [2021/08/26 MGG/MFLAC 研究进展][research] by @ix64 & @Akarinnnnn
- [unlock-music][unlock-music]
- Developed with Visual Studio 2022

[research]: https://gist.github.com/ix64/bcd72c151f21e1b050c9cc52d6ff27d5
[qmc2_wasm]: https://jixunmoe.github.io/qmc2/
[unlock-music]: https://github.com/unlock-music/unlock-music
[latest_release]: https://github.com/jixunmoe/qmc2/releases/latest
[vs2022_runtime]: https://aka.ms/vs/17/release/vc_redist.x64.exe
[npm_readme]: https://github.com/jixunmoe/qmc2/tree/main/QMC2-wasm/npm
[npm_usage]: https://jixunmoe.github.io/qmc2/source.html
