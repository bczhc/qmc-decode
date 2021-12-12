﻿# QMC2-Decode

解密 `mflac` / `mgg1` 文件到 `flac` / `ogg` 文件。

## 构建

注意：克隆仓库的时候 submodule 也要一并同步。

```sh
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

二进制文件可以在 `build/QMC2-decoder/QMC2-decoder` 找到 (Windows 下加上 .exe)。

## 可执行文件下载

Windows 版本需要最新的 [VC++ x64 运行时][vs2022_runtime]。

二进制文件可以在 [Release][latest_release] 区找到。

## 使用方式

```sh
QMC2-decoder encrypted_file.mflac decrypted.flac
```

注：如果解密失败，可以尝试将文件名中的非 ASCII 字符去掉后尝试。
    Linux 因为使用 UTF-8 编码因而没有该问题。

## 致谢

- [2021/08/26 MGG/MFLAC研究进展][research] by @ix64 & @Akarinnnnn
- [unlock-music 项目][unlock-music]
- 使用 Visual Studio 2022 进行开发

[research]: https://gist.github.com/ix64/bcd72c151f21e1b050c9cc52d6ff27d5
[unlock-music]: https://github.com/unlock-music/unlock-music
[latest_release]: https://github.com/jixunmoe/qmc2/releases/latest
[vs2022_runtime]: https://aka.ms/vs/17/release/vc_redist.x64.exe
