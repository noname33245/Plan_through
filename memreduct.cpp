// 防止Windows.h与Qt的min/max宏冲突
#define WIN32_LEAN_AND_MEAN

// Qt核心头文件
#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QCoreApplication>
#include <QString>

// Windows系统API头文件
#include <windows.h>
#include <psapi.h>
#include <shellapi.h>
#include <stdio.h>

// 定义NTSTATUS类型
typedef LONG NTSTATUS;

// 链接依赖库（仅MSVC支持）
#ifdef _MSC_VER
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "ntdll.lib")
#endif

// ===================== 全量清理掩码（开启所有可清理内存区域） =====================
#define REDUCT_WORKING_SET              0x01  // 所有进程工作集
#define REDUCT_SYSTEM_FILE_CACHE        0x02  // 系统文件缓存
#define REDUCT_STANDBY_PRIORITY0_LIST   0x04  // 低优先级备用列表
#define REDUCT_STANDBY_LIST             0x08  // 全量备用列表
#define REDUCT_MODIFIED_LIST             0x10  // 修改页面列表
#define REDUCT_COMBINE_MEMORY_LISTS     0x20  // Win10+合并内存列表
#define REDUCT_REGISTRY_CACHE            0x40  // Win8.1+注册表缓存
#define REDUCT_MODIFIED_FILE_CACHE       0x80  // 修改的文件缓存
// 全量清理总掩码
#define REDUCT_MASK_FULL (REDUCT_WORKING_SET | REDUCT_SYSTEM_FILE_CACHE | REDUCT_STANDBY_PRIORITY0_LIST | REDUCT_STANDBY_LIST | REDUCT_MODIFIED_LIST | REDUCT_COMBINE_MEMORY_LISTS | REDUCT_REGISTRY_CACHE | REDUCT_MODIFIED_FILE_CACHE)

// 定义NT_SUCCESS宏
#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#endif

// ===================== Windows Native API 常量与类型定义 =====================
#define SystemFileCacheInformation        0x0015
#define SystemMemoryListCommand           0x0050
#define SystemCombinedMemoryListInformation 0x0057
#define SystemRegistryReconciliationInformation 0x0056
#define SystemModifiedPageListByFileInformation 0x0051
#define MemoryFlushModifiedList           0x0003
#define MemoryPurgeStandbyList            0x0004
#define MemoryPurgeLowPriorityStandbyList 0x0005

// Native API 函数指针类型定义
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

// 系统文件缓存信息结构体
typedef struct _SYSTEM_FILECACHE_INFORMATION {
    LARGE_INTEGER CurrentSize;
    LARGE_INTEGER PeakSize;
    ULONG         PageFaultCount;
    LARGE_INTEGER MinimumWorkingSet;
    LARGE_INTEGER MaximumWorkingSet;
} SYSTEM_FILECACHE_INFORMATION, *PSYSTEM_FILECACHE_INFORMATION;

// 内存状态结构体
typedef struct _MEMORY_STATUS {
    struct {
        ULONGLONG TotalBytes;
        ULONGLONG UsedBytes;
        ULONGLONG FreeBytes;
        ULONG     PercentUsed;
    } PhysicalMemory;
} MEMORY_STATUS, *PMEMORY_STATUS;

// ===================== 工具函数（核心逻辑与原开源项目完全一致） =====================
// 检查是否拥有管理员权限
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

// ===================== 核心全量内存清理函数 =====================
BOOL FullMemoryClean()
{
    // 加载ntdll.dll获取原生API地址
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll) return FALSE;

    PFN_NtSetSystemInformation pfnNtSetSystemInformation = reinterpret_cast<PFN_NtSetSystemInformation>(GetProcAddress(hNtdll, "NtSetSystemInformation"));
    PFN_NtQuerySystemInformation pfnNtQuerySystemInformation = reinterpret_cast<PFN_NtQuerySystemInformation>(GetProcAddress(hNtdll, "NtQuerySystemInformation"));
    if (!pfnNtSetSystemInformation || !pfnNtQuerySystemInformation) return FALSE;

    // 1. 清理所有用户进程工作集（跳过系统关键进程、自身进程）
    if (REDUCT_MASK_FULL & REDUCT_WORKING_SET)
    {
        DWORD dwPids[4096] = {0};
        DWORD cbNeeded = 0;

        if (EnumProcesses(dwPids, sizeof(dwPids), &cbNeeded))
        {
            DWORD dwProcessCount = cbNeeded / sizeof(DWORD);
            DWORD dwCurrentPid = GetCurrentProcessId();

            for (DWORD i = 0; i < dwProcessCount; i++)
            {
                DWORD dwPid = dwPids[i];
                if (dwPid == 0 || dwPid == 4 || dwPid == dwCurrentPid)
                    continue;

                HANDLE hProcess = OpenProcess(PROCESS_SET_QUOTA | PROCESS_QUERY_INFORMATION, FALSE, dwPid);
                if (hProcess)
                {
                    SetProcessWorkingSetSize(hProcess, (SIZE_T)-1, (SIZE_T)-1);
                    CloseHandle(hProcess);
                }
            }
        }
    }

    // 2. 清理系统文件缓存
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

    // 3. 清理低优先级备用列表
    if (REDUCT_MASK_FULL & REDUCT_STANDBY_PRIORITY0_LIST)
    {
        ULONG ulCommand = MemoryPurgeLowPriorityStandbyList;
        pfnNtSetSystemInformation(SystemMemoryListCommand, &ulCommand, sizeof(ulCommand));
    }

    // 4. 清理全量备用列表
    if (REDUCT_MASK_FULL & REDUCT_STANDBY_LIST)
    {
        ULONG ulCommand = MemoryPurgeStandbyList;
        pfnNtSetSystemInformation(SystemMemoryListCommand, &ulCommand, sizeof(ulCommand));
    }

    // 5. 清理修改页面列表
    if (REDUCT_MASK_FULL & REDUCT_MODIFIED_LIST)
    {
        ULONG ulCommand = MemoryFlushModifiedList;
        pfnNtSetSystemInformation(SystemMemoryListCommand, &ulCommand, sizeof(ulCommand));
    }

    // 6. Win10+合并内存列表优化
    if (REDUCT_MASK_FULL & REDUCT_COMBINE_MEMORY_LISTS)
    {
        ULONG ulCombine = 1;
        pfnNtSetSystemInformation(SystemCombinedMemoryListInformation, &ulCombine, sizeof(ulCombine));
    }

    // 7. Win8.1+注册表缓存清理
    if (REDUCT_MASK_FULL & REDUCT_REGISTRY_CACHE)
    {
        ULONG ulRegFlush = 0;
        pfnNtSetSystemInformation(SystemRegistryReconciliationInformation, &ulRegFlush, sizeof(ulRegFlush));
    }

    // 8. 清理修改的文件缓存
    if (REDUCT_MASK_FULL & REDUCT_MODIFIED_FILE_CACHE)
    {
        ULONG ulFileFlush = 0;
        pfnNtSetSystemInformation(SystemModifiedPageListByFileInformation, &ulFileFlush, sizeof(ulFileFlush));
    }

    return TRUE;
}



// ===================== 内存状态检测函数 =====================
// 检查当前内存使用情况，返回内存使用百分比
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

// ===================== 自动内存清理函数 =====================
// 检查内存使用情况，如果超过阈值则执行清理
static BOOL AutoMemoryCleanIfNeeded(int thresholdPercent)
{
    // 非Windows平台不支持
#ifndef Q_OS_WIN
    return FALSE;
#endif

    // 检查权限
    if (!IsElevated())
    {
        return FALSE;
    }

    // 检查内存使用情况
    int usagePercent = GetMemoryUsagePercent();
    if (usagePercent >= thresholdPercent)
    {
        // 执行内存清理
        return FullMemoryClean();
    }
    return FALSE;
}

// ===================== 对外暴露的内存清理函数 =====================
void PerformMemoryClean(QWidget *parentWidget)
{
    // 非Windows平台直接返回
#ifndef Q_OS_WIN
    return;
#endif

    // 1. 权限校验
    if (!IsElevated())
    {
        // 没有管理员权限，直接显示失败提示
        if (parentWidget)
        {
            // 发送自定义信号通知主窗口显示失败提示
            QMetaObject::invokeMethod(parentWidget, "showMemoryCleanError", Qt::QueuedConnection);
        }
        return;
    }

    // 2. 直接执行内存清理，无需确认

    // 3. 执行全量内存清理
    BOOL cleanResult = FullMemoryClean();

    // 4. 结果反馈
    if (!cleanResult)
    {
        // 清理失败，通知主窗口显示失败提示
        if (parentWidget)
        {
            QMetaObject::invokeMethod(parentWidget, "showMemoryCleanError", Qt::QueuedConnection);
        }
        return;
    }

    // 静默执行，不显示结果提示
}

// ===================== 对外暴露的自动内存清理函数 =====================
// 检查内存使用情况，如果超过阈值则执行清理
bool CheckAndCleanMemory(int thresholdPercent)
{
    return AutoMemoryCleanIfNeeded(thresholdPercent);
}

// 获取当前内存使用百分比
int GetCurrentMemoryUsage()
{
    return GetMemoryUsagePercent();
}