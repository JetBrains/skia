// Copyright 2019 Google LLC.
// Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.

#include "SkLoadICU.h"

#if defined(_WIN32) && defined(SK_USING_THIRD_PARTY_ICU)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <io.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <sstream>

#include "unicode/udata.h"

static void* win_mmap(const std::wstring& dataFile, std::wostringstream& buffer) {
    struct FCloseWrapper { void operator()(FILE* f) { fclose(f); } };
    std::unique_ptr<FILE, FCloseWrapper> stream(_wfopen(dataFile.c_str(), L"rb"));
    if (!stream) {
        buffer << L"SkLoadICU: datafile '" << dataFile << L"' is missing" << std::endl;
        return nullptr;
    }
    int fileno = _fileno(stream.get());
    if (fileno < 0) {
        buffer << L"SkLoadICU: datafile '" << dataFile << L"' fileno error " << fileno << std::endl;
        return nullptr;
    }
    HANDLE file = (HANDLE)_get_osfhandle(fileno);
    if ((HANDLE)INVALID_HANDLE_VALUE == file) {
        buffer << L"SkLoadICU: datafile '" << dataFile << L"' handle error" << std::endl;
        return nullptr;
    }
    struct CloseHandleWrapper { void operator()(HANDLE h) { CloseHandle(h); } };
    std::unique_ptr<void, CloseHandleWrapper> mmapHandle(
        CreateFileMapping(file, nullptr, PAGE_READONLY, 0, 0, nullptr));
    if (!mmapHandle) {
        buffer << L"SkLoadICU: datafile '" << dataFile << L"' mmap error" << std::endl;
        return nullptr;
    }
    void* addr = MapViewOfFile(mmapHandle.get(), FILE_MAP_READ, 0, 0, 0);
    if (nullptr == addr) {
        buffer << L"SkLoadICU: datafile '" << dataFile << L"' view error" << std::endl;
        return nullptr;
    }
    return addr;
}

static bool init_icu(void* addr) {
    UErrorCode err = U_ZERO_ERROR;
    udata_setCommonData(addr, &err);
    if (err != U_ZERO_ERROR) {
        fprintf(stderr, "udata_setCommonData() returned %d.\n", (int)err);
        return false;
    }
    udata_setFileAccess(UDATA_ONLY_PACKAGES, &err);
    if (err != U_ZERO_ERROR) {
        fprintf(stderr, "udata_setFileAccess() returned %d.\n", (int)err);
        return false;
    }
    return true;
}

static std::wstring get_module_path(HMODULE module) {
    DWORD len;
    std::wstring path;
    path.resize(MAX_PATH);

    len = GetModuleFileNameW(module, (LPWSTR)path.data(), (DWORD)path.size());
    if (len > path.size()) {
        path.resize(len);
        len = GetModuleFileNameW(module, (LPWSTR)path.data(), (DWORD)path.size());
    }
    path.resize(len);
    std::size_t end = path.rfind('\\');
    if (end == std::wstring::npos) {
        return std::wstring();
    }
    path.resize(end);
    return path;
}

static std::wstring library_directory() {
    HMODULE hModule = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        reinterpret_cast<LPCSTR>(&library_directory), &hModule);
    return get_module_path(hModule);
}

static std::wstring executable_directory() {
    HMODULE hModule = GetModuleHandleA(NULL);
    return get_module_path(hModule);
}

bool SkLoadICU() {
    static bool loaded = false;
    static std::once_flag flag;
    std::call_once(flag, []() {
        std::wostringstream buffer;
        void* addr = win_mmap(library_directory() + L"\\icudtl.dat", buffer);
        if (addr == nullptr)
            addr = win_mmap(executable_directory() + L"\\icudtl.dat", buffer);
        if (addr == nullptr)
            std::wcerr << buffer.str() << std::endl;
        if (addr)
            loaded = init_icu(addr);
    });
    return loaded;
}

#endif  // defined(_WIN32) && defined(SK_USING_THIRD_PARTY_ICU)
