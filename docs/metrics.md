# 轻量指标实测记录

## 验收门槛

| 指标 | 门槛 | 说明 |
| --- | --- | --- |
| 单 exe 体积 | 数 MB 量级 | 无随包 DLL、无运行时 |
| 待机 Working Set | **< 20MB** | 程序启动后不唤起截图、静置采样 |
| 会话结束回收值 | 回到待机水平 | 确认或取消截图后，全屏位图应被释放 |
| 唤起后峰值 | 记录，不设门槛 | 会话期间持有全屏 DIB 属预期行为 |

## 测量方法

1. `Release` 构建，取 `build/bin/Release/ScreenShotApp.exe` 的字节数。
2. 运行 `scripts/measure-metrics.ps1`，脚本启动进程、静置采样 Working Set / Private Bytes，再结束进程。
3. 手写复核：任务管理器或 Process Explorer 中观察不唤起截图时的内存，并对比唤起、结束一次截图后的回收值。
4. 若不达标，优先排查是否残留全屏位图、GDI+ 缓存或未释放的会话级缓冲。

## 测量环境

| 项 | 值 |
| --- | --- |
| 系统 | Windows 11（内核 10.0.22621） |
| 工具链 | VS 2022 Community / MSVC v143 (14.44.35207) / CMake 3.31.6-msvc6 |
| SDK | Windows SDK 10.0.26100.0 |
| 构建配置 | Release / x64 / `/MT` |

## 记录

数据由 `scripts/measure-metrics.ps1` 采集。下表为加入设置窗口、托盘双击 / 重复启动唤起，并将保存目录改为系统 MRU 记忆（移除自有缓存模块）后的最终 Release 构建采样，两种启动形态各一次。

| 阶段 | exe 字节数 | 待机 Working Set | 待机 Private Bytes | 唤起后峰值 | 会话结束回收值 | 门槛结论 |
| --- | --- | --- | --- | --- | --- | --- |
| 静默后台（`--autostart`） | 210432（205.5 KB） | 11.31 MB | 1.86 MB | 41.87 MB | 1.20 MB | **通过** |
| 设置窗口可见 | 210432（205.5 KB） | 11.95 MB | 2.04 MB | 42.48 MB | 1.75 MB | **通过** |

两次采样 `Pass20MB` 均为 True。会话期间 Private Bytes 约 16.4~16.5 MB，结束后回落到 2.27~2.39 MB，证明全屏 DIB 与标注缓冲已按会话级释放；会话结束调用 `SetProcessWorkingSetSize(..., -1, -1)` 促系统裁剪工作集，回收 Working Set 降到 1 MB 量级。

体积演进：195.5 KB（PoC）→ 198.5 KB（自定义图标、图标工具栏、实时框选）→ 205.5 KB（设置 GUI 与热键配置）→ 221 KB（重复启动唤起、自有保存路径缓存）→ 205.5 KB（删除自有缓存，保存目录改用系统 MRU）。

## 验收结论

- 单 exe 205.5 KB，无随包 DLL、无运行时依赖，**满足数 MB 量级要求**。
- `dumpbin /DEPENDENTS` 实测仅依赖 `gdiplus.dll`、`USER32.dll`、`GDI32.dll`、`SHELL32.dll`、`ole32.dll`、`COMDLG32.dll`、`KERNEL32.dll`、`ADVAPI32.dll`，全部为系统自带 DLL；无 vcruntime / msvcp / .NET 依赖，证实 `/MT` 静态链接生效。
- HKCU 自启项默认未写入，符合「不擅自修改系统状态」；自启开关需显式执行命令行。
- 会话创建（`--snapshot`）与结束（`--cancel`）均已实跑验证，进程可正常驻留并优雅退出，无残留进程。
- `GetProcessDpiAwareness` 实测返回 2（PER_MONITOR_AWARE，对应 PerMonitorV2 上下文），确认 DPI 感知在进程启动早期生效；`--quit` 可优雅退出。
- 本轮交互验证：托盘双击与重复启动均能把隐藏的设置窗口唤起并置前，进程数保持为 1；另存为对话框初始目录由系统 MRU 接管（首次默认「图片」，之后记忆上次目录），移除原有的 exe 同目录 `.savepath` 缓存文件。
- 未覆盖：真实多显示器 / 混合 DPI 缩放环境下的选区坐标实测，以及真人操作下的框选手感、标注、Esc 退出与剪贴板输出，需在目标机器上手工确认。
