#ifndef CONFIG_INI_H
#define CONFIG_INI_H

#define MAX_SECTION_LENGTH 256
#define MAX_KEY_LENGTH 256
#define MAX_VALUE_LENGTH 256
#define MAX_LINE_LENGTH 1024

typedef struct {
    char section[MAX_SECTION_LENGTH];
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
} IniEntry;

typedef struct {
    IniEntry entries[MAX_LINE_LENGTH];
    int count;
} IniData;

void parse_ini_file(const char* filename, IniData* iniData);
const char* get_value(const IniData* iniData, const char* section, const char* key);

#endif /* CONFIG_INI_H */
