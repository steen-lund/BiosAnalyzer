#include "include/pci_accessReg.h"

#include "../cJSON.h"
#include "../MemAllocator.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned char pci_config[256];
typedef struct barInfo
{
	uint32_t size;
	uintptr_t address;
	unsigned char restoreAddress; // if true, then we need to restore the address from the cache after the user requested the size
} barInfo;

static barInfo barInfoCache[6];

#define VENDOR_ID_OFFSET 0x00
#define DEVICE_ID_OFFSET 0x02
#define COMMAND_OFFSET 0x04
#define STATUS_OFFSET 0x06
#define REVISION_ID_OFFSET 0x08
#define PROG_IF_OFFSET 0x09
#define SUBCLASS_OFFSET 0x0a
#define CLASS_OFFSET 0x0b
#define HEADER_TYPE_OFFSET 0x0e
#define BAR0_OFFSET 0x10
#define BAR1_OFFSET 0x14
#define BAR2_OFFSET 0x18
#define BAR3_OFFSET 0x1c
#define BAR4_OFFSET 0x20
#define BAR5_OFFSET 0x24
#define EXPANSION_ROM_OFFSET 0x30

static unsigned char jsongStringItemToUnsignedByte(const cJSON* json)
{
	const char* value = cJSON_GetStringValue(json);
	if (value == NULL)
		return 0;
	return (unsigned char)strtol(value, NULL, 16);
}

static unsigned short jsonStringItemToShort(const cJSON* json)
{
	const char* value = cJSON_GetStringValue(json);
	if (value == NULL)
		return 0;
	return (unsigned short)strtol(value, NULL, 16);
}

static unsigned int jsonStringItemToUnsignedInteger(const cJSON* json)
{
	const char* value = cJSON_GetStringValue(json);
	if (value == NULL)
		return 0;
	return (unsigned int)strtol(value, NULL, 16);
}

void setUnsignedShortInConfig(unsigned char* config, int offset, unsigned short value)
{
	uint16_t* address = (uint16_t*)(config + offset);
	*address = value;
}

void setUnsignedIntInConfig(unsigned char* config, int offset, unsigned int value)
{
	uint32_t* address = (uint32_t*)(config + offset);
	*address = value;
}

unsigned char* buildConfigFromJsonAndRom(const cJSON* json, uintptr_t rom)
{
	uint32_t romAddress = (uint32_t)rom;
	if (romAddress != rom)
	{
		printf("Bad rom allocation\nIt has be within the first 4GB of memory\n");
		return NULL;
	}

	memset(pci_config, 0, 256);
	memset(barInfoCache, 0, sizeof(barInfoCache));

	// Read the various fields from the json file
	// and write them to the pci_config array
	// making sure to get the right size and byte order for each field
	// (see https://wiki.osdev.org/PCI#Configuration_Space_Access_Mechanism_.231)

	// The json file is expected to have the following fields:
	// "vendor_id" (2 bytes)
	// "device_id" (2 bytes)
	// "command" (2 bytes)
	// "status" (2 bytes)
	// "revision_id" (1 byte)
	// "prog_if" (1 byte)
	// "subclass" (1 byte)
	// "class" (1 byte)
	// "header_type" (1 byte)
	// "bar0size" (4 bytes)   // This is not the address, but the size. We'll use the size to allocate memory for the bar
	// "bar1size" (4 bytes)	  // and we will cache the size elsewhere for when the user request the size of the bar
	// "bar2size" (4 bytes)	  // by writing 0xffffffff to the bar.
	// "bar3size" (4 bytes)   // Then we will return the cached size and reset the bar to the allocated memory address
	// "bar4size" (4 bytes)
	// "bar5size" (4 bytes)
	unsigned short vendor_id = jsonStringItemToShort(cJSON_GetObjectItem(json, "vendor_id"));
	setUnsignedShortInConfig(pci_config, VENDOR_ID_OFFSET, vendor_id);

	unsigned short device_id = jsonStringItemToShort(cJSON_GetObjectItem(json, "device_id"));
	setUnsignedShortInConfig(pci_config, DEVICE_ID_OFFSET, device_id);

	// Clear command and status
	setUnsignedIntInConfig(pci_config, COMMAND_OFFSET, 0x00000000);

	unsigned int pciClass = (unsigned int)jsongStringItemToUnsignedByte(cJSON_GetObjectItem(json, "class"));
	unsigned int pciSubclass = (unsigned int)jsongStringItemToUnsignedByte(cJSON_GetObjectItem(json, "subclass"));
	unsigned int pciProgIf = (unsigned int)jsongStringItemToUnsignedByte(cJSON_GetObjectItem(json, "prog_if"));
	unsigned int pciRevisionId = (unsigned int)jsongStringItemToUnsignedByte(cJSON_GetObjectItem(json, "revision_id"));	
	setUnsignedIntInConfig(pci_config, REVISION_ID_OFFSET, ((pciClass << 24) & 0xff000000) &
													  	   ((pciSubclass << 16) & 0x00ff0000) &
													  	   ((pciProgIf << 8) & 0x0000ff00) &
													  	   (pciRevisionId & 0x000000ff));


	char buf[9] = { 0 };
	for (unsigned int i = 0; i < 6; ++i)
	{
		snprintf(buf, 9, "bar%1usize", i);
		unsigned int barSize = jsonStringItemToUnsignedInteger(cJSON_GetObjectItem(json, buf));
		if (barSize != 0)
		{
			barInfoCache[i].size = barSize;
			barInfoCache[i].address = allocateIn4GBRange(barSize);
			barInfoCache[i].restoreAddress = 1;
			setUnsignedIntInConfig(pci_config, BAR0_OFFSET + i * 4, barInfoCache[i].address);
		}
		else
			setUnsignedIntInConfig(pci_config, BAR0_OFFSET + i * 4, 0);
	}

	setUnsignedIntInConfig(pci_config, EXPANSION_ROM_OFFSET, romAddress);
	uint32_t* rom_address = (uint32_t*)(pci_config + EXPANSION_ROM_OFFSET);
	*rom_address = rom;

	return pci_config;
}

ulong PCI_accessReg(int index, ulong value, int func, PCIDeviceInfo *info)
{
	switch (func)
	{
	case PCI_READ_BYTE:
		printf("Reading byte from config at index %d\n", index);
		return (uchar)*(pci_config + index);
	case PCI_READ_WORD:
		printf("Reading word from config at index %d\n", index);
		return (ushort)*(pci_config + index);
	case PCI_READ_DWORD:
	{
		ulong result = (ulong)*(pci_config + index);

		int readingSize = 0;
		// If we are reading from a bar, we might have to reset the address to the cached address
		if(index >= BAR0_OFFSET && index <= BAR5_OFFSET)
		{
			int barIndex = (index - BAR0_OFFSET) / 4;
			if (barInfoCache[barIndex].restoreAddress)
			{
				readingSize = 1;
				barInfoCache[barIndex].restoreAddress = 0;
				*(pci_config + index) = (ulong)barInfoCache[barIndex].address;
			}
		}

		if (readingSize)
			printf("Reading size from config at index %d\n", index);
		else
			printf("Reading dword from config at index %d\n", index);

		return result;
	}
	case PCI_WRITE_BYTE:
		printf("Writing byte to config at index %d, value: %#x\n", index, value);
		*(pci_config + index) = (uchar)value;
		break;
	case PCI_WRITE_WORD:
		printf("Writing word to config at index %d, value: %#x\n", index, value);
		*(pci_config + index) = (ushort)value;
		break;
	case PCI_WRITE_DWORD:
		printf("Writing word to config at index %d, value: %#x\n", index, value);
		*(pci_config + index) = value;

		// If writing 0xffffffff into a bar, we have to set the size of the bar in the register
		if (value == 0xffffffff && (index >= BAR0_OFFSET && index <= BAR5_OFFSET))
		{
			int barIndex = (index - BAR0_OFFSET) / 4;
			*(pci_config + index) = (ulong)barInfoCache[barIndex].size;
		}
		break;
	}

	return value;
}
