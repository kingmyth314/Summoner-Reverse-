// registry.h - Sum.exe v1.40 registry access (vsdk\os\registry.cpp, 0x511d80-0x5124b5)
// Written from the Sum.exe disassembly. Names from symbols/sum.symbols.
#ifndef REGISTRY_H
#define REGISTRY_H

#include <windows.h>

class String;

// subkey: NULL = the application key (os_registry_app_key), "HKEY_xxx\path" = absolute,
// anything else = relative to the application key. All keys default to HKEY_CURRENT_USER.
// Return values: 0 = success, -1 = failure (the error text is formatted but never shown).

extern char os_registry_app_key[256];  // 0x2692ef8 "Software\<company>\<app>"
extern bool os_registry_app_key_set;   // 0x2692ff8

int os_registry_write(const char *subkey, const char *name, DWORD type, const void *data, DWORD size);
HKEY os_registry_resolve_key(char *out_path, const char *subkey);
HKEY os_registry_hkey_from_string(const char *s);
int os_registry_read(const char *subkey, const char *name, void *data, DWORD size);
int os_registry_set_app_key(const char *company, const char *app);
int os_registry_read_String(const char *subkey, const char *name, String *out);
int os_registry_read_string_nodefault(const char *subkey, const char *name, char *out);
void os_registry_read_string(const char *subkey, const char *name, char *out, const char *default_value);
int os_registry_write_string(const char *subkey, const char *name, const char *s);
int os_registry_read_dword(const char *subkey, const char *name, DWORD *out, DWORD default_value);
int os_registry_write_dword(const char *subkey, const char *name, DWORD value);
int os_registry_read_int(const char *subkey, const char *name, int *out, int default_value);
int os_registry_write_binary_int(const char *subkey, const char *name, int value);
int os_registry_delete(const char *subkey, const char *name);

#endif
