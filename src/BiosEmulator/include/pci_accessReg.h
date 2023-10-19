#pragma once
/* Function codes to pass to PCI_accessReg */

#define PCI_READ_BYTE               0
#define PCI_READ_WORD               1
#define PCI_READ_DWORD              2
#define PCI_WRITE_BYTE              3
#define PCI_WRITE_WORD              4
#define PCI_WRITE_DWORD             5

/* Function to access PCI configuration registers */

#include "x86emu/types.h"
#include "pciinfo.h"

struct cJSON;

unsigned char* buildConfigFromJsonAndRom(const struct cJSON* json, uintptr_t rom);
ulong PCI_accessReg(int index, ulong value, int func, PCIDeviceInfo *info);
