<div align="center">

  <img src="assets/AppIcon-256.png" width="160" height="160" alt="Menu11 for Git 图标" />

# Menu11 for Git

**将 Git 命令原生加入 Windows 11 现代右键菜单**

[![Windows 11](https://img.shields.io/badge/Windows_11-x64-0078D4?style=flat&labelColor=33201A&logo=windows11&logoColor=white)](https://www.microsoft.com/windows/windows-11)
[![Native Shell](https://img.shields.io/badge/Shell-Native_C%2B%2B-F4512C?style=flat&labelColor=33201A)](src/Menu11.Shell)
[![WinUI 3](https://img.shields.io/badge/Settings-WinUI_3-7A5AF8?style=flat&labelColor=33201A)](src/Menu11.App)
[![.NET 8](https://img.shields.io/badge/.NET-8-512BD4?style=flat&labelColor=33201A&logo=dotnet&logoColor=white)](https://dotnet.microsoft.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-D4A72C?style=flat&labelColor=33201A)](LICENSE)

[English](README.md) | 中文

</div>

Menu11 for Git 是一个轻量级 Windows 11 Shell Extension，把 Git for Windows
命令直接加入资源管理器原生现代右键菜单。它复用电脑中已有的 Git，不捆绑、
替换或重新实现 Git。

资源管理器会在第一级分别显示 `Git Bash Here`、`Git GUI Here` 和 `Git`。
已启用的操作都是唯一一个 `Git` 子菜单的直接子项，因此菜单始终不超过两层。
`Menu11.exe` 同时是程序本体和设置中心；没有托盘进程、Service、开机启动项、
计划任务或后台常驻组件。

> Menu11 for Git 是独立的第三方项目，与 Git Project 或 Git for Windows
> 没有关联，也未获得其认可或背书。Git 及相关标识归各自权利人所有。

## 功能

### 原生 Windows 11 集成

- 通过 Sparse Package 注册原生、进程外 `IExplorerCommand` Shell Extension，
  不依赖旧版“显示更多选项”。
- `Git Bash Here`、`Git GUI Here` 和 `Git` 均位于第一级菜单。
- 仓库操作：Status、Pull、Fetch、Push、Commit、Log、Branch、Stash。
- 文件操作：Add、Diff、Log、Blame、Restore，并为支持的命令安全处理多选。
- 其他操作：Clone、Init。
- 只有一个扁平的 `Git` 子菜单；每个操作都是叶子命令，无法产生第三级菜单，
  最后一项“设置”可打开设置中心。

### 设置与语言

- 紧凑的 WinUI 3 设置中心，可配置命令可见性、图标、Git 位置并刷新资源管理器。
- 设置中心与资源管理器命令均支持 English、简体中文和系统语言模式。
- 自动检测 Git for Windows，也可指定自定义安装位置。
- 原创的分支与节点视觉标识，并为小尺寸命令设计独立图标；不复制 Git 官方 Logo。

### 更新与进程模型

- `Menu11.exe` 启动时可检查更新，默认开启，也可在“关于”页面手动检查。
- 更新来自本仓库公开的 GitHub Release；安装始终需要用户明确确认。
- 更新器只接受三段式 `vX.Y.Z` Release 和固定命名的 x64 Setup，下载安装程序后
  必须通过已发布 SHA-256 清单校验才会启动。
- Shell 工作交给短时运行的原生 Runner；Menu11 不会让更新器、Runner、托盘或
  设置进程在后台常驻。

## 系统要求

### 使用 Menu11 for Git

- Windows 11 x64，内部版本 22000 或更高。
- 必须先安装 [Git for Windows](https://gitforwindows.org/)。

品牌化安装程序会在显示安装流程前强制检查 Git for Windows，并在实际写入文件前
再次检查。找不到有效的 Git for Windows 安装时不允许安装 Menu11。

### 从源码构建

- Visual Studio 2022 Build Tools 与 MSVC v143 x64 工具集
- Windows 11 SDK 10.0.26100.0
- .NET SDK 8.0.424
- 重新生成视觉资产时需要 Node.js 24 与 npm
- 构建安装程序时需要 Inno Setup 6.7 或更高版本
- 原生集成测试和安装器验证需要 Git for Windows

## 下载与更新

从 [GitHub Releases](https://github.com/CodeH-top/Menu11-For-Git/releases)
下载对应版本的 x64 Setup EXE 及 `.sha256` 文件：

```text
Menu11ForGitSetup-X.Y.Z-x64.exe
Menu11ForGitSetup-X.Y.Z-x64.exe.sha256
```

Setup 会把 Menu11 按当前用户安装到
`%LOCALAPPDATA%\Programs\Menu11 for Git`，注册三个现代右键菜单 Package，并创建
用户选择的快捷方式。此目录由安装程序固定，不能改到其他磁盘或网络位置，因为
Windows 对 Sparse Package 的外部内容路径有额外信任限制。发布包只携带 English
和简体中文资源。裸 MSIX/Sparse Package 只是实现细节，不是正常分发方式。

只有设置程序运行时才会进行自动检查。Menu11 访问公开 GitHub Releases API，
显示可用版本，并等待用户点击**下载并安装**。下载可随时取消；不完整、地址异常或
校验不匹配的安装程序会被删除且绝不会启动。

> 当前 CI 产物尚未进行 Authenticode 签名，因此 Windows 可能显示信誉提示，直到
> 项目引入正式签名。手动下载时请同时核对 Release 中发布的 SHA-256 文件。

## 构建

在 PowerShell 中克隆仓库并构建完整 x64 工程：

```powershell
git clone git@github.com:CodeH-top/Menu11-For-Git.git
cd Menu11-For-Git
./eng/build.ps1 -Configuration Debug -Platform x64
./eng/test.ps1 -Configuration Debug -Platform x64 -NoBuild
```

构建并验证品牌化 Release 安装程序：

```powershell
./eng/build.ps1 -Configuration Release -Platform x64 -Rebuild
./eng/test.ps1 -Configuration Release -Platform x64 -NoBuild
./eng/validate-package-manifests.ps1
./eng/build-installer.ps1 -Configuration Release -Platform x64 -NoBuild
```

重新生成全部应用、Package、安装器和命令图标：

```powershell
./eng/build-assets.ps1
```

## Release 工作流

推送 `v0.1.2` 这种严格三段式 tag 后，
[`Build and publish Release`](.github/workflows/release.yml) 工作流会自动运行。
如果 tag 与 `eng/Product.props` 的版本不同，或 tag 指向的提交不属于 `main`，
工作流会直接拒绝发布。GitHub 托管的 Windows 2025 runner 会重新生成品牌资产、
完整重建 Release x64、运行原生与托管测试、校验全部 Sparse Package Manifest、
构建 Inno Setup 安装器、生成 SHA-256 清单，并把两个文件发布到同一个 GitHub Release。

工作流不会修改或提交版本。请先更新下列两个值并提交到 `main`，再创建匹配 tag：

```xml
<Menu11Version>0.1.2</Menu11Version>
<Menu11FileVersion>0.1.2.0</Menu11FileVersion>
```

第四段固定为零，仅供 Windows/MSIX 技术元数据使用；所有用户可见版本和 Release tag
都严格使用三个数字。

## 架构

| 组件 | 职责 |
| --- | --- |
| `Menu11.exe` | WinUI 3 程序、设置中心、Git 检测和由用户发起的更新流程 |
| `Menu11.Shell.dll` | 在 Explorer 进程外加载的轻量级原生 COM `IExplorerCommand` |
| `Menu11.Runner.exe` | 短时运行的原生命令校验与进程启动边界 |
| Sparse Packages | 为三个 Windows 11 第一级命令提供按用户注册 |
| Git for Windows | 提供 `git.exe`、Git Bash、Git GUI 和 Bash；Menu11 不捆绑这些组件 |

用户选择的路径以独立 UTF-16 进程参数传递，绝不会插入临时拼接的 Shell 脚本。
仓库操作只使用 Menu11 内置的固定脚本和 Bash 位置参数；详见
[`docs/runner-protocol.md`](docs/runner-protocol.md)。

## 项目结构

```text
assets/                         品牌、Package、安装器与命令图标资产
eng/                            构建、测试、暂存、Package 与安装器脚本
installer/                      中英文品牌化 Inno Setup 工程
package/                        三个 Sparse Package Manifest
src/Menu11.App/                 WinUI 3 设置中心与更新交互
src/Menu11.Shell/               原生现代右键菜单 COM Server
src/Menu11.Runner/              短时运行的原生 Runner
src/Menu11.Shared.Managed/      托管设置、Git 检测与更新核心
src/Menu11.Shared.Native/       原生设置、检测、启动与协议代码
tests/                          原生与托管可执行测试套件
docs/                           架构、配置、品牌与验收记录
```

## 隐私与安全

- 无遥测、广告、账号系统或云端数据存储。
- 设置按用户保存在 `HKCU\Software\Menu11ForGit`。
- 无 Service、托盘进程、计划任务、开机启动项或后台更新器。
- 只有启用自动检查或用户点击**检查更新**时，`Menu11.exe` 才会访问更新网络。
- Shell Extension 只进行少量本地读取，并把进程创建交给 Runner，优先保证
  Explorer 稳定性。
- 卸载会移除 Menu11 Package、文件、快捷方式和设置，不修改 Git for Windows
  或用户的 Git 配置。

## 许可证

本项目根据 MIT 许可证授权，详情请参阅 [LICENSE](LICENSE)。

[返回顶部](#menu11-for-git)
