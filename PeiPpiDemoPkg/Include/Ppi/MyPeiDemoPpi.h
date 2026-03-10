#ifndef _MY_PEI_DEMO_PPI_H_
#define _MY_PEI_DEMO_PPI_H_

#include <Uefi.h>
#include <PiPei.h>

//
// 自訂 PPI GUID
//
#define MY_PEI_DEMO_PPI_GUID \
  { 0x12345678, 0x9abc, 0x4def, { 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11 } }

extern EFI_GUID gMyPeiDemoPpiGuid;

//
// 自訂 Variable GUID
//
#define MY_PEI_VAR_GUID \
  { 0xa1b2c3d4, 0x1111, 0x2222, { 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa } }

extern EFI_GUID gMyPeiVarGuid;

//
// 自訂 GUID HOB GUID
//
#define MY_GUID_HOB_GUID \
  { 0xdeadbeef, 0xaaaa, 0xbbbb, { 0xcc, 0xdd, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 } }

extern EFI_GUID gMyGuidHobGuid;

typedef
EFI_STATUS
(EFIAPI *MY_PEI_HELLO)(
  IN CONST CHAR16 *Message OPTIONAL
  );

typedef struct {
  MY_PEI_HELLO  Hello;
} MY_PEI_DEMO_PPI;

typedef struct {
  UINT32 Signature;
  UINT32 Value;
} MY_HOB_DATA;

#endif