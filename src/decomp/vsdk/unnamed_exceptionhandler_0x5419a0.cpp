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

// The instruction and stack pointers of the crash context: Eip / Esp in the 32-bit builds (the original's text),
// Rip / Rsp in a 64-bit build (docs/future/modern-port.md, L3).
#ifdef _WIN64
#define CONTEXT_IP(c) ((c)->Rip)
#define CONTEXT_SP(c) ((c)->Rsp)
#define ADDR_FMT "%016I64x"           // code and fault addresses in full
#else
#define CONTEXT_IP(c) ((c)->Eip)
#define CONTEXT_SP(c) ((c)->Esp)
#define ADDR_FMT "%08x"
#endif

static void hprintf(HANDLE file, const char *format, ...);
static void RecordModuleList(HANDLE file);
static void ShowModuleInfo(HANDLE file, HINSTANCE module);
static void PrintTime(char *output, FILETIME time);
static void RecordSystemInformation(HANDLE file);
static const char *GetExceptionDescription(DWORD code);
static char *GetFilePart(char *path);

// 0x5419a0  Confidence: exact (VC6 8966 byte match, w1 / D11000: locals declared where they are first used, with
// the names of the article the unit follows, which give the binary's frame slots; the locals the __except
// blocks need stay in memory; the loop index of the code-byte dump is reloaded from its slot after an
// exception; the 50-byte slack is a stored local).
// Runs once: a second exception (also one raised while writing the report) returns at once. Always
// returns EXCEPTION_CONTINUE_SEARCH (0), so WinMain's __except body never runs and the exception goes on to
// the system handler.
int RecordExceptionInfo(PEXCEPTION_POINTERS data, const char *message)
{
    static bool been_here = false;      // 0x2ee250c

    if (been_here)
        return EXCEPTION_CONTINUE_SEARCH;
    been_here = true;

    char ModuleName[MAX_PATH];
    char FileName[MAX_PATH] = "Unknown";
    if (GetModuleFileNameA(NULL, ModuleName, MAX_PATH) <= 0)
        ModuleName[0] = '\0';
    char *FilePart = GetFilePart(ModuleName);
    lstrcpyA(FileName, FilePart);
    char *lastperiod = strrchr(FileName, '.');
    if (lastperiod)
        *lastperiod = '\0';
    lstrcpyA(FilePart, "errorlog.txt");

    HANDLE LogFile = CreateFileA(ModuleName, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
    if (LogFile == INVALID_HANDLE_VALUE) {
        OutputDebugStringA("Error creating exception report");
        return EXCEPTION_CONTINUE_SEARCH;
    }
    SetFilePointer(LogFile, 0, NULL, FILE_END);
    hprintf(LogFile, "\r\n\r\n\r\n\r\n");

    PEXCEPTION_RECORD Exception = data->ExceptionRecord;
    PCONTEXT Context = data->ContextRecord;

    char CrashModulePathName[MAX_PATH];
    const char *CrashModuleFileName = "Unknown";
    MEMORY_BASIC_INFORMATION MemInfo;
    if (VirtualQuery((void *)CONTEXT_IP(Context), &MemInfo, sizeof(MemInfo))
        && GetModuleFileNameA((HINSTANCE)MemInfo.AllocationBase, CrashModulePathName, MAX_PATH) > 0)
        CrashModuleFileName = GetFilePart(CrashModulePathName);

    hprintf(LogFile, "%s caused %s in module %s at %04x:" ADDR_FMT ".\r\n", FileName,
            GetExceptionDescription(Exception->ExceptionCode), CrashModuleFileName, Context->SegCs,
            CONTEXT_IP(Context));
    hprintf(LogFile, "Exception handler called in %s.\r\n", message);
    RecordSystemInformation(LogFile);

    if (Exception->ExceptionCode == STATUS_ACCESS_VIOLATION && Exception->NumberParameters >= 2) {
        char DebugMessage[1000];
        const char *readwrite = "Read from";
        if (Exception->ExceptionInformation[0])
            readwrite = "Write to";
        wsprintfA(DebugMessage, "%s location " ADDR_FMT " caused an access violation.\r\n", readwrite,
                  Exception->ExceptionInformation[1]);
        hprintf(LogFile, "%s", DebugMessage);
    }

    hprintf(LogFile, "\r\n");
    hprintf(LogFile, "Registers:\r\n");
#ifdef _WIN64
    hprintf(LogFile, "RAX=%016I64x RBX=%016I64x RCX=%016I64x RDX=%016I64x\r\n", Context->Rax, Context->Rbx,
            Context->Rcx, Context->Rdx);
    hprintf(LogFile, "RSI=%016I64x RDI=%016I64x RBP=%016I64x RSP=%016I64x\r\n", Context->Rsi, Context->Rdi,
            Context->Rbp, Context->Rsp);
    hprintf(LogFile, "RIP=%016I64x EFLGS=%08x CS=%04x SS=%04x\r\n", Context->Rip, Context->EFlags, Context->SegCs,
            Context->SegSs);
#else
    hprintf(LogFile, "EAX=%08x CS=%04x EIP=%08x EFLGS=%08x\r\n", Context->Eax, Context->SegCs, Context->Eip,
            Context->EFlags);
    hprintf(LogFile, "EBX=%08x SS=%04x ESP=%08x EBP=%08x\r\n", Context->Ebx, Context->SegSs, Context->Esp,
            Context->Ebp);
    hprintf(LogFile, "ECX=%08x DS=%04x ESI=%08x FS=%04x\r\n", Context->Ecx, Context->SegDs, Context->Esi,
            Context->SegFs);
    hprintf(LogFile, "EDX=%08x ES=%04x EDI=%08x GS=%04x\r\n", Context->Edx, Context->SegEs, Context->Edi,
            Context->SegGs);
#endif

    hprintf(LogFile, "Bytes at CS:EIP:\r\n");
    unsigned char *code = (unsigned char *)CONTEXT_IP(Context);
    for (int codebyte = 0; codebyte < NUM_CODE_BYTES; codebyte++) {
        __try {
            hprintf(LogFile, "%02x ", code[codebyte]);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            hprintf(LogFile, "?? ");
        }
    }

    hprintf(LogFile, "\r\nStack dump:\r\n");
    __try {
        DWORD *pStack = (DWORD *)CONTEXT_SP(Context);
        DWORD *pStackTop;
#if defined(_MSC_VER) && !defined(MODERN_BUILD)
        __asm {
            mov eax, fs:[4]
            mov pStackTop, eax
        }
#else
        // The stack's top (fs:[4] in 32-bit code): the end of the committed region that holds the stack pointer.
        MEMORY_BASIC_INFORMATION StackInfo;
        pStackTop = pStack;
        if (VirtualQuery(pStack, &StackInfo, sizeof(StackInfo)))
            pStackTop = (DWORD *)((char *)StackInfo.BaseAddress + StackInfo.RegionSize);
#endif
        if (pStackTop > pStack + MAX_STACK_DUMP)
            pStackTop = pStack + MAX_STACK_DUMP;

        int Count = 0;
        char buffer[1000] = "";
        const int safetyzone = STACK_DUMP_SLACK;
        char *nearend = buffer + sizeof(buffer) - safetyzone;
        char *output = buffer;
        while (pStack + 1 <= pStackTop) {
            if ((Count % 8) == 0)
                output += wsprintfA(output, "%08x: ", pStack);
            const char *Suffix = " ";
            if ((++Count % 8) == 0 || pStack + 2 > pStackTop)
                Suffix = "\r\n";
            output += wsprintfA(output, "%08x%s", *pStack, Suffix);
            pStack++;
            if (output > nearend) {
                hprintf(LogFile, "%s", buffer);
                buffer[0] = '\0';
                output = buffer;
            }
        }
        hprintf(LogFile, "%s", buffer);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        hprintf(LogFile, "Exception encountered during stack dump.\r\n");
    }

    RecordModuleList(LogFile);
    CloseHandle(LogFile);
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

// 0x541fb0  Confidence: exact (VC6 8966 byte match, w1 / D11000: the article's local names give the frame slots;
// the returns inside the __try only reset the try level). Prints the module's
// path, base, file size, link time stamp and file date; any fault while reading the headers is swallowed.
static void ShowModuleInfo(HANDLE file, HINSTANCE module)
{
    char ModName[MAX_PATH];

    __try {
        if (GetModuleFileNameA(module, ModName, sizeof(ModName)) > 0) {
            IMAGE_DOS_HEADER *DosHeader = (IMAGE_DOS_HEADER *)module;
            if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
                return;
            IMAGE_NT_HEADERS *NTHeader = (IMAGE_NT_HEADERS *)((char *)DosHeader + DosHeader->e_lfanew);
            if (NTHeader->Signature != IMAGE_NT_SIGNATURE)
                return;

            HANDLE ModuleFile = CreateFileA(ModName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                             FILE_ATTRIBUTE_NORMAL, NULL);
            char TimeBuffer[100] = "";
            DWORD FileSize = 0;
            if (ModuleFile != INVALID_HANDLE_VALUE) {
                FILETIME LastWriteTime;
                FileSize = GetFileSize(ModuleFile, NULL);
                if (GetFileTime(ModuleFile, NULL, NULL, &LastWriteTime)) {
                    wsprintfA(TimeBuffer, " - file date is ");
                    PrintTime(TimeBuffer + lstrlenA(TimeBuffer), LastWriteTime);
                }
                CloseHandle(ModuleFile);
            }
            hprintf(file, "%s, loaded at 0x%08x - %d bytes - %08x%s\r\n", ModName, module, FileSize,
                    NTHeader->FileHeader.TimeDateStamp, TimeBuffer);
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
