#pragma once

extern void stopMain();

extern void ctrlC(int s, siginfo_t *si, void *arg);

extern void terminate_handler(bool backtrace);

// Installs signal handlers for the current thread.
extern void installSignalHandlers();
