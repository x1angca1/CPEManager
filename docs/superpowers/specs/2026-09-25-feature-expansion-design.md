# CPEManager 功能扩展设计（十项）

- 日期：2026-09-25
- 状态：待用户评审
- 前置调研：原生 Web UI（192.168.8.1）共暴露约 280 个 API；CPEManager 目前使用 24 个。本文档定义在其基础上新增 10 项功能的完整设计。

## 1. 目标

在不重构现有架构的前提下，把原生 Web UI 的高价值功能搬进 CPEManager 桌面端，做到"不打开浏览器也能完成日常管理"。10 项功能分 3 期交付，每期独立可用、独立发布。

## 2. 非目标

- 不做固件自动升级（只做"检查新版本"并提示）。
- 不做 Wi-Fi 二维码分享。
- 不把 C++ 层重构为本地 HTTP 服务（保持 WebView2 消息桥）。
- 不做多语言（界面维持中文）。

## 3. 总体设计

### 3.1 架构（扩展现有两层）

- **协议层**：`CpeProtocol.h/cpp` 新增约 18 个函数，全部沿用现有模式——WinHTTP 发送、`<request>...</request>` XML 请求体、`Parse*` 静态解析函数、`std::wstring& error` 出参。
- **界面层**：`index.html` 新增"高级"页签组；每个功能一个面板，照 `acceleration-ui.js`/`parameters-ui.js` 的注入与消息桥模式实现，C++ 侧在 `main.cpp` WebView2 消息处理中增加对应分支。

### 3.2 能力探测与自动隐藏

- 登录成功后新增调用 `FetchFeatureSwitches()`：读取设备 feature-switch / module-switch 相关端点（如 `/api/global/module-switch` 与各功能自己的 feature-switch 端点），构建"能力表"。
- 某功能能力表为"不支持"（开关关闭 / 接口报错 / 返回空）时，对应面板不渲染。不允许出现点了报错的死按钮。

### 3.3 请求格式事实

已验证：现有协议层请求体为 XML（`<request><reboot>1</reboot></request>` 风格）。所有新端点按此格式实现；响应字段名的唯一权威来源是原生 UI 的 `main.js` 解析代码（实施时逐个提取），不做臆测字段。

## 4. 分期明细

### P1（低风险，先行发布）

| 功能 | API | 行为 |
|---|---|---|
| 一键重启 | `POST /api/device/control`（body: reboot=1） | 工具栏按钮；确认对话框（防误触）；重启期间界面显示"设备重启中"并自动重连 |
| 每日流量限额 | `GET/POST /api/monitoring/daily-data-limit` | 设置限额值与提醒开关；今日用量达到限额时主界面红字提醒（复用现有流量统计轮询） |
| 清零流量统计 | `POST /api/monitoring/clear-traffic` | 统计面板内按钮，确认后清零 |
| 信号历史曲线 | 无新 API（复用现有 FetchSignals 轮询） | 见 §5 |
| 固件更新检查 | `GET /api/online-update/status` + `POST /api/online-update/check-new-version` | 显示当前版本 vs 最新版本；有新版只提示+显示说明，不自动升级 |
| CI 单元测试 | — | build.yml 增加步骤：用 tests/build-*-test.bat 构建并运行全部非 Live 测试，失败则构建失败 |

### P2

| 功能 | API | 行为 |
|---|---|---|
| Wi-Fi 管理 | `GET/POST /api/wlan/multi-basic-settings`、`multi-switch-settings`、`guesttime-setting`、`POST /api/wlan/wps-pbc` | 列出全部 SSID：改名、改密码、开关、隐藏广播；访客网络开关+时限；WPS 按钮触发（带倒计时状态） |
| USSD 查询 | `POST /api/ussd/send`、`GET /api/ussd/get`、`POST /api/ussd/release` | 输入 `*100#` 类代码；轮询取结果文本显示；会话结束自动 release；设备不支持则隐藏 |

### P3

| 功能 | API | 行为 |
|---|---|---|
| 天线对准助手 | `/api/antennascan/scan-operate`、`scan-status`、`scan-mcu-version` | 启动扫描→轮询状态→结果按步骤展示信号强度，指引外接天线方向 |
| 最佳位置引导 | `/api/monitoring/best-signal-position`、`-status`、`-result` | 启动引导→按步骤提示移动设备→结果给出最佳位置评分 |
| eSIM 配置切换 | `GET /api/sim/esim-profiles`、`POST /api/sim/operate-profile` | 列出 profile，切换需确认（切换会导致断网重连）；无 eSIM 能力则隐藏 |
| 端口转发/DMZ/UPnP | `GET/POST /api/security/virtual-servers`、`dmz`、`upnp` | 端口转发表格+增删表单；DMZ 开关+IP；UPnP 开关 |

## 5. 信号历史（唯一有本地状态的功能）

- 存储：EXE 同目录 `signal-history.json`（与 `cpe_login.dat` 同级的 `PathOf()` 模式）。
- 记录：每次信号轮询追加 `{t, rsrp, rsrq, rssi, sinr, band}`；落盘节流（至少 60 秒一次 + 退出时落盘）。
- 保留：7 天，超期清理。
- 展示：新面板 Canvas 折线图（不引第三方库），时间档位 10 分钟 / 1 小时 / 24 小时 / 7 天。
- 损坏容错：JSON 解析失败则备份改名旧文件后从空历史开始，不崩溃。

## 6. 错误处理

- 每个新面板内部展示红字错误（沿用现有模式），不影响其他面板。
- 能力探测失败（探测接口本身异常）：显示全部面板，单个面板报各自的错——宁可可见可诊断，不可整页消失。

## 7. 测试与发布

- 每个新协议函数配 `tests/*Test.cpp` 解析测试（样例 XML 固定写死在测试里），沿用现有 `tests/build-*-test.bat` 模式；Live 探针类测试不进 CI。
- 本机无 MSVC：开发循环 = 提交 → Actions 跑测试+构建+发 Release → 用户下载实测。
- 每期一个提交批次；期内在 GitHub 上以功能为粒度提交。

## 8. 里程碑

1. P1 合并发布 → 用户实测通过
2. P2 合并发布 → 用户实测通过
3. P3 合并发布 → 用户实测通过
