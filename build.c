// no cross compilation, guess build based on host
// NOTE: windows implementation untested
#ifndef FUNBUILD_NO_GUESS_TARGET
#ifdef __WIN32__
#define FUNBUILD_TARGET_WINDOWS
#else
#ifdef __linux__
#define FUNBUILD_TARGET_LINUX
#endif
#endif
#endif

// platform specific includes
#ifdef FUNBUILD_TARGET_WINDOWS
#include <windows.h>
#elif defined(FUNBUILD_TARGET_LINUX)
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#elif defined(FUNBUILD_TARGET_MAC)
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

/// Returns zero on failure and one on success
int spawn_process(const char* exe, int argc, const char** argv);

int spawn_process(const char* exe, int argc, const char** argv)
{
#ifdef FUNBUILD_TARGET_WINDOWS
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	char cmdline[1024];
	cmdline[0] = '\0';
	for (int i = 0; i < argc; ++i) {
		strcat(cmdline, argv[i]);
		if (i < argc - 1) {
			strcat(cmdline, " ");
		}
	}

	if (!CreateProcess(exe,		// Application name
					   cmdline, // Command line arguments
					   NULL,	// Process security attributes
					   NULL,	// Thread security attributes
					   FALSE,	// Inherit handles
					   0,		// Creation flags
					   NULL,	// Environment variables
					   NULL,	// Current directory
					   &si,		// Startup info
					   &pi)		// Process information
	) {
		return;
	}
	// TODO: can we just pass null for pi and si
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return 0;

#elif defined(FUNBUILD_TARGET_LINUX) || defined(FUNBUILD_TARGET_MAC)
	pid_t pid = fork();

	if (pid == -1) {
		return 1; // fork failed
	}

	if (pid == 0) {
		execvp(exe, (char* const*)argv);
		// OS error trying to execvp
		return 1;
	}

	return 0;
#else
#error "Unsupported platform"
#endif
}
