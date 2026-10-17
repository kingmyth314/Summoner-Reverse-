// unnamed_exceptionhandler_0x5419a0.cpp - Sum.exe v1.40, code 0x5419a0-0x5424fa (sum.symbols module "exceptionhandler")
// FILE NAME AND DIRECTORY ARE PLACEHOLDERS: the unit has no assert or path string. sum.symbols' tag and the
// function names come from Bruce Dawson's ExceptionHandler.cpp (Game Developer, 1999), which this object
// matches by its strings and structure; the code below is written from Sum.exe's disassembly only. It sits
// in the vsdk block, between character.cpp's tail (skeleton__scalar_deleting_dtor 0x541980) and
// skeleton.cpp (0x543040).
// Written from the Sum.exe disassembly (tools/asm.mjs). Target: VC++ 6.0 (cl 12.00.8966), /O2 /Ob1.
// Confidence per function: exact = expect identical code, likely = same semantics, small layout risk,
// unsure = the source shape is a guess.
//
// .data 0x5ac368-0x5ac8bf: string literals in function order (two "Unknown" in RecordExceptionInfo, two in
// RecordSystemInformation; "%s" and "\r\n" twice each: no string pooling). .bss: the been-here flag
// (0x2ee250c) and the two "" literals of the "= \"\"" initialisers (0x2ee2510, 0x2ee2514).
// SEH scope tables: RecordExceptionInfo 0x581e78 (two __try blocks, both at the outer level),
// ShowModuleInfo 0x581e90 (one). Imports: KERNEL32 GetModuleFileNameA, lstrcpyA, lstrlenA, CreateFileA,
// SetFilePointer, WriteFile, CloseHandle, OutputDebugStringA, VirtualQuery, GetSystemInfo,
// GlobalMemoryStatus, GetFileSize, GetFileTime, FileTimeToLocalFileTime, FileTimeToDosDateTime,
// GetSystemTimeAsFileTime; USER32 wsprintfA, wvsprintfA; ADVAPI32 GetUserNameA.
//
// mingw has no __try / __except and no MSVC inline assembly; the syntax check sees stand-ins (D601 did
// the same for WinMain).

#include <windows.h>
#include <string.h>
#include <stdarg.h>
#include "vsdk/os/exceptionhandler.h"

#ifndef _MSC_VER
#define __try if (1)
#define __except(filter) else if ((filter), 0)
#endif

#define ONE_K         1024
#define SIXTY_FOUR_K  (64 * ONE_K)
#define ONE_M         (ONE_K * ONE_K)
#define ONE_G         (ONE_K * ONE_K * ONE_K)

#define NUM_CODE_BYTES   16              // bytes dumped at CS:EIP
#define MAX_STACK_DUMP   2048            // dwords (8 KB) of stack at most
#define STACK_DUMP_SLACK 50              // flush the line buffer when fewer bytes are left

static void hprintf(HANDLE file, const char *format, ...);
static void RecordModuleList(HANDLE file);
static void ShowModuleInfo(HANDLE file, HINSTANCE module);
static void PrintTime(char *output, FILETIME time);
static void RecordSystemInformation(HANDLE file);
static const char *GetExceptionDescription(DWORD code);
static char *GetFilePart(char *path);

// 0x5419a0  Confidence: likely (frame: the locals the __except blocks need stay in memory; the loop index of
// the code-byte dump is reloaded from its slot after an exception; the 50-byte slack is a stored local).
// Runs once: a second exception (also one raised while writing the report) returns at once. Always
// returns EXCEPTION_CONTINUE_SEARCH (0), so WinMain's __except body never runs and the exception goes on to
// the system handler.
int RecordExceptionInfo(PEXCEPTION_POINTERS data, const char *message)
{
    static bool been_here = false;      // 0x2ee250c
    char module_name[MAX_PATH] = "Unknown";
    char file_name[MAX_PATH];
    char *file_part;
    char *dot;
    HANDLE log_file;
    PEXCEPTION_RECORD exception;
    PCONTEXT context;
    char crash_module_path[MAX_PATH];
    const char *crash_module_name;
    MEMORY_BASIC_INFORMATION mem_info;
    unsigned char *code;
    int code_byte;
    DWORD *stack;
    DWORD *stack_top;

    if (been_here)
        return EXCEPTION_CONTINUE_SEARCH;
    been_here = true;

    if (GetModuleFileNameA(NULL, file_name, MAX_PATH) <= 0)
        file_name[0] = '\0';
    file_part = GetFilePart(file_name);
    lstrcpyA(module_name, file_part);
    dot = strrchr(module_name, '.');
    if (dot)
        *dot = '\0';
    lstrcpyA(file_part, "errorlog.txt");

    log_file = CreateFileA(file_name, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
    if (log_file == INVALID_HANDLE_VALUE) {
        OutputDebugStringA("Error creating exception report");
        return EXCEPTION_CONTINUE_SEARCH;
    }
    SetFilePointer(log_file, 0, NULL, FILE_END);
    hprintf(log_file, "\r\n\r\n\r\n\r\n");

    exception = data->ExceptionRecord;
    context = data->ContextRecord;

    crash_module_name = "Unknown";
    if (VirtualQuery((void *)context->Eip, &mem_info, sizeof(mem_info))
        && GetModuleFileNameA((HINSTANCE)mem_info.AllocationBase, crash_module_path, MAX_PATH) > 0)
        crash_module_name = GetFilePart(crash_module_path);

    hprintf(log_file, "%s caused %s in module %s at %04x:%08x.\r\n", module_name,
            GetExceptionDescription(exception->ExceptionCode), crash_module_name, context->SegCs, context->Eip);
    hprintf(log_file, "Exception handler called in %s.\r\n", message);
    RecordSystemInformation(log_file);

    if (exception->ExceptionCode == STATUS_ACCESS_VIOLATION && exception->NumberParameters >= 2) {
        char text[1000];
        const char *read_write = "Read from";
        if (exception->ExceptionInformation[0])
            read_write = "Write to";
        wsprintfA(text, "%s location %08x caused an access violation.\r\n", read_write,
                  exception->ExceptionInformation[1]);
        hprintf(log_file, "%s", text);
    }

    hprintf(log_file, "\r\n");
    hprintf(log_file, "Registers:\r\n");
    hprintf(log_file, "EAX=%08x CS=%04x EIP=%08x EFLGS=%08x\r\n", context->Eax, context->SegCs, context->Eip,
            context->EFlags);
    hprintf(log_file, "EBX=%08x SS=%04x ESP=%08x EBP=%08x\r\n", context->Ebx, context->SegSs, context->Esp,
            context->Ebp);
    hprintf(log_file, "ECX=%08x DS=%04x ESI=%08x FS=%04x\r\n", context->Ecx, context->SegDs, context->Esi,
            context->SegFs);
    hprintf(log_file, "EDX=%08x ES=%04x EDI=%08x GS=%04x\r\n", context->Edx, context->SegEs, context->Edi,
            context->SegGs);

    hprintf(log_file, "Bytes at CS:EIP:\r\n");
    code = (unsigned char *)context->Eip;
    for (code_byte = 0; code_byte < NUM_CODE_BYTES; code_byte++) {
        __try {
            hprintf(log_file, "%02x ", code[code_byte]);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            hprintf(log_file, "?? ");
        }
    }

    hprintf(log_file, "\r\nStack dump:\r\n");
    __try {
        stack = (DWORD *)context->Esp;
#ifdef _MSC_VER
        __asm {
            mov eax, fs:[4]
            mov stack_top, eax
        }
#else
        stack_top = (DWORD *)__readfsdword(4);
#endif
        if (stack_top > stack + MAX_STACK_DUMP)
            stack_top = stack + MAX_STACK_DUMP;

        int count = 0;
        char buffer[1000] = "";
        const int slack = STACK_DUMP_SLACK;
        char *near_end = buffer + sizeof(buffer) - slack;
        char *output = buffer;
        while (stack + 1 <= stack_top) {
            if ((count % 8) == 0)
                output += wsprintfA(output, "%08x: ", stack);
            const char *suffix = " ";
            if ((++count % 8) == 0 || stack + 2 > stack_top)
                suffix = "\r\n";
            output += wsprintfA(output, "%08x%s", *stack, suffix);
            stack++;
            if (output > near_end) {
                hprintf(log_file, "%s", buffer);
                buffer[0] = '\0';
                output = buffer;
            }
        }
        hprintf(log_file, "%s", buffer);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        hprintf(log_file, "Exception encountered during stack dump.\r\n");
    }

    RecordModuleList(log_file);
    CloseHandle(log_file);
    return EXCEPTION_CONTINUE_SEARCH;
}

// 0x541ec0  Confidence: exact. Frame 0x7d4: the 2000-byte buffer and the WriteFile count.
static void hprintf(HANDLE file, const char *format, ...)
{
    char buffer[2000];
    DWORD written;
    va_list args;

    va_start(args, format);
    wvsprintfA(buffer, format, args);
    va_end(args);
    WriteFile(file, buffer, lstrlenA(buffer), &written, NULL);
}

// 0x541f10  Confidence: exact (`mov esi, 0` instead of xor for the page number). Walks the low 4 GB of the
// address space page region by page region and prints every committed allocation base (each module once).
static void RecordModuleList(HANDLE file)
{
    SYSTEM_INFO system_info;
    MEMORY_BASIC_INFORMATION mem_info;
    size_t page_size;
    size_t num_pages;
    size_t page_num;
    void *last_base;

    hprintf(file, "\r\n\tModule list: names, addresses, sizes, time stamps and file times:\r\n");
    GetSystemInfo(&system_info);
    page_size = system_info.dwPageSize;
    num_pages = 4 * (ONE_G / page_size);
    page_num = 0;
    last_base = 0;
    while (page_num < num_pages) {
        if (VirtualQuery((void *)(page_num * page_size), &mem_info, sizeof(mem_info)) && mem_info.RegionSize > 0) {
            page_num += mem_info.RegionSize / page_size;
            if (mem_info.State == MEM_COMMIT && mem_info.AllocationBase > last_base) {
                last_base = mem_info.AllocationBase;
                ShowModuleInfo(file, (HINSTANCE)last_base);
            }
        } else {
            page_num += SIXTY_FOUR_K / page_size;
        }
    }
}

// 0x541fb0  Confidence: likely (the returns inside the __try only reset the try level). Prints the module's
// path, base, file size, link time stamp and file date; any fault while reading the headers is swallowed.
static void ShowModuleInfo(HANDLE file, HINSTANCE module)
{
    char module_name[MAX_PATH];

    __try {
        if (GetModuleFileNameA(module, module_name, sizeof(module_name)) > 0) {
            IMAGE_DOS_HEADER *dos_header = (IMAGE_DOS_HEADER *)module;
            if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
                return;
            IMAGE_NT_HEADERS *nt_header = (IMAGE_NT_HEADERS *)((char *)dos_header + dos_header->e_lfanew);
            if (nt_header->Signature != IMAGE_NT_SIGNATURE)
                return;

            HANDLE module_file = CreateFileA(module_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                             FILE_ATTRIBUTE_NORMAL, NULL);
            char time_buffer[100] = "";
            DWORD file_size = 0;
            if (module_file != INVALID_HANDLE_VALUE) {
                FILETIME last_write;
                file_size = GetFileSize(module_file, NULL);
                if (GetFileTime(module_file, NULL, NULL, &last_write)) {
                    wsprintfA(time_buffer, " - file date is ");
                    PrintTime(time_buffer + lstrlenA(time_buffer), last_write);
                }
                CloseHandle(module_file);
            }
            hprintf(file, "%s, loaded at 0x%08x - %d bytes - %08x%s\r\n", module_name, module, file_size,
                    nt_header->FileHeader.TimeDateStamp, time_buffer);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}

// 0x542130  Confidence: exact. Converts in place (the FILETIME argument is both source and destination);
// month / day / year hour:minute:second in local time, an empty string on failure.
static void PrintTime(char *output, FILETIME time)
{
    WORD date, clock;

    if (FileTimeToLocalFileTime(&time, &time) && FileTimeToDosDateTime(&time, &date, &clock)) {
        wsprintfA(output, "%d/%d/%d %02d:%02d:%02d", (date / 32) & 15, date & 31, (date / 512) + 1980,
                  clock >> 11, (clock >> 5) & 0x3f, (clock & 0x1f) * 2);
    } else {
        output[0] = '\0';
    }
}

// 0x5421d0  Confidence: exact (frame 0x280 = the seven locals).
static void RecordSystemInformation(HANDLE file)
{
    FILETIME current_time;
    char time_buffer[100];
    char module_name[MAX_PATH];
    char user_name[200];
    DWORD user_name_size;
    SYSTEM_INFO system_info;
    MEMORYSTATUS mem_info;

    GetSystemTimeAsFileTime(&current_time);
    PrintTime(time_buffer, current_time);
    hprintf(file, "Error occurred at %s.\r\n", time_buffer);

    if (GetModuleFileNameA(NULL, module_name, sizeof(module_name)) <= 0)
        lstrcpyA(module_name, "Unknown");
    user_name_size = sizeof(user_name);
    if (!GetUserNameA(user_name, &user_name_size))
        lstrcpyA(user_name, "Unknown");
    hprintf(file, "%s, run by %s.\r\n", module_name, user_name);

    GetSystemInfo(&system_info);
    hprintf(file, "%d processor(s), type %d.\r\n", system_info.dwNumberOfProcessors, system_info.dwProcessorType);

    mem_info.dwLength = sizeof(mem_info);
    GlobalMemoryStatus(&mem_info);
    hprintf(file, "%d MBytes physical memory.\r\n", (mem_info.dwTotalPhys + ONE_M - 1) / ONE_M);
}

// 0x5422f0  Confidence: exact. The 24-entry table is built on the stack on every call.
static const char *GetExceptionDescription(DWORD code)
{
    struct exception_name {
        DWORD code;
        const char *name;
    };
    exception_name names[] = {
        { 0x40010005, "a Control-C" },
        { 0x40010008, "a Control-Break" },
        { 0x80000002, "a Datatype Misalignment" },
        { 0x80000003, "a Breakpoint" },
        { 0xc0000005, "an Access Violation" },
        { 0xc0000006, "an In Page Error" },
        { 0xc0000017, "a No Memory" },
        { 0xc000001d, "an Illegal Instruction" },
        { 0xc0000025, "a Noncontinuable Exception" },
        { 0xc0000026, "an Invalid Disposition" },
        { 0xc000008c, "a Array Bounds Exceeded" },
        { 0xc000008d, "a Float Denormal Operand" },
        { 0xc000008e, "a Float Divide by Zero" },
        { 0xc000008f, "a Float Inexact Result" },
        { 0xc0000090, "a Float Invalid Operation" },
        { 0xc0000091, "a Float Overflow" },
        { 0xc0000092, "a Float Stack Check" },
        { 0xc0000093, "a Float Underflow" },
        { 0xc0000094, "an Integer Divide by Zero" },
        { 0xc0000095, "an Integer Overflow" },
        { 0xc0000096, "a Privileged Instruction" },
        { 0xc00000fd, "a Stack Overflow" },
        { 0xc0000142, "a DLL Initialization Failed" },
        { 0xe06d7363, "a Microsoft C++ Exception" },
    };
    unsigned int i;

    for (i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        if (code == names[i].code)
            return names[i].name;
    }
    return "Unknown exception type";
}

// 0x5424e0  Confidence: exact.
static char *GetFilePart(char *path)
{
    char *result = strrchr(path, '\\');
    if (result)
        result++;
    else
        result = path;
    return result;
}
