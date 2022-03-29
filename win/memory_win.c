/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2019 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2019 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <windows.h>
#include <ntsecapi.h>
#include <tchar.h>

#include "options.h"
#include "persistent_memory.h"
#include "utils/applog.h"


char *persistent_memory;
int persistent_memory_flags = 0;


/*****************************************************************
SetLockPagesPrivilege: a function to obtain or
release the privilege of locking physical pages.

Inputs:

HANDLE hProcess: Handle for the process for which the
privilege is needed

BOOL bEnable: Enable (TRUE) or disable?

Return value: TRUE indicates success, FALSE failure.

*****************************************************************/
/**
 * AWE Example: https://msdn.microsoft.com/en-us/library/windows/desktop/aa366531(v=vs.85).aspx
 * Creating a File Mapping Using Large Pages: https://msdn.microsoft.com/en-us/library/aa366543(VS.85).aspx
 */
static BOOL SetLockPagesPrivilege() {
    HANDLE token;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token) != TRUE) {
        return FALSE;
    }

    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (LookupPrivilegeValue(NULL, SE_LOCK_MEMORY_NAME, &(tp.Privileges[0].Luid)) != TRUE) {
        return FALSE;
    }

    BOOL rc = AdjustTokenPrivileges(token, FALSE, (PTOKEN_PRIVILEGES) &tp, 0, NULL, NULL);
    if (rc != TRUE || GetLastError() != ERROR_SUCCESS) {
        return FALSE;
    }

    CloseHandle(token);

    return TRUE;
}


static LSA_UNICODE_STRING StringToLsaUnicodeString(LPCTSTR string) {
    LSA_UNICODE_STRING lsaString;
    DWORD dwLen = 0;

    dwLen = wcslen(string);
    lsaString.Buffer = (LPWSTR) string;
    lsaString.Length = (USHORT)((dwLen) * sizeof(WCHAR));
    lsaString.MaximumLength = (USHORT)((dwLen + 1) * sizeof(WCHAR));
    return lsaString;
}


static BOOL ObtainLockPagesPrivilege() {
    HANDLE token;
    PTOKEN_USER user = NULL;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token) == TRUE) {
        DWORD size = 0;

        GetTokenInformation(token, TokenUser, NULL, 0, &size);
        if (size) {
            user = (PTOKEN_USER) LocalAlloc(LPTR, size);
        }

        GetTokenInformation(token, TokenUser, user, size, &size);
        CloseHandle(token);
    }

    if (!user) {
        return FALSE;
    }

    LSA_HANDLE handle;
    LSA_OBJECT_ATTRIBUTES attributes;
    ZeroMemory(&attributes, sizeof(attributes));

    BOOL result = FALSE;
    if (LsaOpenPolicy(NULL, &attributes, POLICY_ALL_ACCESS, &handle) == 0) {
        LSA_UNICODE_STRING str = StringToLsaUnicodeString(_T(SE_LOCK_MEMORY_NAME));

        if (LsaAddAccountRights(handle, user->User.Sid, &str, 1) == 0) {
            applog_notime(LOG_WARNING, "Huge pages support was successfully enabled, but reboot required to use it");
            result = TRUE;
        }

        LsaClose(handle);
    }

    LocalFree(user);
    return result;
}


static BOOL TrySetLockPagesPrivilege() {
    if (SetLockPagesPrivilege()) {
        return TRUE;
    }

    return ObtainLockPagesPrivilege() && SetLockPagesPrivilege();
}


const char * persistent_memory_allocate() {
    const int ratio = (opt_double_hash && opt_algo != ALGO_CRYPTONIGHT_LITE) ? 2 : 1;
    const int size  = MEMORY * (opt_n_threads * ratio + 1);

    if (TrySetLockPagesPrivilege()) {
        persistent_memory_flags |= MEMORY_HUGEPAGES_AVAILABLE;
    }

    persistent_memory = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE | MEM_LARGE_PAGES, PAGE_READWRITE);
    if (!persistent_memory) {
        persistent_memory = _mm_malloc(size, 16);
    }
    else {
        persistent_memory_flags |= MEMORY_HUGEPAGES_ENABLED;
    }

    return persistent_memory;
}


void persistent_memory_free() {
    if (persistent_memory_flags & MEMORY_HUGEPAGES_ENABLED) {
        VirtualFree(persistent_memory, 0, MEM_RELEASE);
    }
    else {
        _mm_free(persistent_memory);
    }
}


void *allocate_executable_memory(size_t size)
{
    return VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
}


void protect_executable_memory(void *p, size_t size)
{
    DWORD oldProtect;
    VirtualProtect(p, size, PAGE_EXECUTE_READ, &oldProtect);
}


void flush_instruction_cache(void *p, size_t size)
{
    FlushInstructionCache(GetCurrentProcess(), p, size);
}
