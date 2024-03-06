#include "config_ini.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void parse_ini_file(const char* filename, IniData* iniData) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return;
    }

    char line[MAX_LINE_LENGTH];
    char section[MAX_SECTION_LENGTH] = "";  // Initialize section buffer

    while (fgets(line, MAX_LINE_LENGTH, file) != NULL) {
        // Remove trailing newline character
        strtok(line, "\n");

        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';')
            continue;

        // Parse sections
        if (line[0] == '[') {
            // Extract section name
            strcpy(section, line + 1);
            section[strlen(section) - 1] = '\0';  // Remove trailing ']'
        }
        // Parse key-value pairs
        else if (sscanf(line, "%[^=]=%s", iniData->entries[iniData->count].key, iniData->entries[iniData->count].value) == 2) {
            // Copy the section name into the current entry
            strcpy(iniData->entries[iniData->count].section, section);
            iniData->count++;
        }
    }

    fclose(file);
}


const char* get_value(const IniData* iniData, const char* section, const char* key) {
    for (int i = 0; i < iniData->count; ++i) {
        if (strcmp(iniData->entries[i].section, section) == 0 && strcmp(iniData->entries[i].key, key) == 0) {
            return iniData->entries[i].value;
        }
    }
    return NULL;
}