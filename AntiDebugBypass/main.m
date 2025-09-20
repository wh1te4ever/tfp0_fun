#include <stdio.h>
#include <dlfcn.h>
#include "krw.h"

void *libjb = NULL;

kern_return_t
init_kernrw(void) {
	libjb = dlopen("/var/jb/basebin/libjailbreak.dylib", RTLD_NOW);

	void *libjb_jbdInitPPLRW = dlsym(libjb, "jbdInitPPLRW");
	int (*jbdInitPPLRW)(void) = libjb_jbdInitPPLRW;
	int ret = jbdInitPPLRW();
	if(ret != 0) {
		printf("[-] jbdInitPPLRW failed, ret: %d", ret);
		return KERN_FAILURE;
	}
	
	return KERN_SUCCESS;
}

uint64_t proc_find(pid_t pidToFind) {
    void *sym = dlsym(libjb, "proc_find");
    uint64_t (*_proc_find)(pid_t) = (uint64_t (*)(pid_t))sym;
    return _proc_find(pidToFind);
}

// Thanks pattern-f; https://github.com/pattern-f/TQ-pre-jailbreak/blob/main/exploit-main/post_exploit.c#L33
#define PROC_ALL_PIDS        1
extern int proc_listpids(uint32_t type, uint32_t typeinfo, void *buffer, int buffersize);
extern int proc_pidpath(int pid, void * buffer, uint32_t  buffersize);

pid_t look_for_proc_internal(const char *name, bool (^match)(const char *path, const char *want))
{
    pid_t *pids = calloc(1, 3000 * sizeof(pid_t));
    int procs_cnt = proc_listpids(PROC_ALL_PIDS, 0, pids, 3000);
    if(procs_cnt > 3000) {
        pids = realloc(pids, procs_cnt * sizeof(pid_t));
        procs_cnt = proc_listpids(PROC_ALL_PIDS, 0, pids, procs_cnt);
    }
    int len;
    char pathBuffer[4096];
    for (int i=(procs_cnt-1); i>=0; i--) {
        if (pids[i] == 0) {
            continue;
        }
        memset(pathBuffer, 0, sizeof(pathBuffer));
        len = proc_pidpath(pids[i], pathBuffer, sizeof(pathBuffer));
        if (len == 0) {
            continue;
        }
        if (match(pathBuffer, name)) {
            free(pids);
            return pids[i];
        }
    }
    free(pids);
    return 0;
}

pid_t look_for_proc_basename(const char *base_name)
{
    return look_for_proc_internal(base_name, ^bool (const char *path, const char *want) {
        const char *base = path;
        const char *last = strrchr(path, '/');
        if (last) {
            base = last + 1;
        }
        if (!strcmp(base, want)) {
            return true;
        }
        return false;
    });
}

#define ISSET(t, f)     ((t) & (f))
#define CLR(t, f)       (t) &= ~(f)
#define SET(t, f)       (t) |= (f)

int main(int argc, char *argv[], char *envp[]) {
	kern_return_t kr;
	kr = init_kernrw();
	if(kr != KERN_SUCCESS) exit(1);

	// 1. Disable ASLR
	uint64_t launchdPid = 1;
	uint64_t launchdProc = proc_find(launchdPid);
	printf("launchd proc = 0x%llx\n", launchdProc);

	#define P_DISABLE_ASLR  0x00001000      /* Disable address space layout randomization */
	uint32_t off_p_flag = 0x264;
	uint32_t p_flag = kread32(launchdProc + off_p_flag);
	SET(p_flag, P_DISABLE_ASLR);
	kwrite32(launchdProc + off_p_flag, p_flag);


	// 2. Bypass Anti-Debugging
	pid_t runnerPid = look_for_proc_basename("Runner");
	printf("runnerPid = %u\n", runnerPid);
	uint64_t runnerProc = proc_find(runnerPid);
	printf("runnerProc = 0x%llx\n", runnerProc);

	uint32_t off_p_lflag = 0x268;
	uint32_t p_lflag = kread32(runnerProc + off_p_lflag);

#define P_LNOATTACH     0x00001000 
#define P_LTRACED       0x00000400
	CLR(p_lflag, P_LNOATTACH);
	CLR(p_lflag, P_LTRACED);
	kwrite32(runnerProc + off_p_lflag, p_lflag);

	return 0;
}
