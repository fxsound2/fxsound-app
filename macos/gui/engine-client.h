#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <deque>
#include <map>
#include "../engine/ipc-server.h"
namespace fxgui {
struct Event { juce::String kind; juce::var data,sent; bool ok=false; juce::String error; };
class EngineClient : private juce::Thread {
public:
    EngineClient();
    ~EngineClient() override;
    void activate() { startThread(); }
    void deactivate() { signalThreadShouldExit(); notify(); stopThread(10000); }
    void begin(const juce::String& output,const juce::String& preset={});
    void send(const juce::String& command,const juce::String& key={},juce::var value={});
    void parameter(const juce::String& name,double value);
    void finish();
    std::function<void(Event)> onEvent;
    static juce::File bundle();
private:
    void run() override;
    void deliver(Event);
    juce::var request(juce::var);
    void execute(const juce::String&,const juce::var&);
    juce::CriticalSection lock_;
    std::deque<std::pair<juce::String,juce::var>> commands_;
    std::map<juce::String,double> parameters_;
    juce::ChildProcess supervisor_;
    juce::String socket_,outputUID_;
    bool finishing_=false;
};
juce::var commandObject(const juce::String& command);
}
