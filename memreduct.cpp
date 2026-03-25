#define WIN32_LEAN_AND_MEAN

#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QCoreApplication>
#include <QString>

#include <windows.h>
#include <psapi.h>
#include <shellapi.h>
#include <stdio.h>
#include <tlhelp32.h>

typedef LONG NTSTATUS;

#ifdef _MSC_VER
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "advapi32.lib")
#endif

#define REDUCT_WORKING_SET              0x01
#define REDUCT_SYSTEM_FILE_CACHE        0x02
#define REDUCT_STANDBY_PRIORITY0_LIST   0x04
#define REDUCT_STANDBY_LIST             0x08
#define REDUCT_MODIFIED_LIST             0x10
#define REDUCT_COMBINE_MEMORY_LISTS     0x20
#define REDUCT_REGISTRY_CACHE            0x40
#define REDUCT_MODIFIED_FILE_CACHE       0x80
#define REDUCT_MASK_FULL (REDUCT_WORKING_SET | REDUCT_SYSTEM_FILE_CACHE | REDUCT_STANDBY_PRIORITY0_LIST | REDUCT_STANDBY_LIST | REDUCT_MODIFIED_LIST | REDUCT_COMBINE_MEMORY_LISTS | REDUCT_REGISTRY_CACHE | REDUCT_MODIFIED_FILE_CACHE)

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#endif

#define SystemFileCacheInformation        0x0015
#define SystemMemoryListCommand           0x0050
#define SystemCombinedMemoryListInformation 0x0057
#define SystemRegistryReconciliationInformation 0x0056
#define SystemModifiedPageListByFileInformation 0x0051
#define MemoryFlushModifiedList           0x0003
#define MemoryPurgeStandbyList            0x0004
#define MemoryPurgeLowPriorityStandbyList 0x0005

typedef NTSTATUS(NTAPI* PFN_NtSetSystemInformation)(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength
);
typedef NTSTATUS(NTAPI* PFN_NtQuerySystemInformation)(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
);

typedef struct _SYSTEM_FILECACHE_INFORMATION {
    LARGE_INTEGER CurrentSize;
    LARGE_INTEGER PeakSize;
    ULONG         PageFaultCount;
    LARGE_INTEGER MinimumWorkingSet;
    LARGE_INTEGER MaximumWorkingSet;
} SYSTEM_FILECACHE_INFORMATION, *PSYSTEM_FILECACHE_INFORMATION;

typedef struct _MEMORY_STATUS {
    struct {
        ULONGLONG TotalBytes;
        ULONGLONG UsedBytes;
        ULONGLONG FreeBytes;
        ULONG     PercentUsed;
    } PhysicalMemory;
} MEMORY_STATUS, *PMEMORY_STATUS;

// 来自 WinMemoryCleaner 原版的关键定义
#define SYSTEMMEMORYLISTINFORMATIONCLASS 0x50
enum SYSTEM_MEMORY_LIST_COMMAND {
    MEMORY_PURGE_STANDBY_LIST = 4,
    MEMORY_PURGE_MODIFIED_PAGE_LIST = 8,
    MEMORY_PURGE_LOW_PRIORITY_STANDBY_LIST = 5
};

typedef NTSTATUS(NTAPI* pNtSetSystemInformation)(ULONG, PVOID, ULONG);

class MemoryCleaner
{
public:
    static void cleanAll() {
        enablePrivilege();          // 提权
        cleanWorkingSet();          // 清理进程工作集（最有效）
        cleanModifiedPageList();    // 清理修改页
        cleanStandbyList();         // 清理备用列表
    }

private:
    // 提权（必须）
    static bool enablePrivilege() {
        HANDLE hToken = NULL;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
            return false;

        TOKEN_PRIVILEGES tp{};
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        LookupPrivilegeValue(NULL, L"SeProfileSingleProcessPrivilege", &tp.Privileges[0].Luid);
        AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
        CloseHandle(hToken);
        return true;
    }

    // 清理所有进程工作集（原版核心）
    static void cleanWorkingSet() {
        DWORD processes[1024], cbNeeded, count;
        if (!EnumProcesses(processes, sizeof(processes), &cbNeeded))
            return;

        count = cbNeeded / sizeof(DWORD);
        for (DWORD i = 0; i < count; i++) {
            HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processes[i]);
            if (hProcess) {
                EmptyWorkingSet(hProcess);
                CloseHandle(hProcess);
            }
        }
    }

    // 清理 Modified Page List
    static void cleanModifiedPageList() {
        HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
        if (hNtdll) {
            pNtSetSystemInformation NtSetSystemInfo = reinterpret_cast<pNtSetSystemInformation>(reinterpret_cast<FARPROC>(GetProcAddress(hNtdll, "NtSetSystemInformation")));
            if (NtSetSystemInfo) {
                int cmd = MEMORY_PURGE_MODIFIED_PAGE_LIST;
                NtSetSystemInfo(SYSTEMMEMORYLISTINFORMATIONCLASS, &cmd, sizeof(cmd));
            }
        }
    }

    // 清理 Standby List
    static void cleanStandbyList() {
        HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
        if (hNtdll) {
            pNtSetSystemInformation NtSetSystemInfo = reinterpret_cast<pNtSetSystemInformation>(reinterpret_cast<FARPROC>(GetProcAddress(hNtdll, "NtSetSystemInformation")));
            if (NtSetSystemInfo) {
                int cmd = MEMORY_PURGE_STANDBY_LIST;
                NtSetSystemInfo(SYSTEMMEMORYLISTINFORMATIONCLASS, &cmd, sizeof(cmd));
            }
        }
    }
};

static BOOL IsElevated()
{
    BOOL bRet = FALSE;
    HANDLE hToken = NULL;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);

        if (GetTokenInformation(hToken, TokenElevation, &Elevation, cbSize, &cbSize))
        {
            bRet = Elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }
    return bRet;
}

BOOL FullMemoryClean()
{
    // 首先使用新的MemoryCleaner类进行清理（WinMemoryCleaner原版方法）
    MemoryCleaner::cleanAll();
    
    // 然后使用原有的清理方法进行补充清理
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll) return FALSE;

    PFN_NtSetSystemInformation pfnNtSetSystemInformation = reinterpret_cast<PFN_NtSetSystemInformation>(reinterpret_cast<FARPROC>(GetProcAddress(hNtdll, "NtSetSystemInformation")));
    PFN_NtQuerySystemInformation pfnNtQuerySystemInformation = reinterpret_cast<PFN_NtQuerySystemInformation>(reinterpret_cast<FARPROC>(GetProcAddress(hNtdll, "NtQuerySystemInformation")));
    if (!pfnNtSetSystemInformation || !pfnNtQuerySystemInformation) return FALSE;

    // 清理系统文件缓存
    if (REDUCT_MASK_FULL & REDUCT_SYSTEM_FILE_CACHE)
    {
        SYSTEM_FILECACHE_INFORMATION sfci;
        ZeroMemory(&sfci, sizeof(SYSTEM_FILECACHE_INFORMATION));
        ULONG cbSize = sizeof(SYSTEM_FILECACHE_INFORMATION);

        NTSTATUS status = pfnNtQuerySystemInformation(SystemFileCacheInformation, &sfci, cbSize, NULL);
        if (NT_SUCCESS(status))
        {
            sfci.MinimumWorkingSet.QuadPart = -1;
            sfci.MaximumWorkingSet.QuadPart = -1;
            pfnNtSetSystemInformation(SystemFileCacheInformation, &sfci, cbSize);
        }
    }

    // 清理低优先级备用列表
    if (REDUCT_MASK_FULL & REDUCT_STANDBY_PRIORITY0_LIST)
    {
        ULONG ulCommand = MemoryPurgeLowPriorityStandbyList;
        pfnNtSetSystemInformation(SystemMemoryListCommand, &ulCommand, sizeof(ulCommand));
    }

    // Win10+合并内存列表优化
    if (REDUCT_MASK_FULL & REDUCT_COMBINE_MEMORY_LISTS)
    {
        ULONG ulCombine = 1;
        pfnNtSetSystemInformation(SystemCombinedMemoryListInformation, &ulCombine, sizeof(ulCombine));
    }

    // Win8.1+注册表缓存清理
    if (REDUCT_MASK_FULL & REDUCT_REGISTRY_CACHE)
    {
        ULONG ulRegFlush = 0;
        pfnNtSetSystemInformation(SystemRegistryReconciliationInformation, &ulRegFlush, sizeof(ulRegFlush));
    }

    // 清理修改的文件缓存
    if (REDUCT_MASK_FULL & REDUCT_MODIFIED_FILE_CACHE)
    {
        ULONG ulFileFlush = 0;
        pfnNtSetSystemInformation(SystemModifiedPageListByFileInformation, &ulFileFlush, sizeof(ulFileFlush));
    }

    return TRUE;
}

static int GetMemoryUsagePercent()
{
    MEMORYSTATUSEX msx;
    ZeroMemory(&msx, sizeof(MEMORYSTATUSEX));
    msx.dwLength = sizeof(MEMORYSTATUSEX);
    
    if (GlobalMemoryStatusEx(&msx))
    {
        return msx.dwMemoryLoad;
    }
    return 0;
}

static BOOL AutoMemoryCleanIfNeeded(int thresholdPercent)
{
#ifndef Q_OS_WIN
    return FALSE;
#endif

    if (!IsElevated())
    {
        return FALSE;
    }

    int usagePercent = GetMemoryUsagePercent();
    if (usagePercent >= thresholdPercent)
    {
        return FullMemoryClean();
    }
    return FALSE;
}

void PerformMemoryClean(QWidget *parentWidget)
{
#ifndef Q_OS_WIN
    return;
#endif

    if (!IsElevated())
    {
        if (parentWidget)
        {
            QMetaObject::invokeMethod(parentWidget, "showMemoryCleanError", Qt::QueuedConnection);
        }
        return;
    }

    BOOL cleanResult = FullMemoryClean();

    if (!cleanResult)
    {
        if (parentWidget)
        {
            QMetaObject::invokeMethod(parentWidget, "showMemoryCleanError", Qt::QueuedConnection);
        }
        return;
    }
}

bool CheckAndCleanMemory(int thresholdPercent)
{
    return AutoMemoryCleanIfNeeded(thresholdPercent);
}

int GetCurrentMemoryUsage()
{
    return GetMemoryUsagePercent();
}