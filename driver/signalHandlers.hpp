#pragma once

extern void stopMain(bool backtrace=true);

extern void terminate_handler(bool backtrace);

// Installs signal handlers for the current thread.
extern void installSignalHandlers();
