`1. 当前 sdk 版本 v3.2.0，首次从 v2.x 升级需先擦除 Flash 或下载 FLS 文件,`

`2. 重点优化了低功耗模式，目前功耗有明显改善。`

`3. 当前支持 2M flash 版本 W600，单用户区高达960KB（设置环境变量FLASH_SIZE=2M）。`

# 更新说明

请查看 [ChangeLog](./doc/ChangeLog.txt)
    
# 配置环境

## Windows

### C 编译器

1. [点击这里](https://launchpad.net/gcc-arm-embedded/4.8/4.8-2014-q1-update)下载GCC，建议选择`Windows zip package`，`Windows installer`亦可
1. 解压（Windows zip package）或安装（Windows installer）
1. 修改`cmd_py3_gcc4.cmd`、`eclipse_py3_gcc4.cmd`和`vscode_py3_gcc4.cmd`内的GCC路径`GCC_HOME`

### 构建工具
1. 到[这里](https://www.python.org/downloads/windows/)下载**Python3**并安装

   > `Optional Features`页面必须勾选`pip`和`py launcher`，`Advanced Options`页面必须勾选`Associates files with Python`。

1. 启动`命令提示符`

1. 逐一输入下列命令执行

   ```batch
   pip install bcolors
   pip install colorama
   pip install pyprind
   pip install scons
   pip install serial
   pip install xmodem
   ```

1. 修改`cmd_py3_gcc4.cmd`、`eclipse_py3_gcc4.cmd`和`vscode_py3_gcc4.cmd`内的Python路径`PYTHON_HOME`
  
### 调试器
#### Eclipse IDE
1. 到[这里](https://www.eclipse.org/downloads/packages/release/oxygen/3a)下载`Eclipse IDE for C/C++ Developers`并解压
1. 修改`eclipse_py3_gcc4.cmd`内的`Eclipse`路径`ECLIPSE_PATH`
1. 到[这里](http://www.sconsolidator.com/projects/sconsolidator/wiki/Installation)安装`SConsolidator`
1. 到[这里](https://gnu-mcu-eclipse.github.io/plugins/install/)安装`GNU MCU Eclipse`

#### Visual Studio Code
1. 到[这里](https://code.visualstudio.com/Download)下载`Visual Studio Code`并安装
1. 修改`vscode_py3_gcc4.cmd`内的`VSCode`路径`VSCODE_PATH`


# 编译

## 使用 Eclipse

   1. 双击`eclipse_py3_gcc4.cmd`启动`Eclipse`
   1. 导入工程
   1. 在`Project Explorer`窗口内右键点击项目名称，进入`SCons`子菜单，点击`Interactive build`。

## 使用命令行

   1. 双击`cmd_py3_gcc4.cmd`启动`命令提示符`
   1. 输入`scons`，回车执行。
   > 使用linux平台编译时，需添加`tools/makeimg`和`tools/makeimg_all`的执行权限，例如`chmod +x makeimg`

## 使用 Keil

   1. 打开`WM_SDK/tools/Keil/Project/WM_W600.uvproj`
   1. 点击`Project`下的子菜单`Build Target`

# 烧写

   1. 双击`cmd_py3_gcc4.cmd`启动`命令提示符`

   1. 输入`flasher Debug/bin/example.blink.blink_gz.img COM11`，回车执行。

   > 成功下载一次之后，再使用同一个串口下载时无需再次指定，即重复烧写命令可为`flasher Debug/bin/example.blink.blink_gz.img`

# 其它
1. 有任何疑问，都不要问我。