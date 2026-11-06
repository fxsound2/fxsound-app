/*
FxSound Linux port — linux/single_instance_posix.h (Phase 8)
JUCE 6.1.6 Linux single-instance CLI forwarding is unimplemented
(MessageManager::broadcastMessage is a TODO stub). UNIX-socket replacement;
see the .cpp header comment. Windows keeps the JUCE/Win32 mechanism.
*/
#pragma once
#include <functional>
#include <string>

namespace fxsound_linux {
// Try to deliver commandLine to a running primary. true = delivered (caller
// should quit); false = no primary (caller becomes the primary).
bool forwardIfRunning(const std::string& commandLine);

// Become the primary: start the accept loop. onCommand is invoked from the
// accept thread; marshal into the JUCE message thread at the call site.
void startServer(std::function<void(const std::string&, std::function<void()> done)> onCommand);

// Stop the accept loop and remove the socket.
void stopServer();
}  // namespace fxsound_linux
