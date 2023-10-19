#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "BiosEmulator/include/biosemu.h"
#include "BiosEmulator/include/pci_accessReg.h"
#include "cJSON.h"
#include "MemAllocator.h"

void printUsage()
{
    printf("Usage: Analyzer -f <filename>\n" \
           "Where the file name is a json file describing the PCI configuration space and the rom file name.\n");
}

cJSON* readConf(const char* fileName)
{
    FILE* file = fopen(fileName, "rb");
    if (file == NULL)
    {
        printf("Could not open file %s\n", fileName);
        return NULL;
    }

    // read the file
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* file_contents = malloc(file_size + 1);
    memset(file_contents, 0, file_size + 1);
    fread(file_contents, file_size, 1, file);
    fclose(file);

    cJSON* conf = cJSON_Parse(file_contents);
    free(file_contents);
    return conf;
}

uintptr_t readROM(const char* filename)
{
    FILE* file = fopen(filename, "rb");
    if (file == NULL)
    {
        printf("Could not open file %s\n", filename);
        return 0;
    }

    // read the file
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uintptr_t file_contents = allocateIn4GBRange(file_size + 1);
    memset((void*)file_contents, 0, file_size + 1);
    fread((void*)file_contents, file_size, 1, file);
    fclose(file);

    return file_contents;
}

int main(int argc, char* argv[])
{
    // The rom needs to be in the first 4GB of memory so we use uintptr_t instead of void* to allow easier casting
    uintptr_t rom = 0;
    cJSON* pciCONF = NULL;
    int32_t returnCode = 0;

    // find -f followed by a filename in argv
    if (argc < 2)
    {
        printUsage();
        goto error;
    }

    if (strcmp(argv[1], "-f") != 0)
    {
        printUsage();
        goto error;
    }

    if (argc < 3)
    {
        printUsage();
        goto error;
    }

    pciCONF = readConf(argv[2]);
    if (pciCONF == NULL)
    {
        printf("Could not parse file %s\n", argv[2]);
        goto error;
    }

    // get the rom file name
    cJSON* romItem = cJSON_GetObjectItem(pciCONF, "rom");
    if (romItem == NULL)
    {
        printf("Could not find rom file name in %s\n", argv[2]);
        goto error;
    }

    const char* filename = romItem->valuestring;
    rom = readROM(filename);
    unsigned char* config = buildConfigFromJsonAndRom(pciCONF, rom);
    if (config == NULL)
    {
        printf("Could not build config from json file %s\n", argv[2]);
        goto error;
    }

    unsigned char* romBytes = (unsigned char*)rom;
    // analyze the file
    // Check if the first two bytes are 0x55 0xAA
    if (romBytes[0] != 0x55 || romBytes[1] != 0xAA)
    {
        printf("File %s is not a bios extension\n", filename);
        goto error;
    }

    // Initialize the bios emulator
    BE_VGAInfo vga_info;
    memset(&vga_info, 0, sizeof(vga_info));

    BE_init(DEBUG_DECODE_F | DEBUG_TRACECALL_F | DEBUG_MEM_TRACE_F | DEBUG_TRACE_F, 65536, &vga_info, 0);

    goto cleanup;

error:
    returnCode = 1;

cleanup:
    cJSON_Delete(pciCONF);
    free((void*)rom);
    return returnCode;
}