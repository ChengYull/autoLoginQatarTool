# AutoLoginQatarTool

Qatar 客户端自动登录工具。常驻系统托盘，自动监测 `QaTar.exe` 启动,并在其弹出“密码对话框”时自动填入账户与密码、点击登录。

当前版本:`v1.0.3`(Qt Widgets / Windows 桌面,界面为无边框圆角卡片风格,窗口与托盘标题均不显示版本号)

---

## 功能特性

| 功能 | 说明 |
| --- | --- |
| 进程监测 | 每 500 毫秒用系统进程快照检测一次 `QaTar.exe`,启动后自动触发登录 |
| 自动填账户 | 自动向对话框的 `Emp ID / 员工ID` 下拉框填入账户 |
| 自动填密码 | 先清空输入框再逐字符输入密码,规避残留内容 |
| 密码错误识别 | 登录后检测错误提示,命中时提示“密码与工号不匹配”,不再重复尝试 |
| 重置密码 | 调用 `forgetpwd` 接口按工号重置,新密码由服务端发送到钉钉 |
| 多语言环境自适应 | 同时兼容**简体中文 / 繁体中文 / 英文**三种客户端界面,无需手动切换 |
| 开机自启 | 卡片式开关,写入 `HKCU\...\CurrentVersion\Run` |
| 系统托盘 | 关闭窗口仅最小化到托盘,双击托盘图标恢复,右键菜单退出 |
| 配置持久化 | 账户、密码、自启开关自动保存,重启后回填 |
| 现代界面 | 无边框圆角卡片 + 自定义标题栏,密码支持显示/隐藏切换 |
| 矢量图标 | 参考 Qatar 客户端的相机造型设计,窗口 / 任务栏 / 托盘 / exe 内嵌图标共用同一套 SVG |

---

## 环境要求

- Windows 10 / 11 (x64)
- Qt 6.5.x (MinGW 64-bit),需要 **Qt Svg** 与 **Qt Network** 模块(工程已通过 `QT += svg network` 声明)
- MinGW 11.2.0 或以上

> 代码使用 `windows.h` 的 Win32 窗口 API 与 `QSystemTrayIcon`,仅支持 Windows 平台。

---

## 编译

### 方式一:命令行(qmake + MinGW)

```powershell
# 1. 将 Qt 与 MinGW 的 bin 目录加入 PATH
$env:Path = "D:\Qt\Tools\mingw1120_64\bin;D:\Qt\6.5.3\mingw_64\bin;" + $env:Path

# 2. 生成 Makefile
mkdir build\release
cd build\release
qmake ..\..\autoLoginTool.pro -spec win32-g++ "CONFIG+=release"

# 3. 编译
mingw32-make -j8
```

产物位于 `build\release\release\autoLoginTool.exe`。

### 方式二:Qt Creator

用 Qt Creator 打开 `autoLoginTool.pro`,选择 `Desktop Qt 6.5.x MinGW 64-bit` 套件,切换到 Release 后构建即可。

---

## 部署

`exe` 无法单独运行,必须带上 Qt 运行库与插件。推荐做法是把 `exe` 复制到独立目录后执行:

```powershell
windeployqt --release --no-translations autoLoginTool.exe
```

> **已知问题**:部分 Qt 6.5.3 MinGW 安装中,`windeployqt` 会把 Release 插件误判为 Debug 版本,报
> `Unable to find the platform plugin.` 此时可手动拷贝以下文件(与 windeployqt 结果一致但更精简):
>
> ```
> Qt6Core.dll  Qt6Gui.dll  Qt6Widgets.dll  Qt6Svg.dll  Qt6Network.dll
> # Qt6Svg 必需，图标渲染依赖它；Qt6Network 必需，重置密码接口依赖它
> libgcc_s_seh-1.dll  libstdc++-6.dll  libwinpthread-1.dll
> platforms\qwindows.dll          # 必需，缺失会直接启动失败
> styles\qwindowsvistastyle.dll   # 原生控件外观
> imageformats\qico.dll  qjpeg.dll  qgif.dll
> ```

### 图标资源(已内嵌)

图标统一为一份 SVG:`assets/icons/app.svg`(相机造型,参考 Qatar 客户端图标)。它有两个入口:

| 用途 | 载体 | 说明 |
| --- | --- | --- |
| 窗口 / 任务栏 / 托盘 | `resources.qrc` 内嵌的 `:/icons/app.svg` | 运行时由 `QSvgRenderer` 渲染成 16~256 的位图 |
| exe 文件图标 | `resources.qrc` 之外,由 `RC_ICONS` 内嵌的 `assets/icons/app.ico` | 资源管理器、快捷方式、任务栏固定时显示 |

`app.ico` 是同一份 SVG 预渲染出的多尺寸图标(16/20/24/32/40/48/64/128/256)。
修改 `app.svg` 后需要重新导出一次 `app.ico`(否则窗口/托盘会立刻更新,但资源管理器里的 exe 图标不变):

```powershell
cd tools\icogen
qmake icogen.pro -spec win32-g++ "CONFIG+=release"
mingw32-make -j8
release\icogen.exe                 # 直接覆盖 assets\icons\app.ico
```

由此**不再需要** `data\img\appico.ico` 这类外部图标文件;因为是自己渲染位图而非依赖 Qt 的图片插件,
部署时也无需额外分发 `iconengines\qsvgicon.dll` 与 `imageformats\qsvg.dll`,只保留 `Qt6Svg.dll` 即可。

---

## 使用说明

1. 运行 `autoLoginTool.exe`,在“账户 / 工号”栏填写工号(`Emp ID`),在“密码”栏填写密码。
2. 按需打开“开机自启动”开关。
3. 点击 **启动自动登录**,状态徽标变为绿色的“运行中”。此状态下账户与密码输入框会被禁用,
   需要修改请先点 **停止**;托盘图标右键菜单里也有同样的启停项。
4. 保持程序在托盘后台运行。当 `QaTar.exe` 启动并弹出密码对话框时,程序会延时 2 秒后自动填入账户与密码并点击登录。
5. 若提示“密码与工号不匹配”,说明账户或密码有误,修正后状态会自动恢复监测。
6. 点击窗口关闭按钮(或标题栏右上角 ✕)只会最小化到托盘;需右键托盘图标 → **退出** 才会真正结束进程。
7. 密码默认以圆点掩码显示,需要核对时可点输入框内右侧的小眼睛图标切换为明文。
8. 忘记密码时点密码框右侧的 **重置**,程序会用当前账户调用重置接口,新密码由服务端发送到钉钉;
   账户为空时会先提示补填。接口成功时响应体只有一个字符 `1`。

### 配置存储位置

配置通过 `QSettings("Qtech", "autoLoginQatarApp")` 保存在注册表:

```
HKEY_CURRENT_USER\Software\Qtech\autoLoginQatarApp
```

| 键 | 说明 |
| --- | --- |
| `account` | 账户 / 工号 |
| `password` | 密码 |
| `autoStart` | 是否开机自启 |

开机自启项写入:

```
HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run\autoLoginQatarApp
```

> ⚠️ **密码以明文保存在注册表中**,仅适合可信的个人办公机环境。

---

## 工作原理

```
ProcessDetector                      MainWidget
  QTimer(0.5s) ──► 进程快照 ──► loginSignal ──► 查找对话框
                                                    ├─ 填入账户
                                                    ├─ 填入密码
                                                    ├─ 点击登录
                                                    └─ 检测错误提示
```

登录流程的关键点(`mainwidget.cpp` 与 `processdetector.cpp`):

**1. 环境自动识别**

按顺序用 `FindWindow` 探测三种语言环境的对话框,命中即用,彻底取代了早期的“简体/繁体”单选框:

```cpp
const DialogEnv kDialogEnvs[] = {
    { L"密码对话框",      L"登录"  }, // 简体环境
    { L"密碼對話框",      L"Login" }, // 繁体环境
    { L"Password Dialog", L"Login" }, // 英文环境
};
```

**2. 控件定位不依赖控件顺序**

对话框由 wxWidgets 构建,内部结构为 `对话框 → wxWindowNR(面板) → 各控件`。为避免“取第一个 Edit 就是密码框”这类脆弱假设,改为按几何位置定位:

| 控件 | 定位规则 |
| --- | --- |
| 密码框 | 面板内**最下方**的可见 `Edit` |
| 账户框 | 面板内 `ComboBox` 中,**位于密码框上方且纵坐标最大**的一个(服务器、端口也在上方) |
| 登录按钮 | 逐个环境匹配按钮文本 `登录` / `Login`(与 `加密狗` / `Keypro` 不重名) |
| 错误提示 | 面板内任意 `Static` 文本包含 `ec: -3` |

**3. 错误提示判定**

三种语言下客户端的提示均为同一串(中文括注为客户端硬编码,不做本地化),因此只需匹配与语言无关的返回码:

```
Right: 0, Area: 0x99, ec: -3(远端响应帐密错误)
```

**4. 写入账户的策略**

若账号已在 `ComboBox` 的下拉列表中,则用 `CB_SETCURSEL` 直接选中;否则回退为 `WM_SETTEXT` 写入文本。账户留空时**跳过填充**,保留对话框原有内容,避免清空导致登录失败。

**5. 启停状态机**

`MainWidget` 用三态枚举维护状态,避免早期“先停后启”写法在延时窗口内出现状态不一致(那时点「停止」会误报“自动登录未启用”):

| 状态 | 含义 | 界面显示 |
| --- | --- | --- |
| `Stopped` | 未启用 | 已停止(红) |
| `Running` | 监测中 | 运行中(绿) |
| `Handling` | 已检测到客户端,正在执行登录动作 | 登录中(橙) |

`Handling` 期间点「停止自动登录」是合法操作:会取消待执行的登录动作(`loginTimer->stop()`)并回到 `Stopped`,不会被延时回调重新拉起。另外 `login()` 进入处理前会先确认对话框确实存在,登录成功后客户端仍驻留时不会反复空转。

**6. 进程监测的开销**

原先每轮起一个 `tasklist` 子进程并同步 `waitForFinished()`,实测单次约 **200 毫秒**,期间界面线程完全阻塞。现改为一次 `CreateToolhelp32Snapshot` 系统调用,实测约 **7~9 毫秒**(随系统进程数变化),不再创建子进程,界面无可感知卡顿。

> 本机实测 `EnumProcesses`(只取 PID 列表)也需约 6.8 毫秒,说明该量级是进程枚举本身的开销下限,与 API 选型关系不大。

---

## 项目结构

```
autoLoginQatarTool/
├── autoLoginTool.pro     # qmake 工程文件
├── resources.qrc         # SVG 图标资源
├── assets/icons/
│   ├── app.svg           # 应用图标源文件（窗口 / 任务栏 / 托盘共用）
│   ├── app.ico           # 由 app.svg 导出的多尺寸图标，经 RC_ICONS 内嵌进 exe
│   ├── eye.svg           # 密码框右侧「显示明文」图标
│   └── eye-off.svg       # 密码框右侧「隐藏明文」图标
├── main.cpp              # 程序入口
├── mainwidget.h/.cpp     # 主窗口界面、托盘、设置持久化、窗口自动化登录
├── tools/icogen/         # 由 app.svg 生成多尺寸 app.ico 的小工具
├── toggleswitch.h/.cpp   # 开机自启用的现代化滑动开关（纯 QPainter 绘制）
├── processdetector.h/.cpp# 基于系统进程快照(Toolhelp32)的进程轮询检测
├── README.md
└── .gitignore
```

---

## 常见问题

**Q:双击 exe 无反应 / 报缺少 DLL?**
未部署 Qt 运行库或 `platforms\qwindows.dll`,参考上文“部署”一节。

**Q:托盘区域没有图标 / 窗口图标空白?**
图标已内嵌在 exe 中,先确认部署目录含 `Qt6Svg.dll`(缺失时 `QSvgRenderer` 无法解析,图标会渲染为空)。
若任务栏通知区只显示一个「^」折叠面板,是 Windows 11 的默认行为——把图标从折叠区拖到任务栏即可常驻显示。

**Q:提示“密码与工号不匹配”但密码确认无误?**
确认“账户”栏填写的工号与对话框中 `Emp ID` 的实际选择项一致;部分环境下下拉框为只读,写入未在列表中的工号会无效。

**Q:客户端界面语言换成了其他语言?**
目前仅支持简体、繁体、英文三种。新增语言只需在 `kDialogEnvs` 表中补充“对话框标题 + 登录按钮文案”即可。
