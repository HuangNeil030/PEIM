#include <Uefi.h>

#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Ppi/MyPeiDemoPpi.h>

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS Status;
  UINT32     Data;

  Data = 0x12345678;

  Print (L"========================================\n");
  Print (L" SetVariableApp Start\n");
  Print (L"========================================\n");

  Status = gRT->SetVariable (
                  L"MyPeiVar",
                  &gMyPeiVarGuid,
                  EFI_VARIABLE_NON_VOLATILE |
                  EFI_VARIABLE_BOOTSERVICE_ACCESS |
                  EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof (Data),
                  &Data
                  );

  Print (L"SetVariable Status = %r\n", Status);
  Print (L"Variable Name      = MyPeiVar\n");
  Print (L"Variable GUID      = %g\n", &gMyPeiVarGuid);
  Print (L"Variable Data      = 0x%08x\n", Data);

  return Status;
}