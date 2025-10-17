// derived from https://github.com/hisptoot/BambuSource2Raw/blob/main/BambuSource2Raw/bambusource2raw.cpp
#define BAMBU_DYNAMIC

#include <stdio.h>
#include "BambuTunnel.h"
#include <unistd.h>
#include <dlfcn.h>
#include <cstdlib>
#include <limits.h>
#include <string.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define BAMBUE_START_STREAM_RETRY_COUNT (40)

struct BambuLib lib = {0};
static void* module = NULL;

static void* get_function(const char* name)
{
    void* function = NULL;

    if (!module)
    {
        return function;
    }

#if defined(_MSC_VER) || defined(_WIN32)
    function = (void *)GetProcAddress(module, name);
#else
    function = (void *)dlsym(module, name);
#endif

    if (!function) 
    {
        fprintf(stderr, ", can not find function %s", name);
        exit(-1);
    }
    return function;
}

#define GET_FUNC(x) *((void **)&lib.x) = get_function(#x)

void bambu_log(void *ctx, int level, tchar const * msg)
{
    if (level <= 1)
    {
#if defined(_MSC_VER) || defined(_WIN32)
      fwprintf(stderr, L"[%d] %s\n", level, msg);
#else
      fprintf(stderr, "[%d] %s\n", level, msg);
#endif
      lib.Bambu_FreeLogMsg(msg);
    }
}

int start_bambu_stream(char *camera_url)
{
    Bambu_Tunnel tunnel = NULL;
    int is_bambu_open = 0;
    int ret = 0;

    do {
        fprintf(stderr, "Starting Session\n");

        ret = lib.Bambu_Create(&tunnel, camera_url);
        if (ret != 0)
        {
            fprintf(stderr, "Bambu_Create failed 0x%x\n", ret);
            break;
        }

        lib.Bambu_SetLogger(tunnel, bambu_log, tunnel);

        ret = lib.Bambu_Open(tunnel);
        if (ret != 0)
        {
            fprintf(stderr, "Bambu_Open failed: 0x%x\n", ret);
            break;
        }
        is_bambu_open++;

        size_t i;
        for (i = 0; i < BAMBUE_START_STREAM_RETRY_COUNT; i++)
        {
            ret = lib.Bambu_StartStream(tunnel, true);
            //fprintf(stderr, "Bambu_StartStream ret: 0x%x\n", ret);

            if (ret == 0)
            {
                break;
            }

#if defined(_MSC_VER) || defined(_WIN32)
            Sleep(1000);
#else
            usleep(1000 * 1000);
#endif
        }

        if (ret != 0)
        {
            fprintf(stderr, "Bambu_StartStream failed 0x%x\n", ret);
            break;
        }

        int result = 0;
        while (true) 
        {
            Bambu_Sample sample;
            result = lib.Bambu_ReadSample(tunnel, &sample);

            if (result == Bambu_success) 
            {
                fwrite(sample.buffer, 1, sample.size, stdout);
                fflush(stdout);
                continue;
            } 
            else if (result == Bambu_would_block) 
            {
#if defined(_MSC_VER) || defined(_WIN32)
                Sleep(100);
#else
                usleep(100 * 1000);
#endif
                continue;
            } 
            else if (result == Bambu_stream_end) 
            {
                fprintf(stderr, "Bambu_stream_end\n");
                result = 0;
            } 
            else 
            {
                result = -1;
                fprintf(stderr, "ERROR_PIPE\n");
                ret = -1;
            }
            break;
        }
    } while (false);

    if (is_bambu_open)
    {
        lib.Bambu_Close(tunnel);
    }

    if (tunnel != NULL)
    {
        lib.Bambu_Destroy(tunnel);
        tunnel = NULL;
    }

    return ret;
}

int main(int argc, char* argv[]){
    if (argc > 4) {
        fprintf(stderr, "Usage: %s [<libBambuSource.so path> [<printer address> [<access code>]]]\n", argv[0]);
        return 1;
    }

    char defaultLibPath[PATH_MAX] = "./libBambuSource.so";
    char resolvedExecPath[PATH_MAX];
    if (realpath(argv[0], resolvedExecPath) != NULL) {
        char* lastSlash = strrchr(resolvedExecPath, '/');
        if (lastSlash) {
            *(lastSlash + 1) = '\0';
            snprintf(defaultLibPath, sizeof(defaultLibPath), "%slibBambuSource.so", resolvedExecPath);
        }
    }

    const char* bambuLibPath = defaultLibPath;
    const char* printerAddress = getenv("PRINTER_ADDRESS");
    const char* accessCode = getenv("PRINTER_ACCESS_CODE");

    if (argc >= 2 && argv[1][0] != '\0') {
        bambuLibPath = argv[1];
    }
    if (argc >= 3 && argv[2][0] != '\0') {
        printerAddress = argv[2];
    }
    if (argc >= 4 && argv[3][0] != '\0') {
        accessCode = argv[3];
    }

    if (!bambuLibPath || bambuLibPath[0] == '\0') {
        fprintf(stderr, "Unable to resolve libBambuSource.so path. Provide it explicitly as the first argument.\n");
        return 1;
    }
    if (!printerAddress || printerAddress[0] == '\0') {
        fprintf(stderr, "Printer address is required. Supply PRINTER_ADDRESS or pass it as the second argument.\n");
        return 1;
    }
    if (!accessCode || accessCode[0] == '\0') {
        fprintf(stderr, "Access code is required. Supply PRINTER_ACCESS_CODE or pass it as the third argument.\n");
        return 1;
    }

    fprintf(stderr, "Starting Bambu Camera Tunnel\n");
    fprintf(stderr, "  libBambuSource.so path: %s\n", bambuLibPath);
    fprintf(stderr, "  printerAddress: %s\n", printerAddress);
    fprintf(stderr, "  accessCode: %s\n\n", accessCode);

    char camera_url[256];

    snprintf(camera_url, 256, "bambu:///local/%s.?port=6000&user=bblp&passwd=%s", printerAddress, accessCode);

    module = dlopen(bambuLibPath, RTLD_LAZY);
	if (module == NULL)
    {
        fprintf(stderr, "Failed loading libBambuSource.so at path %s\n", bambuLibPath);
        return -1;
    }
	GET_FUNC(Bambu_Init);
    GET_FUNC(Bambu_Deinit);
    GET_FUNC(Bambu_Create);
    GET_FUNC(Bambu_Destroy);
    GET_FUNC(Bambu_Open);
    GET_FUNC(Bambu_StartStream);
    GET_FUNC(Bambu_SendMessage);
    GET_FUNC(Bambu_ReadSample);
    GET_FUNC(Bambu_Close);
    GET_FUNC(Bambu_SetLogger);
    GET_FUNC(Bambu_FreeLogMsg);
    GET_FUNC(Bambu_GetLastErrorMsg);
    GET_FUNC(Bambu_GetStreamCount);
    GET_FUNC(Bambu_GetDuration);
    GET_FUNC(Bambu_GetStreamInfo);

	start_bambu_stream(camera_url);

	return 0;
}
