#include "md5_util.h"

#include <windows.h>
#include <Wincrypt.h>

struct HandlesHolder {
public:
    HandlesHolder() : hProv(0), hHash(0), hFile(nullptr) {};

    ~HandlesHolder() {
        CryptReleaseContext(hProv, 0);
        CryptDestroyHash(hHash);
        if (hFile) CloseHandle(hFile);
    }

    HCRYPTPROV hProv;
    HCRYPTHASH hHash;
    HANDLE hFile;
};

// https://docs.microsoft.com/en-gb/windows/win32/seccrypto/example-c-program--creating-an-md-5-hash-from-file-content
std::string calculate_md5_of_file(const std::string &filename) {
    const int BUFSIZE = 1024;
    const int MD5LEN = 16;

    DWORD dwStatus = 0;
    BOOL bResult = FALSE;

    HandlesHolder handlesHolder;

    BYTE rgbFile[BUFSIZE];
    DWORD cbRead = 0;
    BYTE rgbHash[MD5LEN];
    DWORD cbHash = 0;
    CHAR rgbDigits[] = "0123456789abcdef";
    // Logic to check usage goes here.

    handlesHolder.hFile = CreateFileA(filename.c_str(),
                                      GENERIC_READ,
                                      FILE_SHARE_READ,
                                      NULL,
                                      OPEN_EXISTING,
                                      FILE_FLAG_SEQUENTIAL_SCAN,
                                      NULL);

    if (INVALID_HANDLE_VALUE == handlesHolder.hFile) {
        // dwStatus = GetLastError();
        // printf("Error opening file %s\nError: %lu\n", filename.c_str(), dwStatus);
        return "";
    }

    // Get handle to the crypto provider
    if (!CryptAcquireContext(&handlesHolder.hProv,
                             NULL,
                             NULL,
                             PROV_RSA_FULL,
                             CRYPT_VERIFYCONTEXT)) {
        // dwStatus = GetLastError();
        // printf("CryptAcquireContext failed: %d\n", dwStatus);
        return "";
    }

    if (!CryptCreateHash(handlesHolder.hProv, CALG_MD5, 0, 0, &handlesHolder.hHash)) {
        // dwStatus = GetLastError();
        // printf("CryptAcquireContext failed: %d\n", dwStatus);
        return "";
    }

    while ((bResult = ReadFile(handlesHolder.hFile, rgbFile, BUFSIZE,
                               &cbRead, NULL))) {
        if (0 == cbRead) {
            break;
        }

        if (!CryptHashData(handlesHolder.hHash, rgbFile, cbRead, 0)) {
            // dwStatus = GetLastError();
            // printf("CryptHashData failed: %d\n", dwStatus);
            return "";
        }
    }

    if (!bResult) {
        // dwStatus = GetLastError();
        // printf("ReadFile failed: %d\n", dwStatus);
        return "";
    }

    cbHash = MD5LEN;
    std::string md5_hash;
    if (CryptGetHashParam(handlesHolder.hHash, HP_HASHVAL, rgbHash, &cbHash, 0)) {
        printf("MD5 hash of file %s is: ", filename.c_str());
        char buffer[512];
        int offset = 0;
        for (DWORD i = 0; i < cbHash; i++) {
            offset += sprintf(buffer+offset, "%c%c", rgbDigits[rgbHash[i] >> 4], rgbDigits[rgbHash[i] & 0xf]);
            printf("%c%c", rgbDigits[rgbHash[i] >> 4], rgbDigits[rgbHash[i] & 0xf]);
        }
        printf("\n");
        md5_hash = std::string(buffer);
    } else {
        // dwStatus = GetLastError();
        // printf("CryptGetHashParam failed: %d\n", dwStatus);
    }

    return md5_hash;
}