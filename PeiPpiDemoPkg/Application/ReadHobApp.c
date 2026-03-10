#include <Uefi.h>
#include <PiPei.h>

#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/HobLib.h>

#include <Ppi/MyPeiDemoPpi.h>

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  VOID        *GuidHob;
  MY_HOB_DATA *Data;

  Print (L"========================================\n");
  Print (L" ReadHobApp Start\n");
  Print (L"========================================\n");

  GuidHob = GetFirstGuidHob (&gMyGuidHobGuid);
  if (GuidHob == NULL) {
    Print (L"GUID HOB not found\n");
    return EFI_NOT_FOUND;
  }

  Data = (MY_HOB_DATA *)GET_GUID_HOB_DATA (GuidHob);

  Print (L"GUID HOB found\n");
  Print (L"Signature = 0x%08x\n", Data->Signature);
  Print (L"Value     = 0x%08x\n", Data->Value);

  return EFI_SUCCESS;
}