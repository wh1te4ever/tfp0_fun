#include "IOSurfaceRoot.h"

#define vm_real_kernel_page_size get_kernel_page_size()
io_connect_t IOSurfaceRootUserClient = MACH_PORT_NULL;

vm_size_t get_kernel_page_size() {
    vm_size_t kernel_page_size = 0;
    vm_size_t *out_page_size = NULL;
    host_t host = mach_host_self();
    if (!MACH_PORT_VALID(host)) goto out;
    out_page_size = (vm_size_t *)malloc(sizeof(vm_size_t));
    if (out_page_size == NULL) goto out;
    bzero(out_page_size, sizeof(vm_size_t));
    if (_host_page_size(host, out_page_size) != KERN_SUCCESS) goto out;
    kernel_page_size = *out_page_size;
out:
    if (MACH_PORT_VALID(host)) mach_port_deallocate(mach_task_self(), host); host = HOST_NULL;
    if(out_page_size != NULL) free(out_page_size);
    return kernel_page_size;
}


void IOSurfaceRoot_init(void)
{
    io_service_t IOSurfaceRoot = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("IOSurfaceRoot"));
    if (IOSurfaceRoot == MACH_PORT_NULL)
    {
        printf("Couldn't find IOSurfaceRoot.\n");   exit(1);
    }
    
    if (IOServiceOpen(IOSurfaceRoot, mach_task_self(), 0, &IOSurfaceRootUserClient) != KERN_SUCCESS)
    {
        printf("Could not open IOSurfaceRootUserClient.\n");   exit(1);
    }
}

uint32_t iosurface_s_create_surface_fast_path(void)
{
    // Brandon Azad's definitions (iosurface.c in oob_timestamp-pac.zip) from https://bugs.chromium.org/p/project-zero/issues/detail?id=1986#c4
    //
    // Estimated struct src: /Library/Developer/KDKs/KDK_13.0_22A380.kdk/System/Library/Extensions/IOSurface.kext
    // sub_FFFFFFF005FEFFA8 6s 14.4.2; 0xf60, 0x18 struct hint
    //
    // https://gist.github.com/jmpews/01b520993a742f72714cd06e40793eed

    struct _IOSurfaceFastCreateArgs {
        uint64_t address;
        uint32_t width;
        uint32_t height;
        uint32_t pixel_format;
        uint32_t bytes_per_element;
        uint32_t bytes_per_row;
        uint32_t alloc_size;
    };

    struct IOSurfaceLockResult {
        uint8_t _pad1[0x18];
        uint32_t surface_id;
        uint8_t _pad2[0xF60-0x18-0x4];
    };
    
    struct _IOSurfaceFastCreateArgs create_args = { .alloc_size = (uint32_t)vm_real_kernel_page_size };
    struct IOSurfaceLockResult lock_result = {0};
    uint64_t lock_result_size = sizeof(lock_result);
    
    //KDK_13.0_22A380.kdk
    //0 __const:000000000002B700 __ZN23IOSurfaceRootUserClient12sMethodDescsE dq offset __ZN23IOSurfaceRootUserClient16s_create_surfaceEPS_PvP25IOExternalMethodArguments
    //1 __const:000000000002B728                 dq offset __ZN23IOSurfaceRootUserClient17s_release_surfaceEPS_PvP25IOExternalMethodArguments 
    //2 __const:000000000002B750                 dq offset __ZN23IOSurfaceRootUserClient14s_lock_surfaceEPS_PvP25IOExternalMethodArguments
    //3 __const:000000000002B778                 dq offset __ZN23IOSurfaceRootUserClient16s_unlock_surfaceEPS_PvP25IOExternalMethodArguments
    //4 __const:000000000002B7A0                 dq offset __ZN23IOSurfaceRootUserClient16s_lookup_surfaceEPS_PvP25IOExternalMethodArguments
    //5 __const:000000000002B7C8                 dq offset __ZN23IOSurfaceRootUserClient17s_set_ycbcrmatrixEPS_PvP25IOExternalMethodArguments
    //6v__const:000000000002B7F0                 dq offset __ZN23IOSurfaceRootUserClient26s_create_surface_fast_pathEPS_PvP25IOExternalMethodArguments

    //iphone 8 14.4.2
    //0 __DATA_CONST:__const:FFFFFFF007866D28 80 63 42 08 F0 FF FF FF off_FFFFFFF007866D28 DCQ sub_FFFFFFF008426380
    //1 __DATA_CONST:__const:FFFFFFF007866D40 60 64 42 08 F0 FF FF FF                 DCQ sub_FFFFFFF008426460
    //2 __DATA_CONST:__const:FFFFFFF007866D58 6C 64 42 08 F0 FF FF FF                 DCQ sub_FFFFFFF00842646C
    //3 __DATA_CONST:__const:FFFFFFF007866D70 84 64 42 08 F0 FF FF FF                 DCQ sub_FFFFFFF008426484
    //4 __DATA_CONST:__const:FFFFFFF007866D88 9C 64 42 08 F0 FF FF FF                 DCQ sub_FFFFFFF00842649C
    //5 __DATA_CONST:__const:FFFFFFF007866DA0 AC 64 42 08 F0 FF FF FF                 DCQ sub_FFFFFFF0084264AC
    //6 __DATA_CONST:__const:FFFFFFF007866DB8 BC 64 42 08 F0 FF FF FF                 DCQ sub_FFFFFFF0084264BC

    //that's why IOConnectCallMethod's selector is 6.

    IOConnectCallMethod(
            IOSurfaceRootUserClient,
            6,
            NULL, 0,
            &create_args, sizeof(create_args),
            NULL, NULL,
            &lock_result, (size_t *)&lock_result_size);
    
    return lock_result.surface_id;
}

uint32_t iosurface_s_get_ycbcrmatrix(void)
{
    uint64_t i_scalar[1] = { 1 }; // fixed, first valid client obj
    uint64_t o_scalar[1];
    uint32_t i_count = 1;
    uint32_t o_count = 1;

    kern_return_t kr = IOConnectCallMethod(
            IOSurfaceRootUserClient,
            8, // s_get_ycbcrmatrix
            i_scalar, i_count,
            NULL, 0,
            o_scalar, &o_count,
            NULL, NULL);
    if (kr != KERN_SUCCESS) {
        printf("s_get_ycbcrmatrix error: 0x%x", kr);
        return 0;
    }
    return (uint32_t)o_scalar[0];
}

void iosurface_s_set_indexed_timestamp(uint64_t v)
{
    uint64_t i_scalar[3] = {
        1, // fixed, first valid client obj
        0, // index
        v, // value
    };
    uint32_t i_count = 3;

    kern_return_t kr = IOConnectCallMethod(
            IOSurfaceRootUserClient,
            33, // s_set_indexed_timestamp
            i_scalar, i_count,
            NULL, 0,
            NULL, NULL,
            NULL, NULL);
    if (kr != KERN_SUCCESS) {
        printf("s_set_indexed_timestamp error: 0x%x", kr);
    }
}

void IOSurfaceRoot_exit()
{
    IOServiceClose(IOSurfaceRootUserClient);
}