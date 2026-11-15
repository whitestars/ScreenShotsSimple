# ScreenShotApp

一款仅 Windows 的轻量截图工具：后台常驻，全局热键唤起全屏遮罩，框选、标注后把结果送入剪贴板或保存为 PNG。

技术选型结论与量化对比见 [docs/tech-selection.md](docs/tech-selection.md)，轻量指标实测记录见 [docs/metrics.md](docs/metrics.md)。

## 技术栈

C++17 + 原生 Win32 + GDI+，MSVC `/MT` 静态 CRT，零第三方库、零运行时依赖。单个 exe 可直接运行，不分发 VC++ Redistributable、.NET 或任何 DLL。

Release 实测：单 exe 205.5 KB，待机内存 11.31 MB（静默后台）/ 11.95 MB（设置窗口可见），详见 [docs/metrics.md](docs/metrics.md)。

## 环境要求

- Windows 10 / 11（x64）
- Visual Studio 2022，勾选「使用 C++ 的桌面开发」与「适用于 Windows 的 C++ CMake 工具」
- Windows 10/11 SDK

CMake 随 VS 安装时通常不在系统 PATH，请在「x64 Native Tools Command Prompt for VS 2022」或 VS 的 Developer PowerShell 中执行构建命令；也可直接使用 VS 自带 CMake 的绝对路径。

## 构建

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

产物：`build/bin/Release/ScreenShotApp.exe`（Release 启用静态 CRT、体积优化与链接期优化）。

## 运行与设置界面

直接运行 exe 会弹出一个简单设置窗口，同时蓝色取景框图标出现在系统托盘。程序已在运行时再次双击 exe，或双击托盘图标，都会重新打开这个设置窗口（不会启动第二个实例）。关闭设置窗口或点「隐藏到托盘」只会隐藏界面，程序继续常驻；退出请用托盘右键菜单的「退出」。截图会话中唤起设置窗口会先结束当前会话，避免置顶遮罩盖住设置界面。

| 设置项 | 说明 |
| --- | --- |
| 截图快捷键 | `Ctrl+Alt+A`（默认）或 `Shift+Alt+A`；被占用时自动降级，状态栏显示实际生效组合 |
| 开机自动启动 | 勾选后写 `HKCU\...\Run`，并带 `--autostart` 静默启动，开机不弹设置窗口 |

点「保存」立即重新注册热键、更新自启状态，并把选择持久化到 `HKCU\Software\ScreenShotApp`。托盘右键菜单提供「截图」「设置」「退出」，双击托盘图标直接打开设置窗口。若 Windows 11 把新图标折叠进托盘溢出区，可在「设置 > 个性化 > 任务栏 > 其他系统托盘图标」中把它设为显示。

## 截图快捷键

| 操作 | 快捷键 |
| --- | --- |
| 唤起截图 | `Ctrl+Alt+A` 或 `Shift+Alt+A`（依设置） |
| 取消 / 退出截图 | `Esc`（任意截图状态直接退出；文字输入中仅放弃当次输入） |
| 完成并复制到剪贴板 | `Enter` 或选区内双击 |
| 另存为 PNG | `Ctrl+S` 或工具栏保存图标 |
| 撤销上一笔标注 | `Ctrl+Z` 或工具栏撤销图标 |
| 切换工具 | `1` 矩形、`2` 椭圆、`3` 箭头、`4` 画笔、`5` 文字、`6` 马赛克 |
| 文字输入 | 选文字工具后点击落点，输入内容，`Enter` 提交、`Esc` 放弃 |

选中标注工具后可连续使用，不会画完一笔就跳回框选；按 `Esc` 直接退出整个截图会话。拖拽框选时选区实时亮显并显示尺寸。工具栏为纯图标按钮，鼠标悬停显示名称。框选完成后选区固定，重新框选直接再次拖拽。

## 开机自启（命令行）

也可以不打开设置窗口，直接写 `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` 的单个值，无需管理员权限：

```bat
ScreenShotApp.exe --autostart-enable
ScreenShotApp.exe --autostart-disable
ScreenShotApp.exe --autostart-status
ScreenShotApp.exe --quit
```

`--quit` 通知已运行的实例退出；`--autostart` 为静默启动；`--snapshot` 唤起截图，`--cancel` 结束当前截图会话（供自动化采集使用）。重复启动不会创建第二个实例：无参数启动会把已运行实例的设置窗口带到前台。

指标采集：`powershell -ExecutionPolicy Bypass -File scripts/measure-metrics.ps1`，可用 `-LaunchArgs '--autostart'` 测量静默后台形态。

## 输出行为

- 「完成」把选区连同标注写入剪贴板（`CF_BITMAP` 与 PNG 格式）。
- 「保存」弹出另存为对话框，默认文件名带时间戳，编码为 PNG。初始目录由系统管理：首次默认定位到「图片」，之后自动记忆上次保存所在的目录（Windows 公共对话框的 MRU，与系统其它保存对话框共享），不引入额外缓存文件，删除应用也不会残留配置。
- 会话结束立即释放全屏位图与标注缓冲，并让系统裁剪工作集，待机态不持有图像数据。

## 首版范围

- 已包含：全局热键（两种可选组合）、单实例、自定义托盘 / 应用图标、设置窗口（托盘双击 / 重复启动唤起）、全屏遮罩、实时框选、六类标注、撤销、图标工具栏、剪贴板 / PNG 输出、保存目录记忆（系统 MRU，无额外缓存文件）、开机自启、多屏与 Per-Monitor DPI v2。
- 暂不包含：选区移动与边缘微调、窗口边框自动识别、取色器、截图钉在桌面、长截图 / 滚动截图、OCR、录屏、自动更新、安装包。

## 实现说明

- DPI 感知通过进程启动早期的 `SetProcessDpiAwarenessContext` 设置（带旧系统回退），不嵌入自定义 manifest，避免与链接器自动生成的 manifest 冲突。
- 遮罩使用覆盖整个虚拟桌面的置顶无边框窗口 + 内存 DC 双缓冲绘制，未走分层窗口 `UpdateLayeredWindow` 路径，以保证 GDI 与 GDI+ 混合绘制的可靠性。
- 设置窗口是无模式 Win32 对话框；资源脚本保持纯 ASCII，中文与快捷键文本由代码设置，避免资源编译器的编码问题。
- 重复启动靠单实例 mutex 识别，第二实例向消息宿主窗口 `PostMessage` 唤起设置窗口，并先放开前台窗口限制。
- 保存目录交给系统公共对话框的 MRU 记忆（首次默认「图片」），不写入额外缓存文件或注册表项，失效目录由系统自行回退。
- 屏幕捕获使用 `BitBlt` 抓虚拟桌面，兼容多显示器负坐标；DirectX 全屏内容可能存在捕获限制。
- 失败路径（热键注册冲突、抓屏失败、编码 / 剪贴板失败）只写 `OutputDebugString`，不记录屏幕图像内容。

## 目录结构

```
src/app/          Application 生命周期、热键、设置持久化、设置窗口、开机自启
src/capture/      屏幕捕获、PNG 编码与剪贴板输出
src/overlay/      遮罩窗口、框选交互、图标工具栏
src/annotation/   标注命令接口、六类图形、撤销栈
src/common/       DPI 与坐标换算、Win32 资源 RAII 包装
src/app.ico       蓝色取景框图标
src/resources.rc  图标与设置对话框资源
scripts/          轻量指标采集脚本
docs/             选型评估与指标记录
```
