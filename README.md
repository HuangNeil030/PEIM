# PEIM

下面我幫你整理成一份 **README 教學版筆記**，主題聚焦在：

* `OneDemoApp.efi` 函數使用方法
* 系統架構與主選單流程
* 你目前「實際發生的流程」分析
* 為什麼 `ReadHobApp.efi` 會找不到 HOB

---

# `OneDemoApp` / `SetVariableApp` / `ReadHobApp` 筆記（README 版）

# 1. 專案目標

本專案原始目標是展示以下五個 UEFI / PEI 相關功能概念：

1. Create a new PPI in PPILIB
2. Create a new PEIM providing PPI services
3. Create another PEIM using PPI
4. Write an APP for set variable
5. Write an APP for read HOB

由於上述功能跨越了 **PEI phase** 與 **UEFI Shell / Application phase**，因此在實作上分成兩種版本：

* **完整專案版**：PEIM 與 Application 分離，符合真實架構
* **單檔展示版 `OneDemoApp.efi`**：在 Shell 中用一支 `.efi` 展示五個功能概念

---

# 2. 專案版本說明

## 2.1 真實架構版

真實架構應該拆成：

* `ProviderPeim`
* `ConsumerPeim`
* `VariableReaderPeim`
* `HobProducerPeim`
* `SetVariableApp`
* `ReadHobApp`

這是符合 UEFI / PI 規範的標準作法。

---

## 2.2 單檔展示版

`OneDemoApp.efi` 是為了方便在 Shell 中一次展示五個功能而設計的。

### 在 `OneDemoApp.efi` 中：

* `SetVariable()`：**真實執行**
* PPI / PEIM / HOB：**概念模擬**

也就是說：

* `Write an APP for set variable` 是真的
* `Create PPI / Provide PPI / Use PPI / Read HOB` 是示範流程用模擬版

---

# 3. 系統架構與主選單流程 (System Architecture & Menu Flow)

## 3.1 系統架構圖（概念版）

```text
+--------------------------------------------------+
|                OneDemoApp.efi                    |
|         (Single UEFI Shell Demo Application)     |
+--------------------------------------------------+
|  Menu UI Layer                                   |
|  - 上下鍵選單                                     |
|  - Enter 執行                                     |
|  - 顏色高亮顯示                                   |
+--------------------------------------------------+
|  Demo Logic Layer                                |
|  1. Create PPI in PPILIB                         |
|  2. Provide PPI Service                          |
|  3. Use PPI Service + Build HOB                  |
|  4. Set Variable                                 |
|  5. Read HOB                                     |
|  6. Run All                                      |
+--------------------------------------------------+
|  Data / State Layer                              |
|  - mPpiCreated                                   |
|  - mPpiInstalled                                 |
|  - mHobBuilt                                     |
|  - mDemoPpi                                      |
|  - mDemoHob                                      |
+--------------------------------------------------+
|  UEFI Service Layer                              |
|  - gRT->SetVariable()                            |
|  - gST->ConIn / ConOut                           |
|  - gBS->WaitForEvent()                           |
+--------------------------------------------------+
```

---

## 3.2 主選單流程圖（Menu Flow）

```text
+----------------------+
| Start OneDemoApp     |
+----------+-----------+
           |
           v
+----------------------+
| 顯示主選單            |
| OneDemoApp Menu      |
+----------+-----------+
           |
           v
+----------------------+
| 上下鍵移動選項        |
| Enter 確認            |
+----------+-----------+
           |
           v
   +-------+--------+--------+--------+--------+--------+--------+
   |                |        |        |        |        |        |
   v                v        v        v        v        v        v
 Function 1      Function 2 Function 3 Function 4 Function 5 Run All Exit
 Create PPI      Provide    Use PPI    Set Var    Read HOB
                 PPI        + Build HOB
   |                |        |        |        |        |
   +----------------+--------+--------+--------+--------+
                            |
                            v
                  顯示執行結果 / 等待按鍵
                            |
                            v
                        返回主選單
```

---

# 4. 主選單功能說明

## 4.1 Function 1

## Create a new PPI in PPILIB

### 功能目的

建立一個模擬的 PPI 結構，展示 PPI 的基本形式。

### 內部動作

* 建立 `DEMO_PPI`
* 把 `Hello` 函式指標指向 `DemoHelloImpl`
* 設定 `mPpiCreated = TRUE`

### 顯示結果

* PPI GUID
* Hello function pointer

### 對應概念

這代表：

> PPI 本質上是一個「函式指標集合的介面結構」

---

## 4.2 Function 2

## Create a new PEIM providing PPI services

### 功能目的

模擬 Provider PEIM 提供 PPI service 的流程。

### 內部動作

* 檢查 `mPpiCreated`
* 設定 `mPpiInstalled = TRUE`

### 顯示結果

* `Simulated InstallPpi()`
* PPI registered into local demo database

### 對應概念

真實 PEI 中通常會做：

```c
PeiServicesInstallPpi(...)
```

這裡改成單檔模擬版。

---

## 4.3 Function 3

## Create another PEIM using PPI

### 功能目的

模擬 Consumer PEIM 找到 Provider 的 PPI 並呼叫 service。

### 內部動作

* 檢查 `mPpiInstalled`
* 模擬 `LocatePpi()`
* 呼叫 `mDemoPpi.Hello(...)`
* 建立模擬 HOB
* 設定 `mHobBuilt = TRUE`

### 顯示結果

* `Simulated LocatePpi()`
* `DemoHelloImpl() called`
* `Simulated BuildGuidHob()`
* HOB GUID / Signature / Value / Flags

### 對應概念

真實 PEI 中通常會做：

```c
PeiServicesLocatePpi(...)
BuildGuidHob(...)
```

這裡改成在同一支 App 裡展示。

---

## 4.4 Function 4

## Write an APP for set variable

### 功能目的

真正呼叫 UEFI Runtime Service 寫入 Variable。

### 內部動作

* 呼叫 `gRT->SetVariable()`
* Variable Name = `MyPeiVar`
* Vendor GUID = `gDemoVarGuid`
* Data = `0x12345678`

### 顯示結果

* `SetVariable Status = Success`
* Variable Name
* Variable GUID
* Variable Data

### 對應概念

這一項是 **真實 UEFI 行為**，不是模擬。

---

## 4.5 Function 5

## Write an APP for read HOB

### 功能目的

在單檔展示版裡讀取「模擬 HOB」。

### 內部動作

* 檢查 `mHobBuilt`
* 若已建立，顯示 HOB 內容
* 若尚未建立，顯示 `GUID HOB not found`

### 顯示結果

* HOB GUID
* Signature
* Value
* Flags

### 對應概念

在真實 PEI / DXE 架構裡，HOB 是跨 phase 傳遞資料的機制。
但在 `OneDemoApp.efi` 中，這裡是示範概念，不是真正從 PEI HOB list 取資料。

---

## 4.6 Function 6

## Run All

### 功能目的

一次依序執行全部示範流程。

### 執行順序

1. Create PPI
2. Provide PPI
3. Use PPI + Build HOB
4. Set Variable
5. Read HOB

### 用途

適合 demo 時一次展示全部功能。

---

# 5. 函數使用方法（README 筆記版）

## 5.1 `SetTextAttr()`

### 功能

設定文字與背景顏色。

### 用法

```c
SetTextAttr(EFI_BLUE | EFI_BACKGROUND_BLACK);
```

### 常見用途

* 藍字標題
* 綠底黑字選中項
* 恢復白字黑底

---

## 5.2 `ResetDefaultTextAttr()`

### 功能

把顯示恢復成白字黑底。

### 用法

```c
ResetDefaultTextAttr();
```

### 注意

印完特殊顏色後最好立刻呼叫，避免後面文字顏色被污染。

---

## 5.3 `PrintHeader()`

### 功能

印出區塊標題。

### 用法

```c
PrintHeader(L"4. Write an APP for set variable");
```

### 效果

會自動輸出上下分隔線，讓畫面更整齊。

---

## 5.4 `WaitAnyKey()`

### 功能

停住畫面，等待使用者按任意鍵。

### 內部 API

* `gBS->WaitForEvent()`
* `gST->ConIn->ReadKeyStroke()`

### 用途

每執行完一個功能後停住，讓使用者看結果。

---

## 5.5 `DrawMenu()`

### 功能

畫出主選單。

### 特點

* `OneDemoApp Menu`：藍字
* 被選中的項目：綠底黑字

### 輸入參數

* `HighlightIndex`：目前高亮項目索引

---

## 5.6 `ReadMenuChoiceByArrow()`

### 功能

用上下鍵移動選項，用 Enter 確認。

### 可識別按鍵

* `SCAN_UP`
* `SCAN_DOWN`
* `CHAR_CARRIAGE_RETURN`

### 回傳

* `EFI_SUCCESS`：成功取得選項
* `EFI_INVALID_PARAMETER`：`Choice == NULL`
* 其他錯誤：鍵盤輸入異常

### 說明

這是目前 UI 的核心函式。

---

## 5.7 `CreatePpiInPpiLib()`

### 功能

建立模擬 PPI 結構。

### 主要動作

```c
mDemoPpi.Hello = DemoHelloImpl;
mPpiCreated = TRUE;
```

### 回傳

* `EFI_SUCCESS`

---

## 5.8 `ProvidePpiService()`

### 功能

模擬 Provider 安裝 PPI。

### 前置條件

* 必須先執行 Function 1

### 失敗情況

若 `mPpiCreated == FALSE`：

* 顯示錯誤
* 回傳 `EFI_NOT_READY`

---

## 5.9 `UsePpiService()`

### 功能

模擬 Consumer 使用 PPI，並建立模擬 HOB。

### 前置條件

* 必須先執行 Function 2

### 失敗情況

若 `mPpiInstalled == FALSE`：

* 回傳 `EFI_NOT_READY`

若 `mDemoPpi.Hello == NULL`：

* 回傳 `EFI_NOT_FOUND`

### 成功後狀態

* `mHobBuilt = TRUE`

---

## 5.10 `SetDemoVariable()`

### 功能

用真實 UEFI Runtime Service 設定 Variable。

### 真實 API

```c
gRT->SetVariable(...)
```

### 寫入內容

* Name = `MyPeiVar`
* GUID = `gDemoVarGuid`
* Data = `0x12345678`

### 回傳

直接回傳 `SetVariable()` 的結果

### 常見結果

* `EFI_SUCCESS`
* `EFI_INVALID_PARAMETER`
* `EFI_OUT_OF_RESOURCES`
* `EFI_WRITE_PROTECTED`
* `EFI_SECURITY_VIOLATION`

---

## 5.11 `ReadDemoHob()`

### 功能

讀取模擬 HOB 內容。

### 前置條件

* 必須先執行 Function 3，或 `Run All`

### 失敗情況

若 `mHobBuilt == FALSE`：

* 顯示 `GUID HOB not found`
* 回傳 `EFI_NOT_FOUND`

---

## 5.12 `RunAll()`

### 功能

一次執行所有功能。

### 順序

```text
CreatePpiInPpiLib()
ProvidePpiService()
UsePpiService()
SetDemoVariable()
ReadDemoHob()
```

### 用途

最適合展示完整流程。

---

# 6. 實際發生的流程（你目前的真實情況）

下面這段是你目前在「原本完整專案版」中，實際發生的流程。

## Step 1

Shell 執行 `SetVariableApp.efi`

```text
SetVariableApp.efi
  -> 成功寫入 Variable
```

### 說明

這一步是成功的，而且是真實寫入 NVRAM variable。

---

## Step 2

重新開機

```text
重新開機
  -> 你的 PEIM 沒有被平台/FV dispatch
  -> 所以沒有 InstallPpi / LocatePpi / GetVariable / BuildGuidHob
```

### 說明

這是整個真實專案版失敗的根本原因。

因為：

* `PeiPpiDemoPeim.efi` 不是 application
* 不能在 Shell 直接執行
* 它必須被 PEI Core 在開機時 dispatch
* 但你目前只是把檔案放在 FS0 / FS4
* 並沒有真的整合進 firmware volume / platform PEI flow

所以 Step 2 沒有成立。

---

## Step 3

Shell 執行 `ReadHobApp.efi`

```text
ReadHobApp.efi
  -> 找不到 HOB
```

### 原因

因為 Step 2 沒真的執行，所以：

* 沒有 `BuildGuidHob()`
* 當然就沒有可讀的 HOB

所以結果只能是：

```text
GUID HOB not found
```

---

# 7. 為什麼最後改成 `OneDemoApp.efi`

因為原始五個功能橫跨：

* PEI phase
* UEFI Shell / Application phase

無法原生整合成一支 Shell 可直接執行的 `.efi`。

因此最終改成：

## `OneDemoApp.efi`

作為 **單檔展示版**

### 特色

* 在 Shell 中直接執行
* 可以一次展示五個功能概念
* UI 友善，適合 demo
* 不依賴平台真的 dispatch PEIM

---

# 8. `OneDemoApp.efi` 的實際展示流程

## Step 1

執行：

```text
FS0:\> OneDemoApp.efi
```

## Step 2

使用上下鍵選擇功能：

* 1：Create PPI
* 2：Provide PPI
* 3：Use PPI + Build HOB
* 4：Set Variable
* 5：Read HOB
* 6：Run All

## Step 3

按 Enter 執行對應功能

## Step 4

查看畫面結果，按任意鍵回主選單

---

# 9. 結論

## 真實完整專案版

目前失敗點不是 `SetVariableApp` 或 `ReadHobApp`，而是：

> **PEIM 沒有被平台 / FV / PEI Core 真正 dispatch**

因此真實 HOB 不會被建立。

---

## 單檔展示版 `OneDemoApp.efi`

這版成功把五個功能的概念整合成同一支可在 Shell 執行的 Demo：

* Create PPI
* Provide PPI Service
* Use PPI Service
* Set Variable
* Read HOB

其中：

* `SetVariable()`：真實
* PPI / PEIM / HOB：模擬展示

這樣的好處是：

* 操作簡單
* 展示完整
* 不受 PEI platform 整合限制

---

# 10. 一句話總結

**`OneDemoApp.efi` 是一支單檔 UEFI Shell 展示程式，用來展示 PPI、PEIM Provider/Consumer、Set Variable、Read HOB 等五個功能的概念流程；其中 SetVariable 為真實 Runtime Service，而 PPI / PEIM / HOB 則以單程式模擬方式呈現。**

如果你要，我下一步可以直接把這份內容再整理成：

**更正式的 Markdown README 成品版**
包含：

* 專案簡介
* 架構圖
* 功能表
* 執行流程
* 截圖說明
* 結論區塊
