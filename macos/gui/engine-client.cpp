#include "engine-client.h"
#include "state-validation.h"
#include "../engine/audio-devices.h"
#include <cmath>
namespace fxgui {
juce::var commandObject(const juce::String& command) {
    auto* object=new juce::DynamicObject(); object->setProperty("version",1); object->setProperty("command",command); return juce::var(object);
}
juce::File EngineClient::bundle() {
    return juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory().getParentDirectory();
}
EngineClient::EngineClient():Thread("FxSound audio control") {
    try { socket_=fxsound::defaultSocketPath(); } catch(...) {}
}
EngineClient::~EngineClient() { deactivate(); }
void EngineClient::deliver(Event event) {
    auto callback=onEvent;
    if(callback) juce::MessageManager::callAsync([callback,event]{callback(event);});
}
void EngineClient::begin(const juce::String& output,const juce::String& preset) {
    auto message=commandObject("start");message.getDynamicObject()->setProperty("outputUID",output);message.getDynamicObject()->setProperty("preset",preset);
    juce::ScopedLock guard(lock_);commands_.emplace_back("start",message);notify();
}
void EngineClient::send(const juce::String& command,const juce::String& key,juce::var value) {
    auto object=commandObject(command);
    if(key.isNotEmpty()) object.getDynamicObject()->setProperty(key,value);
    juce::ScopedLock guard(lock_); commands_.emplace_back(command,object); notify();
}
void EngineClient::parameter(const juce::String& name,double value) {
    juce::ScopedLock guard(lock_); parameters_[name]=value; notify();
}
void EngineClient::finish() { send("finish"); }
juce::var EngineClient::request(juce::var object) {
    if(socket_.isEmpty()) throw std::runtime_error("Private audio connection unavailable.");
    try {
        auto result=juce::JSON::parse(fxsound::IPCServer::request(socket_.toStdString(),juce::JSON::toString(object,true).toStdString()));
        auto version=result["version"];
        if(!result.isObject() || !(version.isInt() || version.isInt64()) || juce::int64(version)!=1 || !result["ok"].isBool())
            throw std::runtime_error("Invalid response");
        return result;
    } catch(...) { throw std::runtime_error("Couldn't connect to audio. Choose Retry."); }
}
void EngineClient::execute(const juce::String& name,const juce::var& sent) {
    try {
        if(name=="start") {
            outputUID_=sent["outputUID"].toString();
            try { auto state=request(commandObject("getState")); if(bool(state["ok"])) return; } catch(...) {}
            auto helpers=bundle().getChildFile("Helpers");
            auto engine=helpers.getChildFile("fxsound-engine"),supervisor=helpers.getChildFile("fxsound-supervisor");
            if(!engine.existsAsFile() || !supervisor.existsAsFile()) throw std::runtime_error("Audio service missing. Reinstall FxSound.");
            juce::StringArray args{supervisor.getFullPathName(),"--engine",engine.getFullPathName(),"--output",sent["outputUID"].toString()};
            auto session=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Application Support/FxSound/user-session.fac");
            auto selected=sent["preset"].toString();auto fallback=selected.isNotEmpty()?juce::File(selected):bundle().getChildFile("Resources/Factsoft/Default.fac");
            auto initial=session.existsAsFile()?session:fallback;
            if(initial.existsAsFile()){args.add("--preset");args.add(initial.getFullPathName());}
            args.add("--fallback-preset");args.add(bundle().getChildFile("Resources/Factsoft/Default.fac").getFullPathName());
            if(!supervisor_.start(args,0)) throw std::runtime_error("Could not start audio. Try again.");
            return;
        }
        if(name=="finish") {
            bool restored=false;
            try { auto message=commandObject("activateRouting"); message.getDynamicObject()->setProperty("value",false);
                restored=bool(request(message)["ok"]); } catch(...) {}
            if(restored) { auto result=request(commandObject("shutdown"));
                if(!bool(result["ok"])) throw std::runtime_error("Could not stop audio safely. Try again."); }
            else {
                auto helper=bundle().getChildFile("Helpers/fxsound-supervisor");
                if(supervisor_.isRunning()) throw std::runtime_error("Audio is still running. Retry before quitting.");
                if(helper.existsAsFile() && outputUID_.isNotEmpty()) {
                    juce::ChildProcess recovery;
                    if(!recovery.start(juce::StringArray{helper.getFullPathName(),"--restore-output",outputUID_},0)
                       || !recovery.waitForProcessToFinish(3000) || recovery.getExitCode()!=0)
                        throw std::runtime_error("Choose your speakers in Sound Settings before quitting.");
                }
            }
            finishing_=true; deliver({name,{},sent,true,{}}); return;
        }
        auto result=request(sent); bool ok=bool(result["ok"]);
        if(name=="getState" && ok && !validState(result["data"]))throw std::runtime_error("Audio needs to reconnect. Choose Retry.");
        deliver({name,result["data"],sent,ok,ok?juce::String():juce::String("That change couldn't be applied. Try again.")});
        if(ok && (name=="setParams" || name=="setPreset" || name=="setNumEqBands")) {
            auto session=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Application Support/FxSound/user-session.fac");
            if(session.getParentDirectory().createDirectory().wasOk()) {
                auto save=commandObject("savePreset");save.getDynamicObject()->setProperty("path",session.getFullPathName());
                try { if(!bool(request(save)["ok"]))deliver({"sessionSave",{}, {},false,"Your sound changed, but couldn't be saved for next time."}); }
                catch(...) {deliver({"sessionSave",{}, {},false,"Your sound changed, but couldn't be saved for next time."});}
            }
        }
    } catch(const std::exception& error) { deliver({name,{},sent,false,error.what()}); }
}
void EngineClient::run() {
    auto lastState=juce::Time::getMillisecondCounter(),lastDevices=lastState-2500,lastCommit=lastState;
    while(!threadShouldExit()) {
        auto now=juce::Time::getMillisecondCounter();
        std::deque<std::pair<juce::String,juce::var>> commands;
        std::map<juce::String,double> parameters;
        { juce::ScopedLock guard(lock_); commands.swap(commands_); if(now-lastCommit>=30 || !commands.empty()){parameters.swap(parameters_);lastCommit=now;} }
        if(!parameters.empty() && !finishing_) {
            auto message=commandObject("setParams"); auto* values=new juce::DynamicObject();
            for(const auto& entry:parameters) values->setProperty(entry.first,entry.second);
            message.getDynamicObject()->setProperty("params",juce::var(values)); execute("setParams",message);
        }
        for(auto& entry:commands) { if(threadShouldExit()) break; execute(entry.first,entry.second); }
        if(!finishing_ && now-lastDevices>=2000) {
            try { auto list=juce::JSON::parse(fxsound::devicesJSON());auto current=fxsound::deviceUID(fxsound::defaultOutput());
                for(auto& device:*list.getArray())device.getDynamicObject()->setProperty("default",device["uid"].toString().toStdString()==current);
                deliver({"devices",list,{},true,{}}); }
            catch(...) { deliver({"devices",{}, {},false,"Couldn't find your speakers. Try again."}); }
            lastDevices=now;
        }
        if(!finishing_ && now-lastState>=500) { execute("getState",commandObject("getState")); lastState=now; }
        wait(30);
    }
}
}
