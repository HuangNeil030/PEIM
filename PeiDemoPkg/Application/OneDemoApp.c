#include <Uefi.h>

#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

//
// ============================================================
// Demo GUIDs
// ============================================================
//

STATIC EFI_GUID gDemoPpiGuid =
  { 0x12345678, 0x9abc, 0x4def, { 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11 } };

STATIC EFI_GUID gDemoVarGuid =
  { 0xa1b2c3d4, 0x1111, 0x2222, { 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa } };

STATIC EFI_GUID gDemoHobGuid =
  { 0xdeadbeef, 0xaaaa, 0xbbbb, { 0xcc, 0xdd, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 } };

//
// ============================================================
// Simulated PPI definition
// ============================================================
//

typedef
EFI_STATUS
(EFIAPI *DEMO_PPI_HELLO)(
  IN CONST CHAR16 *Message OPTIONAL
  );

typedef struct {
  DEMO_PPI_HELLO  Hello;
} DEMO_PPI;

//
// ============================================================
// Simulated HOB payload
// ============================================================
//

typedef struct {
  UINT32 Signature;
  UINT32 Value;
  UINT32 Flags;
} DEMO_HOB_DATA;

//
// ============================================================
// Global demo state
// ============================================================
//

STATIC BOOLEAN       mPpiCreated   = FALSE;
STATIC BOOLEAN       mPpiInstalled = FALSE;
STATIC BOOLEAN       mHobBuilt     = FALSE;
STATIC DEMO_PPI      mDemoPpi;
STATIC DEMO_HOB_DATA mDemoHob;

//
// ============================================================
// Menu strings
// ============================================================
//

#define MENU_ITEM_COUNT  7

STATIC CONST CHAR16 *mMenuItems[MENU_ITEM_COUNT] = {
  L"1. Create a new PPI in PPILIB",
  L"2. Create a new PEIM providing PPI services",
  L"3. Create another PEIM using PPI",
  L"4. Write an APP for set variable",
  L"5. Write an APP for read HOB",
  L"6. Run All",
  L"0. Exit"
};

//
// ============================================================
// Helpers
// ============================================================
//

STATIC
VOID
SetTextAttr (
  IN UINTN Attribute
  )
{
  gST->ConOut->SetAttribute (gST->ConOut, Attribute);
}

STATIC
VOID
ResetDefaultTextAttr (
  VOID
  )
{
  SetTextAttr (EFI_WHITE | EFI_BACKGROUND_BLACK);
}

STATIC
VOID
PrintLine (
  VOID
  )
{
  Print (L"==================================================\n");
}

STATIC
VOID
PrintHeader (
  IN CONST CHAR16 *Title
  )
{
  Print (L"\n");
  PrintLine ();
  Print (L"%s\n", Title);
  PrintLine ();
}

STATIC
VOID
WaitAnyKey (
  VOID
  )
{
  EFI_INPUT_KEY Key;
  UINTN         Index;

  Print (L"\nPress any key to continue...");
  gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &Index);
  gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
}

STATIC
EFI_STATUS
EFIAPI
DemoHelloImpl (
  IN CONST CHAR16 *Message OPTIONAL
  )
{
  Print (L"[PPI Service] DemoHelloImpl() called\n");

  if (Message != NULL) {
    Print (L"[PPI Service] Message = %s\n", Message);
  }

  return EFI_SUCCESS;
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
DrawMenu (
  IN UINTN HighlightIndex
  )
{
  UINTN Index;

  Print (L"\n");
  Print (L"==================================================\n");

  //
  // Menu title in blue
  //
  SetTextAttr (EFI_BLUE | EFI_BACKGROUND_BLACK);
  Print (L"OneDemoApp Menu\n");

  //
  // Restore normal color
  //
  ResetDefaultTextAttr ();

  Print (L"==================================================\n");

  for (Index = 0; Index < MENU_ITEM_COUNT; Index++) {
    if (Index == HighlightIndex) {
      //
      // Selected item: green background + black text
      //
      SetTextAttr (EFI_BLACK | EFI_BACKGROUND_GREEN);
      Print (L"  %s\n", mMenuItems[Index]);
      ResetDefaultTextAttr ();
    } else {
      Print (L"  %s\n", mMenuItems[Index]);
    }
  }

  Print (L"==================================================\n");
  Print (L"Use Up/Down Arrow, press Enter to select.\n");
}

//
// ============================================================
// Read menu choice by arrow keys + Enter
// Less flicker version
// ============================================================
//

STATIC
EFI_STATUS
ReadMenuChoiceByArrow (
  OUT UINTN *Choice
  )
{
  EFI_STATUS    Status;
  EFI_INPUT_KEY Key;
  UINTN         Highlight;
  BOOLEAN       NeedRedraw;

  if (Choice == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Highlight  = 0;
  NeedRedraw = TRUE;

  while (TRUE) {
    if (NeedRedraw) {
      gST->ConOut->ClearScreen (gST->ConOut);
      ResetDefaultTextAttr ();
      DrawMenu (Highlight);
      NeedRedraw = FALSE;
    }

    //
    // Wait for key
    //
    while (TRUE) {
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
      if (Status == EFI_NOT_READY) {
        continue;
      }
      break;
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Up arrow
    //
    if (Key.ScanCode == SCAN_UP) {
      if (Highlight == 0) {
        Highlight = MENU_ITEM_COUNT - 1;
      } else {
        Highlight--;
      }
      NeedRedraw = TRUE;
      continue;
    }

    //
    // Down arrow
    //
    if (Key.ScanCode == SCAN_DOWN) {
      Highlight = (Highlight + 1) % MENU_ITEM_COUNT;
      NeedRedraw = TRUE;
      continue;
    }

    //
    // Enter
    //
    if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
      switch (Highlight) {
      case 0:
        *Choice = 1;
        return EFI_SUCCESS;
      case 1:
        *Choice = 2;
        return EFI_SUCCESS;
      case 2:
        *Choice = 3;
        return EFI_SUCCESS;
      case 3:
        *Choice = 4;
        return EFI_SUCCESS;
      case 4:
        *Choice = 5;
        return EFI_SUCCESS;
      case 5:
        *Choice = 6;
        return EFI_SUCCESS;
      case 6:
        *Choice = 0;
        return EFI_SUCCESS;
      default:
        break;
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
CreatePpiInPpiLib (
  VOID
  )
{
  PrintHeader (L"1. Create a new PPI in PPILIB");

  mDemoPpi.Hello = DemoHelloImpl;
  mPpiCreated    = TRUE;

  Print (L"[Success] PPI structure created.\n");
  Print (L"PPI GUID          = %g\n", &gDemoPpiGuid);
  Print (L"Hello FunctionPtr = 0x%p\n", mDemoPpi.Hello);

  return EFI_SUCCESS;
}

//
// ============================================================
// 2. Create a new PEIM providing PPI services (simulated)
// ============================================================
//

STATIC
EFI_STATUS
ProvidePpiService (
  VOID
  )
{
  PrintHeader (L"2. Create a new PEIM providing PPI services");

  if (!mPpiCreated) {
    Print (L"[Error] PPI not created yet.\n");
    Print (L"Please run Function 1 first.\n");
    return EFI_NOT_READY;
  }

  mPpiInstalled = TRUE;

  Print (L"[Success] Simulated InstallPpi().\n");
  Print (L"PPI has been registered into local demo database.\n");

  return EFI_SUCCESS;
}

//
// ============================================================
// 3. Create another PEIM using PPI (simulated)
//    Also simulate BuildGuidHob()
// ============================================================
//

STATIC
EFI_STATUS
UsePpiService (
  VOID
  )
{
  EFI_STATUS Status;

  PrintHeader (L"3. Create another PEIM using PPI");

  if (!mPpiInstalled) {
    Print (L"[Error] PPI service not installed yet.\n");
    Print (L"Please run Function 2 first.\n");
    return EFI_NOT_READY;
  }

  Print (L"[Success] Simulated LocatePpi().\n");

  if (mDemoPpi.Hello == NULL) {
    Print (L"[Error] PPI Hello() function is NULL.\n");
    return EFI_NOT_FOUND;
  }

  Status = mDemoPpi.Hello (L"Hello from simulated Consumer PEIM");
  Print (L"Hello() returned = %r\n", Status);

  //
  // Simulate HOB creation
  //
  mDemoHob.Signature = SIGNATURE_32 ('D', 'E', 'M', 'O');
  mDemoHob.Value     = 0x55AA1234;
  mDemoHob.Flags     = 0x00000001;
  mHobBuilt          = TRUE;

  Print (L"[Success] Simulated BuildGuidHob().\n");
  Print (L"HOB GUID      = %g\n", &gDemoHobGuid);
  Print (L"HOB Signature = 0x%08x\n", mDemoHob.Signature);
  Print (L"HOB Value     = 0x%08x\n", mDemoHob.Value);
  Print (L"HOB Flags     = 0x%08x\n", mDemoHob.Flags);

  return EFI_SUCCESS;
}

//
// ============================================================
// 4. Write an APP for set variable (real)
// ============================================================
//

STATIC
EFI_STATUS
SetDemoVariable (
  VOID
  )
{
  EFI_STATUS Status;
  UINT32     Data;

  PrintHeader (L"4. Write an APP for set variable");

  Data = 0x12345678;

  Status = gRT->SetVariable (
                  L"MyPeiVar",
                  &gDemoVarGuid,
                  EFI_VARIABLE_NON_VOLATILE |
                  EFI_VARIABLE_BOOTSERVICE_ACCESS |
                  EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof (Data),
                  &Data
                  );

  Print (L"SetVariable Status = %r\n", Status);
  Print (L"Variable Name      = MyPeiVar\n");
  Print (L"Variable GUID      = %g\n", &gDemoVarGuid);
  Print (L"Variable Data      = 0x%08x\n", Data);

  return Status;
}

//
// ============================================================
// 5. Write an APP for read HOB (simulated)
// ============================================================
//

STATIC
EFI_STATUS
ReadDemoHob (
  VOID
  )
{
  PrintHeader (L"5. Write an APP for read HOB");

  if (!mHobBuilt) {
    Print (L"GUID HOB not found.\n");
    Print (L"[Note] In this single-EFI demo, HOB is simulated in app memory.\n");
    Print (L"Please run Function 3 first, or use Run All.\n");
    return EFI_NOT_FOUND;
  }

  Print (L"GUID HOB found.\n");
  Print (L"HOB GUID      = %g\n", &gDemoHobGuid);
  Print (L"Signature     = 0x%08x\n", mDemoHob.Signature);
  Print (L"Value         = 0x%08x\n", mDemoHob.Value);
  Print (L"Flags         = 0x%08x\n", mDemoHob.Flags);

  return EFI_SUCCESS;
}

//
// ============================================================
// 6. Run all
// ============================================================
//

STATIC
EFI_STATUS
RunAll (
  VOID
  )
{
  EFI_STATUS Status;

  PrintHeader (L"6. Run All");

  Status = CreatePpiInPpiLib ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ProvidePpiService ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = UsePpiService ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SetDemoVariable ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ReadDemoHob ();
  return Status;
}

//
// ============================================================
// Main
// ============================================================
//

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status;
  UINTN      Choice;
  BOOLEAN    ExitLoop;

  ExitLoop = FALSE;

  ResetDefaultTextAttr ();
  gST->ConOut->ClearScreen (gST->ConOut);

  while (!ExitLoop) {
    Status = ReadMenuChoiceByArrow (&Choice);
    if (EFI_ERROR (Status)) {
      Print (L"ReadMenuChoiceByArrow failed: %r\n", Status);
      return Status;
    }

    gST->ConOut->ClearScreen (gST->ConOut);
    ResetDefaultTextAttr ();

    switch (Choice) {
    case 1:
      CreatePpiInPpiLib ();
      WaitAnyKey ();
      break;

    case 2:
      ProvidePpiService ();
      WaitAnyKey ();
      break;

    case 3:
      UsePpiService ();
      WaitAnyKey ();
      break;

    case 4:
      SetDemoVariable ();
      WaitAnyKey ();
      break;

    case 5:
      ReadDemoHob ();
      WaitAnyKey ();
      break;

    case 6:
      RunAll ();
      WaitAnyKey ();
      break;

    case 0:
      ExitLoop = TRUE;
      break;

    default:
      Print (L"Invalid selection.\n");
      WaitAnyKey ();
      break;
    }
  }

  gST->ConOut->ClearScreen (gST->ConOut);
  ResetDefaultTextAttr ();
  Print (L"Exit OneDemoApp.\n");

  return EFI_SUCCESS;
}