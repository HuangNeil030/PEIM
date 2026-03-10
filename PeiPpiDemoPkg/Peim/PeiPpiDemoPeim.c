#include <Base.h>
#include <Uefi.h>
#include <PiPei.h>
#include <Pi/PiHob.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>

#include <Ppi/ReadOnlyVariable2.h>
#include <Ppi/MyPeiDemoPpi.h>

EFI_GUID gMyPeiDemoPpiGuid = MY_PEI_DEMO_PPI_GUID;
EFI_GUID gMyPeiVarGuid     = MY_PEI_VAR_GUID;
EFI_GUID gMyGuidHobGuid    = MY_GUID_HOB_GUID;

EFI_STATUS
EFIAPI
MyPeiHelloImpl (
  IN CONST CHAR16 *Message OPTIONAL
  )
{
  DEBUG ((DEBUG_INFO, "[PeiPpiDemo] MyPeiHelloImpl() enter\n"));

  if (Message == NULL) {
    DEBUG ((DEBUG_INFO, "[PeiPpiDemo] Message = NULL\n"));
  } else {
    DEBUG ((DEBUG_INFO, "[PeiPpiDemo] Message pointer valid\n"));
  }

  DEBUG ((DEBUG_INFO, "[PeiPpiDemo] MyPeiHelloImpl() return EFI_SUCCESS\n"));
  return EFI_SUCCESS;
}

STATIC MY_PEI_DEMO_PPI mMyPeiDemoPpi = {
  MyPeiHelloImpl
};

STATIC EFI_PEI_PPI_DESCRIPTOR mMyPeiDemoPpiDescriptor = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gMyPeiDemoPpiGuid,
  &mMyPeiDemoPpi
};

STATIC
EFI_STATUS
EFIAPI
InstallMyPpi (
  IN EFI_PEI_FILE_HANDLE    FileHandle,
  IN CONST EFI_PEI_SERVICES **PeiServices
  )
{
  EFI_STATUS Status;

  Status = PeiServicesInstallPpi (&mMyPeiDemoPpiDescriptor);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[PeiPpiDemo] InstallPpi failed: %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "[PeiPpiDemo] InstallPpi success\n"));
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
ConsumeMyPpi (
  IN CONST EFI_PEI_SERVICES **PeiServices
  )
{
  EFI_STATUS              Status;
  EFI_PEI_PPI_DESCRIPTOR  *Descriptor;
  MY_PEI_DEMO_PPI         *MyPpi;

  Descriptor = NULL;
  MyPpi      = NULL;

  Status = PeiServicesLocatePpi (
             &gMyPeiDemoPpiGuid,
             0,
             &Descriptor,
             (VOID **)&MyPpi
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[PeiPpiDemo] LocatePpi failed: %r\n", Status));
    return Status;
  }

  if ((MyPpi == NULL) || (MyPpi->Hello == NULL)) {
    DEBUG ((DEBUG_ERROR, "[PeiPpiDemo] Invalid PPI interface\n"));
    return EFI_NOT_FOUND;
  }

  Status = MyPpi->Hello (L"Hello from PEI consumer");
  DEBUG ((DEBUG_INFO, "[PeiPpiDemo] MyPpi->Hello() returned: %r\n", Status));

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
ReadMyVariable (
  IN EFI_PEI_FILE_HANDLE    FileHandle,
  IN CONST EFI_PEI_SERVICES **PeiServices
  )
{
  EFI_STATUS                       Status;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI *VariablePpi;
  UINT32                           Attributes;
  UINTN                            DataSize;
  UINT32                           Data;

  VariablePpi = NULL;
  Attributes  = 0;
  DataSize    = sizeof (Data);
  Data        = 0;

  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
             (VOID **)&VariablePpi
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "[PeiPpiDemo] Locate ReadOnlyVariable2 PPI failed: %r\n", Status));
    return Status;
  }

  Status = VariablePpi->GetVariable (
                          VariablePpi,
                          L"MyPeiVar",
                          &gMyPeiVarGuid,
                          &Attributes,
                          &DataSize,
                          &Data
                          );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "[PeiPpiDemo] GetVariable failed: %r\n", Status));
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "[PeiPpiDemo] GetVariable success: Data=0x%x Attr=0x%x Size=%u\n",
    Data,
    Attributes,
    (UINT32)DataSize
    ));

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
BuildMyGuidHob (
  IN EFI_PEI_FILE_HANDLE    FileHandle,
  IN CONST EFI_PEI_SERVICES **PeiServices
  )
{
  MY_HOB_DATA *HobData;

  HobData = BuildGuidHob (&gMyGuidHobGuid, sizeof (MY_HOB_DATA));
  if (HobData == NULL) {
    DEBUG ((DEBUG_ERROR, "[PeiPpiDemo] BuildGuidHob failed\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  HobData->Signature = SIGNATURE_32 ('D', 'E', 'M', 'O');
  HobData->Value     = 0x55AA1234;

  DEBUG ((DEBUG_INFO, "[PeiPpiDemo] GUID HOB created\n"));
  DEBUG ((DEBUG_INFO, "[PeiPpiDemo] Signature = 0x%x\n", HobData->Signature));
  DEBUG ((DEBUG_INFO, "[PeiPpiDemo] Value     = 0x%x\n", HobData->Value));

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PeiPpiDemoPeimEntry (
  IN EFI_PEI_FILE_HANDLE    FileHandle,
  IN CONST EFI_PEI_SERVICES **PeiServices
  )
{
  EFI_STATUS Status;

  DEBUG ((DEBUG_INFO, "========================================\n"));
  DEBUG ((DEBUG_INFO, "[PeiPpiDemo] PEIM Entry Start\n"));
  DEBUG ((DEBUG_INFO, "========================================\n"));

  Status = InstallMyPpi (FileHandle, PeiServices);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ConsumeMyPpi (PeiServices);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "[PeiPpiDemo] ConsumeMyPpi failed: %r\n", Status));
  }

  Status = ReadMyVariable (FileHandle, PeiServices);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "[PeiPpiDemo] ReadMyVariable failed: %r\n", Status));
  }

  Status = BuildMyGuidHob (FileHandle, PeiServices);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "[PeiPpiDemo] PEIM Entry Done\n"));
  DEBUG ((DEBUG_INFO, "========================================\n"));

  return EFI_SUCCESS;
}