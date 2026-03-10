#include <Uefi.h>                                   // UEFI 核心型別定義；提供 EFI_STATUS、EFI_GUID、EFI_HANDLE、EFI_SYSTEM_TABLE 等基本資料型別

#include <Library/UefiApplicationEntryPoint.h>      // UEFI Application 入口點相關宣告；讓 UefiMain() 可作為應用程式入口
#include <Library/UefiLib.h>                        // 提供 Print() 等常用 UEFI 輸出函式
#include <Library/UefiRuntimeServicesTableLib.h>    // 提供 gRT，全域 Runtime Services 指標；可呼叫 SetVariable() 等 Runtime Service
#include <Library/UefiBootServicesTableLib.h>       // 提供 gBS，全域 Boot Services 指標；可用於 WaitForEvent() 等 Boot Service
#include <Library/BaseLib.h>                        // 提供基礎巨集 / 函式，例如 SIGNATURE_32()
#include <Library/BaseMemoryLib.h>                  // 提供基礎記憶體操作函式；本檔目前未大量使用，但保留給擴充用途
#include <Library/MemoryAllocationLib.h>            // 提供 AllocatePool / FreePool 等動態記憶體配置函式；本檔目前未大量使用，但保留給擴充用途

//
// ============================================================
// Demo GUIDs
// ============================================================
//

STATIC EFI_GUID gDemoPpiGuid =                      // 定義一個靜態 GUID，代表「示範用 PPI 的 GUID」
  { 0x12345678, 0x9abc, 0x4def, { 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11 } };

STATIC EFI_GUID gDemoVarGuid =                      // 定義一個靜態 GUID，代表「示範用 UEFI Variable 的 Vendor GUID」
  { 0xa1b2c3d4, 0x1111, 0x2222, { 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa } };

STATIC EFI_GUID gDemoHobGuid =                      // 定義一個靜態 GUID，代表「示範用 HOB 的 GUID」
  { 0xdeadbeef, 0xaaaa, 0xbbbb, { 0xcc, 0xdd, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 } };

//
// ============================================================
// Simulated PPI definition
// ============================================================
//

typedef                                             // 宣告一個函式指標型別，模擬 PPI 裡面會提供的 service function
EFI_STATUS                                          // 回傳型別是 EFI_STATUS；這是 UEFI 常見的標準回傳值型別
(EFIAPI *DEMO_PPI_HELLO)(                           // EFIAPI 是 UEFI 規範使用的 calling convention；DEMO_PPI_HELLO 是型別名稱
  IN CONST CHAR16 *Message OPTIONAL                 // 輸入參數：一個可選的 Unicode 字串；OPTIONAL 表示可傳 NULL
  );

typedef struct {                                    // 宣告一個結構，模擬「PPI Interface」
  DEMO_PPI_HELLO  Hello;                            // 這個欄位是一個函式指標；表示此 PPI 提供的 Hello 服務入口
} DEMO_PPI;                                         // DEMO_PPI 代表整個 PPI 的介面結構

//
// ============================================================
// Simulated HOB payload
// ============================================================
//

typedef struct {                                    // 宣告一個結構，模擬 HOB 中承載的資料內容
  UINT32 Signature;                                 // Signature：常用來辨識資料格式；這裡會放 'DEMO' 的四字元簽章
  UINT32 Value;                                     // Value：示範資料值；可視為 HOB 中主要 payload
  UINT32 Flags;                                     // Flags：示範旗標欄位；可表示狀態或控制位元
} DEMO_HOB_DATA;                                    // DEMO_HOB_DATA 代表這份模擬 HOB 的內容格式

//
// ============================================================
// Global demo state
// ============================================================
//

STATIC BOOLEAN       mPpiCreated   = FALSE;         // 是否已經建立 PPI 結構；FALSE 表示尚未建立
STATIC BOOLEAN       mPpiInstalled = FALSE;         // 是否已經完成「模擬 InstallPpi()」；FALSE 表示尚未安裝
STATIC BOOLEAN       mHobBuilt     = FALSE;         // 是否已經建立模擬 HOB；FALSE 表示尚未建好
STATIC DEMO_PPI      mDemoPpi;                      // 全域的 PPI 結構實體；用來保存 Hello 函式指標
STATIC DEMO_HOB_DATA mDemoHob;                      // 全域的 HOB 資料實體；用來保存模擬建立的 HOB payload

//
// ============================================================
// Menu strings
// ============================================================
//

#define MENU_ITEM_COUNT  7                          // 定義選單項目數量；目前共有 7 個項目（1~6 + 0 Exit）

STATIC CONST CHAR16 *mMenuItems[MENU_ITEM_COUNT] = {// 宣告一個固定字串陣列；每個元素都是選單顯示文字
  L"1. Create a new PPI in PPILIB",                 // 選單項目 0：對應功能 1
  L"2. Create a new PEIM providing PPI services",   // 選單項目 1：對應功能 2
  L"3. Create another PEIM using PPI",              // 選單項目 2：對應功能 3
  L"4. Write an APP for set variable",              // 選單項目 3：對應功能 4
  L"5. Write an APP for read HOB",                  // 選單項目 4：對應功能 5
  L"6. Run All",                                    // 選單項目 5：一次執行全部功能
  L"0. Exit"                                        // 選單項目 6：離開程式
};

//
// ============================================================
// Helpers
// ============================================================
//

STATIC                                              // STATIC 表示此函式只在本檔案可見，避免外部模組誤用
VOID                                                // 回傳值為 VOID，表示此函式不回傳資料
SetTextAttr (                                       // 封裝 ConsoleOut->SetAttribute()，方便統一改顏色
  IN UINTN Attribute                                // 輸入參數：文字屬性值，例如白字黑底、藍字黑底、黑字綠底
  )
{
  gST->ConOut->SetAttribute (gST->ConOut, Attribute); // 呼叫 UEFI Console Output Protocol 的 SetAttribute() 設定文字與背景顏色
}

STATIC
VOID
ResetDefaultTextAttr (                              // 將文字顏色恢復成預設狀態
  VOID
  )
{
  SetTextAttr (EFI_WHITE | EFI_BACKGROUND_BLACK);   // 設定成白字黑底；這是最常見、最穩定的預設顯示模式
}

STATIC
VOID
PrintLine (                                         // 輔助函式：印出一條分隔線，讓 UI 區塊更清楚
  VOID
  )
{
  Print (L"==================================================\n"); // 用 Print() 輸出一整行等號作為視覺分隔
}

STATIC
VOID
PrintHeader (                                       // 輔助函式：印出區塊標題（上下有分隔線）
  IN CONST CHAR16 *Title                            // 輸入參數：要顯示的標題文字
  )
{
  Print (L"\n");                                    // 先空一行，避免前一段內容和標題黏在一起
  PrintLine ();                                     // 印出上方分隔線
  Print (L"%s\n", Title);                           // 印出標題文字本身
  PrintLine ();                                     // 印出下方分隔線
}

STATIC
VOID
WaitAnyKey (                                        // 輔助函式：暫停畫面，等待使用者按任意鍵
  VOID
  )
{
  EFI_INPUT_KEY Key;                                // 用來接收鍵盤輸入資料；包含 ScanCode 與 UnicodeChar
  UINTN         Index;                              // WaitForEvent() 會回傳觸發的事件索引；此處只等一個事件，但仍需提供變數接收

  Print (L"\nPress any key to continue...");        // 提示使用者按任意鍵繼續
  gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &Index); // 等待鍵盤事件；1 表示只等一個事件，事件來源為 WaitForKey
  gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);     // 讀走那顆按下的鍵，避免殘留在輸入緩衝中
}

STATIC
EFI_STATUS
EFIAPI
DemoHelloImpl (                                     // 模擬 PPI 提供的 Hello 服務實作
  IN CONST CHAR16 *Message OPTIONAL                 // 可選輸入字串；模擬 consumer 傳訊息給 provider service
  )
{
  Print (L"[PPI Service] DemoHelloImpl() called\n"); // 顯示此 PPI service 已被呼叫

  if (Message != NULL) {                            // 如果呼叫者有傳入訊息字串
    Print (L"[PPI Service] Message = %s\n", Message); // 就把訊息內容印出，展示 consumer 傳參數給 provider 的效果
  }

  return EFI_SUCCESS;                               // 回傳 EFI_SUCCESS，表示服務執行成功
}

//
// ============================================================
// Draw menu
// - Title: blue text
// - Highlighted item: green background + black text
// ============================================================
//

STATIC
VOID
DrawMenu (                                          // 負責把目前的選單畫到螢幕上
  IN UINTN HighlightIndex                           // 輸入：目前高亮的選單索引；決定哪一列要用綠底黑字顯示
  )
{
  UINTN Index;                                      // 迴圈變數，用來走訪每一個選單項目

  Print (L"\n");                                    // 先空一行，使畫面比較不擁擠
  Print (L"==================================================\n"); // 印出上方分隔線

  //
  // Menu title in blue
  //
  SetTextAttr (EFI_BLUE | EFI_BACKGROUND_BLACK);    // 將接下來輸出的字改成藍字黑底
  Print (L"OneDemoApp Menu\n");                     // 印出主選單標題
  ResetDefaultTextAttr ();                          // 印完標題後立刻恢復白字黑底，避免影響後續文字

  Print (L"==================================================\n"); // 印出標題下方分隔線

  for (Index = 0; Index < MENU_ITEM_COUNT; Index++) { // 逐一輸出所有選單項目
    if (Index == HighlightIndex) {                  // 如果目前這一列就是被選中的項目
      SetTextAttr (EFI_BLACK | EFI_BACKGROUND_GREEN); // 設定成黑字綠底，表示目前選中
      Print (L"  %s\n", mMenuItems[Index]);         // 印出選單項目文字
      ResetDefaultTextAttr ();                      // 印完選中項後恢復預設白字黑底
    } else {                                        // 如果不是目前選中的項目
      Print (L"  %s\n", mMenuItems[Index]);         // 直接用預設白字黑底印出
    }
  }

  Print (L"==================================================\n"); // 印出選單區塊下方分隔線
  Print (L"Use Up/Down Arrow, press Enter to select.\n"); // 顯示操作說明
}

//
// ============================================================
// Read menu choice by arrow keys + Enter
// Less flicker version
// ============================================================
//

STATIC
EFI_STATUS
ReadMenuChoiceByArrow (                             // 讀取使用者用上下鍵 + Enter 所選的功能
  OUT UINTN *Choice                                 // 輸出參數：回傳實際選擇的功能編號（1~6 或 0）
  )
{
  EFI_STATUS    Status;                             // 接收 UEFI API 回傳狀態碼
  EFI_INPUT_KEY Key;                                // 接收鍵盤輸入資料
  UINTN         Highlight;                          // 目前高亮選中的項目索引
  BOOLEAN       NeedRedraw;                         // 是否需要重畫畫面；TRUE 表示有必要清畫面再畫 menu

  if (Choice == NULL) {                             // 防呆：如果呼叫者沒有提供有效的輸出指標
    return EFI_INVALID_PARAMETER;                   // 回傳 EFI_INVALID_PARAMETER，表示輸入參數無效
  }

  Highlight  = 0;                                   // 預設一開始選中第一個項目（索引 0）
  NeedRedraw = TRUE;                                // 第一次進入時一定要先畫出選單

  while (TRUE) {                                    // 無限迴圈，直到使用者按 Enter 做出選擇才 return
    if (NeedRedraw) {                               // 若目前需要重畫畫面
      gST->ConOut->ClearScreen (gST->ConOut);       // 清除目前螢幕內容；只在必要時才做，以降低閃爍
      ResetDefaultTextAttr ();                      // 確保清畫面後先回到白字黑底
      DrawMenu (Highlight);                         // 根據目前 Highlight 值重畫選單
      NeedRedraw = FALSE;                           // 畫完後先設成 FALSE；除非選中項改變才再重畫
    }

    //
    // Wait for key
    //
    while (TRUE) {                                  // 內層迴圈：持續等鍵盤輸入，直到真正有鍵進來
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key); // 嘗試從鍵盤讀一個 key stroke
      if (Status == EFI_NOT_READY) {                // EFI_NOT_READY 代表目前沒有按鍵可讀，不算錯誤
        continue;                                   // 沒鍵就繼續等
      }
      break;                                        // 一旦不是 EFI_NOT_READY，就跳出內層等待迴圈
    }

    if (EFI_ERROR (Status)) {                       // 如果 ReadKeyStroke() 回傳的是其他錯誤
      return Status;                                // 直接把錯誤狀態往上回傳，讓上層決定怎麼處理
    }

    //
    // Up arrow
    //
    if (Key.ScanCode == SCAN_UP) {                  // 若使用者按的是上方向鍵
      if (Highlight == 0) {                         // 如果目前已在第一項
        Highlight = MENU_ITEM_COUNT - 1;            // 就循環跳到最後一項，形成環狀選單
      } else {                                      // 如果不是第一項
        Highlight--;                                // 就把選中索引往上移一格
      }
      NeedRedraw = TRUE;                            // 選中項改變了，需要重畫選單
      continue;                                     // 回到外層 while，重畫新狀態
    }

    //
    // Down arrow
    //
    if (Key.ScanCode == SCAN_DOWN) {                // 若使用者按的是下方向鍵
      Highlight = (Highlight + 1) % MENU_ITEM_COUNT; // 往下移一格；若超過最後一項則回到第一項
      NeedRedraw = TRUE;                            // 選中項改變，需要重畫選單
      continue;                                     // 回到外層 while，重畫新狀態
    }

    //
    // Enter
    //
    if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {  // 若使用者按的是 Enter
      switch (Highlight) {                          // 根據目前高亮的索引決定要回傳哪個功能編號
      case 0:                                       // 第一列：「1. Create a new PPI in PPILIB」
        *Choice = 1;                                // 回傳功能編號 1
        return EFI_SUCCESS;                         // 成功結束此函式
      case 1:                                       // 第二列
        *Choice = 2;                                // 回傳功能編號 2
        return EFI_SUCCESS;
      case 2:                                       // 第三列
        *Choice = 3;                                // 回傳功能編號 3
        return EFI_SUCCESS;
      case 3:                                       // 第四列
        *Choice = 4;                                // 回傳功能編號 4
        return EFI_SUCCESS;
      case 4:                                       // 第五列
        *Choice = 5;                                // 回傳功能編號 5
        return EFI_SUCCESS;
      case 5:                                       // 第六列
        *Choice = 6;                                // 回傳功能編號 6
        return EFI_SUCCESS;
      case 6:                                       // 第七列
        *Choice = 0;                                // Exit 項目對外回傳 0
        return EFI_SUCCESS;
      default:                                      // 理論上不會走到這裡，因為 Highlight 一定在合法範圍內
        break;                                      // 保留 default 只是完整性考量
      }
    }
  }
}

//
// ============================================================
// 1. Create a new PPI in PPILIB
// ============================================================
//

STATIC
EFI_STATUS
CreatePpiInPpiLib (                                 // 功能 1：建立一個模擬 PPI 結構
  VOID
  )
{
  PrintHeader (L"1. Create a new PPI in PPILIB");   // 顯示功能標題

  mDemoPpi.Hello = DemoHelloImpl;                   // 把 PPI 的 Hello 函式指標指向真正的 service implementation
  mPpiCreated    = TRUE;                            // 記錄狀態：PPI 已建立完成

  Print (L"[Success] PPI structure created.\n");    // 顯示建立成功
  Print (L"PPI GUID          = %g\n", &gDemoPpiGuid); // 顯示此 PPI 的 GUID
  Print (L"Hello FunctionPtr = 0x%p\n", mDemoPpi.Hello); // 顯示函式指標位址，讓使用者看到 PPI 本質上是 function pointer 的集合

  return EFI_SUCCESS;                               // 回傳成功
}

//
// ============================================================
// 2. Create a new PEIM providing PPI services (simulated)
// ============================================================
//

STATIC
EFI_STATUS
ProvidePpiService (                                 // 功能 2：模擬 Provider PEIM 安裝 PPI
  VOID
  )
{
  PrintHeader (L"2. Create a new PEIM providing PPI services"); // 顯示功能標題

  if (!mPpiCreated) {                               // 如果 PPI 還沒先建立
    Print (L"[Error] PPI not created yet.\n");      // 顯示錯誤訊息
    Print (L"Please run Function 1 first.\n");      // 提醒使用者要先執行功能 1
    return EFI_NOT_READY;                           // 回傳 EFI_NOT_READY，表示當前狀態尚未準備好
  }

  mPpiInstalled = TRUE;                             // 記錄狀態：PPI 已經「模擬安裝」完成

  Print (L"[Success] Simulated InstallPpi().\n");   // 顯示模擬 InstallPpi 成功
  Print (L"PPI has been registered into local demo database.\n"); // 說明它已註冊到本地 demo database（這是展示概念，不是真正 PEI PPI DB）

  return EFI_SUCCESS;                               // 回傳成功
}

//
// ============================================================
// 3. Create another PEIM using PPI (simulated)
//    Also simulate BuildGuidHob()
// ============================================================
//

STATIC
EFI_STATUS
UsePpiService (                                     // 功能 3：模擬 Consumer PEIM LocatePpi 並使用服務，同時建立模擬 HOB
  VOID
  )
{
  EFI_STATUS Status;                                // 儲存呼叫 Hello service 的回傳狀態

  PrintHeader (L"3. Create another PEIM using PPI"); // 顯示功能標題

  if (!mPpiInstalled) {                             // 如果 Provider 尚未安裝 PPI
    Print (L"[Error] PPI service not installed yet.\n"); // 提示還不能使用
    Print (L"Please run Function 2 first.\n");      // 告知必須先安裝 PPI
    return EFI_NOT_READY;                           // 回傳尚未準備好
  }

  Print (L"[Success] Simulated LocatePpi().\n");    // 顯示模擬 LocatePpi 成功；代表 consumer 已經找到 provider 的 service

  if (mDemoPpi.Hello == NULL) {                     // 如果 Hello 函式指標是空的
    Print (L"[Error] PPI Hello() function is NULL.\n"); // 顯示 service 不可用
    return EFI_NOT_FOUND;                           // 回傳 EFI_NOT_FOUND，表示找不到可用的 service 入口
  }

  Status = mDemoPpi.Hello (L"Hello from simulated Consumer PEIM"); // 呼叫 provider 的 Hello service，模擬 consumer 使用 PPI
  Print (L"Hello() returned = %r\n", Status);       // 印出 service 回傳的 EFI_STATUS

  //
  // Simulate HOB creation
  //
  mDemoHob.Signature = SIGNATURE_32 ('D', 'E', 'M', 'O'); // 用 SIGNATURE_32 建立 4-byte 簽章 'DEMO'，方便辨識資料格式
  mDemoHob.Value     = 0x55AA1234;                  // 填入示範資料值
  mDemoHob.Flags     = 0x00000001;                  // 填入示範旗標值
  mHobBuilt          = TRUE;                        // 記錄狀態：模擬 HOB 已建立完成

  Print (L"[Success] Simulated BuildGuidHob().\n"); // 顯示模擬 BuildGuidHob 成功
  Print (L"HOB GUID      = %g\n", &gDemoHobGuid);   // 顯示此 HOB 的 GUID
  Print (L"HOB Signature = 0x%08x\n", mDemoHob.Signature); // 顯示 HOB 內的 Signature
  Print (L"HOB Value     = 0x%08x\n", mDemoHob.Value); // 顯示 HOB 內的 Value
  Print (L"HOB Flags     = 0x%08x\n", mDemoHob.Flags); // 顯示 HOB 內的 Flags

  return EFI_SUCCESS;                               // 回傳成功
}

//
// ============================================================
// 4. Write an APP for set variable (real)
// ============================================================
//

STATIC
EFI_STATUS
SetDemoVariable (                                   // 功能 4：真實呼叫 UEFI Runtime Service 的 SetVariable()
  VOID
  )
{
  EFI_STATUS Status;                                // 儲存 SetVariable() 的回傳狀態
  UINT32     Data;                                  // 要寫進 variable 的 32-bit 資料

  PrintHeader (L"4. Write an APP for set variable"); // 顯示功能標題

  Data = 0x12345678;                                // 設定示範用資料值

  Status = gRT->SetVariable (                       // 呼叫 Runtime Service：SetVariable()
                  L"MyPeiVar",                      // Variable Name：變數名稱
                  &gDemoVarGuid,                    // Vendor GUID：變數所屬的 GUID namespace
                  EFI_VARIABLE_NON_VOLATILE |       // 屬性 1：Non-Volatile，代表重開機後仍保留
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | // 屬性 2：Boot Service Access，代表 Boot Services 階段可讀寫
                  EFI_VARIABLE_RUNTIME_ACCESS,      // 屬性 3：Runtime Access，代表 Runtime 階段也可存取
                  sizeof (Data),                    // DataSize：資料大小，這裡是 4 bytes
                  &Data                             // Data：要寫入的實際資料位址
                  );

  Print (L"SetVariable Status = %r\n", Status);     // 顯示 SetVariable() 回傳狀態；Success 表示寫入成功
  Print (L"Variable Name      = MyPeiVar\n");       // 顯示變數名稱
  Print (L"Variable GUID      = %g\n", &gDemoVarGuid); // 顯示變數 GUID
  Print (L"Variable Data      = 0x%08x\n", Data);   // 顯示寫入的資料值

  return Status;                                    // 把真實 SetVariable() 的執行結果回傳給上層
}

//
// ============================================================
// 5. Write an APP for read HOB (simulated)
// ============================================================
//

STATIC
EFI_STATUS
ReadDemoHob (                                       // 功能 5：讀取模擬 HOB
  VOID
  )
{
  PrintHeader (L"5. Write an APP for read HOB");    // 顯示功能標題

  if (!mHobBuilt) {                                 // 如果目前尚未建立 HOB
    Print (L"GUID HOB not found.\n");               // 顯示找不到 HOB
    Print (L"[Note] In this single-EFI demo, HOB is simulated in app memory.\n"); // 說明這裡不是讀真實 PEI HOB，而是讀本程式內部模擬資料
    Print (L"Please run Function 3 first, or use Run All.\n"); // 提示使用者要先讓 consumer 觸發建立 HOB
    return EFI_NOT_FOUND;                           // 回傳 EFI_NOT_FOUND，表示目前沒有可讀的 HOB
  }

  Print (L"GUID HOB found.\n");                     // 若已建好 HOB，顯示找到
  Print (L"HOB GUID      = %g\n", &gDemoHobGuid);   // 顯示 HOB GUID
  Print (L"Signature     = 0x%08x\n", mDemoHob.Signature); // 顯示 HOB 內容的 Signature
  Print (L"Value         = 0x%08x\n", mDemoHob.Value); // 顯示 HOB 內容的 Value
  Print (L"Flags         = 0x%08x\n", mDemoHob.Flags); // 顯示 HOB 內容的 Flags

  return EFI_SUCCESS;                               // 回傳成功
}

//
// ============================================================
// 6. Run all
// ============================================================
//

STATIC
EFI_STATUS
RunAll (                                            // 功能 6：依序把 1~5 全部跑一遍
  VOID
  )
{
  EFI_STATUS Status;                                // 用來保存每一步的回傳狀態；一旦有錯就中止後續動作

  PrintHeader (L"6. Run All");                      // 顯示功能標題

  Status = CreatePpiInPpiLib ();                    // 先建立 PPI 結構
  if (EFI_ERROR (Status)) {                         // 若失敗
    return Status;                                  // 直接回傳錯誤，不再繼續
  }

  Status = ProvidePpiService ();                    // 再模擬 provider 安裝 PPI
  if (EFI_ERROR (Status)) {                         // 若失敗
    return Status;                                  // 直接回傳錯誤
  }

  Status = UsePpiService ();                        // 再模擬 consumer LocatePpi + 呼叫 service + 建立 HOB
  if (EFI_ERROR (Status)) {                         // 若失敗
    return Status;                                  // 直接回傳錯誤
  }

  Status = SetDemoVariable ();                      // 呼叫真實 SetVariable()
  if (EFI_ERROR (Status)) {                         // 若失敗
    return Status;                                  // 直接回傳錯誤
  }

  Status = ReadDemoHob ();                          // 最後讀取模擬 HOB
  return Status;                                    // 回傳最後一步的狀態
}

//
// ============================================================
// Main
// ============================================================
//

EFI_STATUS
EFIAPI
UefiMain (                                          // UEFI Application 入口函式；當 Shell 執行此 EFI 時會從這裡開始
  IN EFI_HANDLE        ImageHandle,                 // ImageHandle：目前執行映像的 Handle；本程式未直接使用，但 UEFI app 標準入口需保留
  IN EFI_SYSTEM_TABLE  *SystemTable                 // SystemTable：UEFI 系統表指標；本程式主要透過 gST / gBS / gRT 全域變數使用
  )
{
  EFI_STATUS Status;                                // 保存子函式回傳值
  UINTN      Choice;                                // 保存使用者在選單中選擇的功能編號
  BOOLEAN    ExitLoop;                              // 控制主選單是否離開；FALSE 表示繼續，TRUE 表示結束

  ExitLoop = FALSE;                                 // 一開始先設定成不離開

  ResetDefaultTextAttr ();                          // 先把文字屬性恢復成白字黑底
  gST->ConOut->ClearScreen (gST->ConOut);           // 程式一啟動先清一次螢幕，避免 Shell 舊內容殘留

  while (!ExitLoop) {                               // 只要還沒要求離開，就持續顯示選單
    Status = ReadMenuChoiceByArrow (&Choice);       // 讓使用者用上下鍵 + Enter 選擇功能
    if (EFI_ERROR (Status)) {                       // 如果讀取選單輸入失敗
      Print (L"ReadMenuChoiceByArrow failed: %r\n", Status); // 印出錯誤資訊
      return Status;                                // 直接結束程式並回傳錯誤
    }

    gST->ConOut->ClearScreen (gST->ConOut);         // 使用者選完後，先清除選單畫面
    ResetDefaultTextAttr ();                        // 清畫面後重新恢復預設文字顏色

    switch (Choice) {                               // 根據使用者選的功能編號，呼叫對應函式
    case 1:                                         // 選擇功能 1
      CreatePpiInPpiLib ();                         // 執行建立 PPI 結構
      WaitAnyKey ();                                // 顯示結果後等待使用者按鍵
      break;                                        // 回到主選單

    case 2:                                         // 選擇功能 2
      ProvidePpiService ();                         // 執行模擬 InstallPpi
      WaitAnyKey ();                                // 等待使用者按鍵
      break;                                        // 回到主選單

    case 3:                                         // 選擇功能 3
      UsePpiService ();                             // 執行模擬 LocatePpi + Hello + BuildGuidHob
      WaitAnyKey ();                                // 等待使用者按鍵
      break;                                        // 回到主選單

    case 4:                                         // 選擇功能 4
      SetDemoVariable ();                           // 執行真實 SetVariable()
      WaitAnyKey ();                                // 等待使用者按鍵
      break;                                        // 回到主選單

    case 5:                                         // 選擇功能 5
      ReadDemoHob ();                               // 執行讀取模擬 HOB
      WaitAnyKey ();                                // 等待使用者按鍵
      break;                                        // 回到主選單

    case 6:                                         // 選擇功能 6
      RunAll ();                                    // 一次執行全部功能
      WaitAnyKey ();                                // 等待使用者按鍵
      break;                                        // 回到主選單

    case 0:                                         // 選擇 Exit
      ExitLoop = TRUE;                              // 設成 TRUE，下一輪 while 條件不成立就會離開
      break;                                        // 跳出 switch

    default:                                        // 理論上不應進入，因為 Choice 由選單函式控制在合法範圍
      Print (L"Invalid selection.\n");              // 若發生非預期值，顯示無效選擇
      WaitAnyKey ();                                // 等待使用者按鍵
      break;                                        // 回到主選單
    }
  }

  gST->ConOut->ClearScreen (gST->ConOut);           // 離開前清畫面，讓結束畫面更乾淨
  ResetDefaultTextAttr ();                          // 確保離開時恢復預設色彩
  Print (L"Exit OneDemoApp.\n");                    // 顯示程式結束訊息

  return EFI_SUCCESS;                               // 正常結束，回傳 EFI_SUCCESS
}