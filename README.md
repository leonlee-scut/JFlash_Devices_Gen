# JFlash Devices Generator

## 1. 项目背景

本项目用于将 Arm CMSIS Device Family Pack（`.pdsc` 和对应的 `.pack` 文件）转换为 SEGGER J-Link Device Support Kit 所需的设备目录结构。

CMSIS Pack 通常包含以下信息：

- 芯片厂商、器件名称和器件系列
- CPU 内核类型
- Flash、SRAM、Option Bytes、OTP 等存储器区域
- CMSIS Flash Algorithm（`.FLM` 文件）及其地址和容量
- 调试相关的 SVD、Debug 配置和调试序列

J-Link 设备支持目录则主要通过 `Devices.xml` 描述器件和 Flash Bank，并在 `Flash` 子目录中放置实际的 Flash Loader 文件。两者的描述格式和目录组织方式不同，手工转换容易出现器件遗漏、地址错误或 FLM 文件路径不匹配等问题。

本仓库以 Puya PY32F0 系列为当前示例，使用 `Puya.PY32F0xx_DFP.1.2.19.pdsc` 作为输入，并生成 `JLinkDevices/Puya/PY32F0/` 下的 J-Link 设备支持文件。

## 2. 项目结构

```text
.
├── generate_jlink_devices.py              # 转换脚本
├── Puya.PY32F0xx_DFP.1.2.19.pdsc          # CMSIS PDSC 输入文件
├── JLinkDevices/                          # 已生成的 J-Link 设备支持目录
│   └── Puya/
│       └── PY32F0/
│           ├── Devices.xml
│           └── Flash/
│               └── *.FLM
└── doc/                                   # 参考文档和原始资料
```

`JLinkDevices/` 下的文件是生成结果。重新生成时，建议使用单独的输出目录，或确认输出目录中的文件可以被覆盖/更新。

## 3. 转换思路

整体处理流程如下：

1. 解析命令行参数，获取输入 PDSC、CMSIS Pack 根目录和输出目录。
2. 使用 XML 解析器读取 PDSC。
3. 从 PDSC 的 `<vendor>`、`<name>` 和第一条 `<release version="...">` 中确定 Pack 的身份和版本。
4. 在以下位置查找对应的 CMSIS Pack：

   ```text
   CMSIS_PACK_ROOT/.Download/<vendor>.<name>.<version>.pack
   ```

   对当前项目而言，期望的文件名是：

   ```text
   Puya.PY32F0xx_DFP.1.2.19.pack
   ```

5. 遍历 PDSC 中的 `<family>`、`<subFamily>` 和 `<device>`：
   - 将 Cortex-M 内核映射到 J-Link 的 Core 常量。
   - 读取器件名称和可写 SRAM，生成 `ChipInfo`。
   - 合并子系列继承的 Flash Algorithm 和器件自身的 Algorithm。
   - 根据 Algorithm 的起始地址区分内部 Flash 与 Configuration 区域。
   - 将 Algorithm 的地址和大小转换成 J-Link 的 `FlashBankInfo`/`LoaderInfo`。
6. 从 `.pack` 压缩包中提取 PDSC 引用的 `.FLM` 文件，放入生成目录的 `Flash/` 子目录。
7. 写出格式化后的 `Devices.xml`，并打印生成文件的路径。

### 地址区域判断

脚本使用以下规则将 PDSC 中的 Algorithm 划分为 J-Link Flash Bank：

- 起始地址小于 `0x10000000`：`Internal Flash`，并设置 `AlwaysPresent="1"`。
- 起始地址大于或等于 `0x10000000`：`Configuration`，并设置 `AlwaysPresent="0"`。

因此，Option Bytes、OTP 等高地址区域会作为 Configuration 区域输出。该规则适合当前项目的 PDSC 数据；如果后续接入其他厂商的 Pack，应先确认其地址布局符合这一约定。

### CPU 内核映射

脚本支持以下 PDSC `Dcore` 值：

- `Cortex-M0`、`Cortex-M0+`、`Cortex-M0Plus`
- `Cortex-M1`
- `Cortex-M3`
- `Cortex-M4`
- `Cortex-M7`
- `Cortex-M23`
- `Cortex-M33`
- `Cortex-M55`

不在映射表中的内核会导致当前转换失败，并提示对应子系列缺少或不支持处理器内核。

## 4. 运行环境

### 4.1 Python 版本

建议使用 Python 3.9 或更高版本。脚本使用了以下 Python 特性：

- `pathlib`
- 类型注解中的内置泛型，例如 `list[str]`、`set[...]`
- `xml.etree.ElementTree`
- `zipfile`

### 4.2 Python 依赖安装

本脚本只依赖 Python 标准库，不需要通过 `pip` 安装第三方包，也不需要 `requirements.txt`。

在 Windows 中确认 Python 已安装：

```powershell
py --version
```

或者：

```powershell
python --version
```

如果尚未安装 Python，请从 Python 官方网站安装 Python 3.9+，并在安装时启用将 Python 添加到 `PATH` 的选项。

## 5. 准备 CMSIS Pack

脚本不会下载 CMSIS Pack，而是从 CMSIS Pack 管理器的本地缓存目录读取已经下载的 `.pack` 文件。需要确保：

1. PDSC 文件存在且可以正常解析。
2. 对应版本的 `.pack` 文件已经下载。
3. `.pack` 文件位于 `CMSIS_PACK_ROOT\.Download` 目录。
4. `.pack` 文件名与 PDSC 内容一致：

   ```text
   <vendor>.<name>.<release version>.pack
   ```

对于当前输入文件，脚本会查找：

```text
<pack-root>\.Download\Puya.PY32F0xx_DFP.1.2.19.pack
```

`CMSIS_PACK_ROOT` 的具体位置取决于本机使用的 CMSIS-Pack 工具。可以通过 `--pack-root` 显式指定，避免依赖环境变量。

## 6. 使用方法

在仓库根目录打开 PowerShell 或命令提示符。

### 6.1 使用环境变量指定 Pack 根目录

PowerShell：

```powershell
$env:CMSIS_PACK_ROOT = "C:\Users\Public\Documents\Arm\Packs"
py .\generate_jlink_devices.py .\Puya.PY32F0xx_DFP.1.2.19.pdsc
```

命令提示符（cmd）：

```cmd
set CMSIS_PACK_ROOT=C:\Users\Public\Documents\Arm\Packs
py generate_jlink_devices.py Puya.PY32F0xx_DFP.1.2.19.pdsc
```

未指定 `--output` 时，默认输出到脚本所在目录下的：

```text
JLinkDevices\
```

### 6.2 使用 `--pack-root` 显式指定 Pack 根目录

```powershell
py .\generate_jlink_devices.py `
  .\Puya.PY32F0xx_DFP.1.2.19.pdsc `
  --pack-root "C:\Users\Public\Documents\Arm\Packs"
```

在 cmd 中可以写成单行：

```cmd
py generate_jlink_devices.py Puya.PY32F0xx_DFP.1.2.19.pdsc --pack-root "C:\Users\Public\Documents\Arm\Packs"
```

### 6.3 指定输出目录

```powershell
py .\generate_jlink_devices.py `
  .\Puya.PY32F0xx_DFP.1.2.19.pdsc `
  --pack-root "C:\Users\Public\Documents\Arm\Packs" `
  --output .\JLinkDevices
```

`--output` 指向的是整个 J-Link 设备目录根路径。脚本会在此目录下继续创建：

```text
<output>\<vendor>\<family>\Devices.xml
<output>\<vendor>\<family>\Flash\*.FLM
```

### 6.4 查看命令帮助

```powershell
py .\generate_jlink_devices.py --help
```

可用参数：

| 参数 | 是否必需 | 说明 |
| --- | ---: | --- |
| `pdsc` | 是 | 输入 CMSIS `.pdsc` 文件 |
| `--pack-root` | 否 | 覆盖 `CMSIS_PACK_ROOT`，脚本会在其 `.Download` 子目录查找 `.pack` |
| `--output` | 否 | 输出 J-Link 设备目录，默认是脚本旁的 `JLinkDevices` |

## 7. 生成结果

成功运行后，脚本会逐行打印生成的 `Devices.xml` 路径。以当前项目为例，主要结果类似于：

```text
JLinkDevices\Puya\PY32F0\Devices.xml
```

对应目录包含：

- `Devices.xml`：J-Link 设备数据库，包含每个 Device 的 `ChipInfo`、Flash Bank 和 Loader 信息。
- `Flash\*.FLM`：从 CMSIS `.pack` 中提取的 Flash Algorithm 文件。

生成的 `LoaderInfo` 会使用如下路径格式：

```xml
<LoaderInfo
  Name="PY061xx_64.FLM"
  MaxSize="0x10000"
  Loader="Flash/PY061xx_64.FLM"
  LoaderType="FLASH_ALGO_TYPE_OPEN" />
```

## 8. 错误排查

### `CMSIS_PACK_ROOT is not set`

没有设置 `CMSIS_PACK_ROOT`，且没有传入 `--pack-root`。解决方式：

```powershell
py .\generate_jlink_devices.py .\Puya.PY32F0xx_DFP.1.2.19.pdsc --pack-root "C:\path\to\CMSIS_PACK_ROOT"
```

### `CMSIS pack not found`

检查以下内容：

- `--pack-root` 是否指向正确的 CMSIS Pack 根目录。
- `.Download` 子目录是否存在。
- `.pack` 文件名是否精确匹配 PDSC 的厂商、包名和第一条 release 版本。
- Windows 文件名大小写通常不敏感，但名称中的点号、版本号和下划线必须正确。

### `algorithm ... not found in ...`

PDSC 声明了一个 FLM 文件，但该文件不在 `.pack` 压缩包中，或压缩包内路径与 PDSC 中的路径不一致。需要检查 PDSC 的 `<algorithm name="...">` 和 `.pack` 内的文件路径。

### `unsupported or missing processor core`

PDSC 中的 `Dcore` 为空，或不在脚本的 CPU 内核映射表中。需要补充映射后再运行，或确认输入 PDSC 是否为预期版本。

### `invalid start value` / `invalid size value`

PDSC 中的地址或大小不是 Python `int(value, 0)` 可识别的格式。地址和大小应使用类似 `0x08000000` 或十进制数字的表示方式。

### `algorithm is not an FLM file`

PDSC 中引用的 Algorithm 文件不是以 `.FLM` 结尾。当前脚本只生成 J-Link 的 FLM Loader 条目。

## 9. 注意事项

- 脚本会读取 PDSC 第一条 `<release>` 的版本作为 Pack 版本，因此输入文件的 release 顺序应保持最新版本在前。
- 子系列级别的 Algorithm 会继承到该子系列下的所有 Device，并与 Device 级别的 Algorithm 合并。
- 相同的 Algorithm 文件可能被多个 Device 引用，但提取时会去重。
- 输出的 FLM 文件使用压缩包内文件名的最后一部分作为目标文件名；如果不同路径下存在同名 FLM，可能产生覆盖，应在使用前检查。
- `safe_pack_path()` 会拒绝绝对路径和包含 `..` 的路径，避免从 `.pack` 中提取到目标目录之外。
- 当前脚本只转换 J-Link 设备数据库和 Flash Loader，不会复制 PDSC 中的 SVD、头文件、调试配置或调试序列。
- 使用生成结果前，应使用目标版本的 SEGGER J-Link/J-Flash 工具验证设备识别、Flash 擦写和 Configuration 区域访问行为。

## 10. 许可与来源

本项目中的 PDSC、FLM 及其他设备支持文件来自 Puya PY32F0 Device Family Pack 或其相关发布内容。使用和再分发这些文件时，请遵循原始厂商、CMSIS Pack 和 SEGGER 相关许可条款。
