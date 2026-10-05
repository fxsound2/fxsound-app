#include "mac-controller.h"
#include "FxMainWindow.h"
#include "FxMessage.h"
#include "audio-permission.h"
#include <cmath>
FxController::FxController() {
 effects_.insertMultiple(0,0.0f,5);gains_.insertMultiple(0,0.0f,10);
 const float defaults[]={62.5f,115.734f,214.311f,396.85f,734.867f,1360.79f,2519.84f,4666.12f,8640.48f,16000};
 frequencies_.addArray(defaults,10);
}
FxController::~FxController() {lifetime_.reset();client_.deactivate();client_.onEvent={};}
void FxController::init(FxMainWindow* window,bool smoke) {
 main_window_=window;smoke_=smoke;view_=settings_.getInt("view",Pro)==Lite?Lite:Pro;initPresets();FxTheme::setThemeMode(settings_.getInt("theme",Dark)==Light?Light:Dark);setLanguage(getLanguage());FxModel::getModel().setHotkeySupport(false);
 client_.onEvent=[this,weak=std::weak_ptr<int>(lifetime_)](fxgui::Event event){if(!weak.expired())receive(std::move(event));};
 if(!smoke_)client_.activate();showView();
}
void FxController::receive(fxgui::Event event) {
 if(event.kind=="devices" && event.ok){updateDevices(event.data);return;}
 if(event.kind=="getState" && event.ok){updateState(event.data);return;}
 if(event.kind=="setPreset"){presetResult(event);advancePower();return;}
 if(event.kind=="finish"){if(event.ok && quit_)quit_();else {if(event.kind=="savePreset"){pending_save_.clear();pending_remove_.clear();}FxModel::getModel().pushMessage(event.error);}return;}
 if((event.kind=="setParams" || event.kind=="setNumEqBands") && restore_pending_>0)--restore_pending_;
 if(event.ok && event.kind=="setParams"){auto params=event.sent["params"];bool presetChange=false;
  for(auto property:params.getDynamicObject()->getProperties()){auto name=property.name.toString();presetChange=presetChange || name.startsWith("eq") || name.startsWith("freq") || name=="fidelity" || name=="ambience" || name=="surround" || name=="dynamicBoost" || name=="bass";}
  if(presetChange)modified();for(auto name:{"masterGain","balance","volumeLeveling","filterQ"})if(params.hasProperty(name))settings_.setDouble(name,double(params[name]));}
 if(event.ok && event.kind=="setNumEqBands"){settings_.setInt("numEqBands",int(event.sent["count"]));}
 if(!event.ok){if(event.kind=="start")start_pending_=false;
  if(event.kind=="setBypass" || event.kind=="activateRouting"){requested_power_=power_waiting_=bypass_pending_=routing_pending_=false;FxModel::getModel().setPowerState(false);}
  if(event.kind=="getState"){bool wasReady=ready_;ready_=false;FxModel::getModel().setPowerState(false);if(wasReady)FxModel::getModel().pushMessage(event.error);}else {if(event.kind=="savePreset"){pending_save_.clear();pending_remove_.clear();}FxModel::getModel().pushMessage(event.error);}return;}
 if(event.kind=="setOutputUID")settings_.setString("outputUID",event.sent["outputUID"].toString());
 if(event.kind=="setBypass" && !bool(event.sent["value"]) && bypass_pending_){
  bypass_pending_=false;
  if(requested_power_ && !smoke_ && fxgui::audioPermissionStatus()==fxgui::AudioPermissionStatus::granted){routing_pending_=true;client_.send("activateRouting","value",true);}
 }
 if(event.kind=="activateRouting")routing_pending_=false;
 if(event.kind=="savePreset" && pending_save_.isNotEmpty()){if(pending_remove_.isNotEmpty()){if(!File(pending_remove_).deleteFile())FxModel::getModel().pushMessage("The renamed preset was saved, but the old copy could not be removed.");pending_remove_.clear();}initPresets();setPreset(pending_save_,false);pending_save_.clear();}
 advancePower();
}
void FxController::updateState(const var& state) {
 state_=state;auto c=state["controls"];ready_=bool(state["metrics"]["ready"]) && bool(c["dspAvailable"]);
 if(ready_)start_pending_=false;
 bands_=int(c["numEqBands"]);if(bands_==0)bands_=c["eq"].size();
 applyPendingPreset();
 if(!smoke_ && ready_ && !restored_){
  restored_=true;int count=settings_.getInt("numEqBands",bands_);
  if(count!=bands_ && (count==5 || count==10 || count==15 || count==20 || count==31)){client_.send("setNumEqBands","count",count);++restore_pending_;}
  bool any=false;const char* names[]={"masterGain","balance","volumeLeveling","filterQ"};const float lo[]={-20,-20,0,1},hi[]={20,20,4,3};
  for(int i=0;i<4;++i)if(settings_.getString(names[i]).isNotEmpty()){double value=settings_.getDouble(names[i]);if(std::isfinite(value) && value>=lo[i] && value<=hi[i]){client_.parameter(names[i],value);any=true;}}
  if(any)++restore_pending_;
 }

 auto read=[](const var& array,Array<float>& dest){dest.clear();for(const auto& value:*array.getArray())dest.add(float(value));};
 read(c["effects"],effects_);read(c["eq"],gains_);read(c["frequencies"],frequencies_);
 bool power=ready_ && !smoke_ && fxgui::audioPermissionStatus()==fxgui::AudioPermissionStatus::granted
            && !bool(c["bypassed"]) && bool(state["routingActive"]);
 if(FxModel::getModel().getPowerState()!=power)FxModel::getModel().setPowerState(power);
 auto uid=state["outputUID"].toString();auto devices=FxModel::getModel().getOutputDevices();
 for(auto& device:devices)if(String(device.pwszID.c_str())==uid && FxModel::getModel().getSelectedOutput().pwszID!=device.pwszID)FxModel::getModel().setSelectedOutput(device);
 if(main_window_){main_window_->update();main_window_->setIcon(power,isAudioProcessing());}
 advancePower();
}
void FxController::updateDevices(const var& data) {
 std::vector<SoundDevice> devices;driver_=false;String defaultUID;
 if(!data.isArray())return;
 for(const auto& d:*data.getArray()){
  if(bool(d["virtual"])){driver_=true;continue;}if(int(d["outputs"])<2)continue;
  SoundDevice device;device.pwszID=d["uid"].toString().toWideCharPointer();
  if(bool(d["default"]))defaultUID=d["uid"].toString();
  device.deviceFriendlyName=d["name"].toString().toWideCharPointer();device.deviceNumChannel=int(d["outputs"]);devices.push_back(device);
 }
 auto& model=FxModel::getModel();auto old=model.getSelectedOutput();auto prior=model.getOutputDevices();bool changed=prior.size()!=devices.size();
 for(size_t i=0;!changed && i<devices.size();++i)changed=prior[i].pwszID!=devices[i].pwszID || prior[i].deviceFriendlyName!=devices[i].deviceFriendlyName || prior[i].deviceNumChannel!=devices[i].deviceNumChannel;
 if(changed)model.initOutputs(devices);
 String selected=ready_?state_["outputUID"].toString():settings_.getString("outputUID");
 bool found=false;for(auto& device:devices)found=found || String(device.pwszID.c_str())==selected;
 if(!found)selected=defaultUID.isNotEmpty()?defaultUID:(devices.empty()?String():String(devices.front().pwszID.c_str()));
 for(auto& device:devices)if(String(device.pwszID.c_str())==selected){model.setSelectedOutput(device,old.pwszID!=device.pwszID);break;}
 if(main_window_)main_window_->enablePowerButton(!devices.empty());
 beginAudio();
}
void FxController::beginAudio() {
 if(smoke_ || !driver_ || ready_ || start_pending_ || fxgui::audioPermissionStatus()!=fxgui::AudioPermissionStatus::granted)return;
 auto& model=FxModel::getModel();auto selected=model.getSelectedOutput().pwszID;
 if(selected.empty())return;
 start_pending_=true;client_.begin(String(selected.c_str()),model.getPreset(model.getSelectedPreset()).path);
}
void FxController::advancePower() {
 if(smoke_ || !requested_power_ || !power_waiting_ || !ready_ || restore_pending_>0
    || preset_pending_.isNotEmpty() || preset_in_flight_.isNotEmpty() || bypass_pending_ || routing_pending_)return;
 if(fxgui::audioPermissionStatus()!=fxgui::AudioPermissionStatus::granted){permissionResult(false);return;}
 power_waiting_=false;bypass_pending_=true;client_.send("setBypass","value",false);
}
void FxController::permissionResult(bool granted) {
 permission_pending_=false;if(!requested_power_ || smoke_)return;
 if(!granted || fxgui::audioPermissionStatus()!=fxgui::AudioPermissionStatus::granted){
  requested_power_=power_waiting_=false;FxModel::getModel().setPowerState(false);
  FxModel::getModel().pushMessage("Allow FxSound in System Settings > Privacy & Security > Microphone, then try turning it on again.");return;
 }
 beginAudio();advancePower();
}
void FxController::setPowerState(bool on) {
 if(smoke_)return;
 if(!on){requested_power_=power_waiting_=false;FxModel::getModel().setPowerState(false);
  if(ready_){client_.send("setBypass","value",true);client_.send("activateRouting","value",false);}return;}
 if(!driver_){FxConfirmationMessage::showMessage("Open FxSound-Install.pkg from the FxSound disk image to install the audio driver. Then restart your Mac and reopen FxSound.",FxConfirmationMessage::Style::OK);return;}
 if(requested_power_)return;
 requested_power_=power_waiting_=true;
 auto status=fxgui::audioPermissionStatus();
 if(status==fxgui::AudioPermissionStatus::granted){beginAudio();advancePower();return;}
 if(status!=fxgui::AudioPermissionStatus::notDetermined){permissionResult(false);return;}
 if(permission_pending_)return;permission_pending_=true;
 fxgui::requestAudioPermission([this,weak=std::weak_ptr<int>(lifetime_)](bool granted){
  juce::MessageManager::callAsync([this,weak,granted]{if(!weak.expired())permissionResult(granted);});
 });
}
bool FxController::isAudioProcessing(){return ready_ && bool(state_["routingActive"]) && !bool(state_["controls"]["bypassed"]);}
bool FxController::isPlaybackDeviceAvailable(){return driver_ && ready_;}
void FxController::checkDeviceChanges() {}
void FxController::refreshOutputList() {}
void FxController::setOutput(int index,bool) {auto devices=FxModel::getModel().getOutputDevices();if(index>=0 && index<int(devices.size()))setOutput(String(devices[size_t(index)].pwszID.c_str()));}
void FxController::setOutput(const String uid,bool) {
 client_.send("setOutputUID","outputUID",uid);
}
float FxController::control(const char* name,float fallback) const {auto value=state_["controls"][name];return value.isVoid()?fallback:float(value);}
void FxController::modified(){auto& m=FxModel::getModel();m.setPresetModified(m.getSelectedPreset(),true);}
void FxController::parameter(const String& name,float value){client_.parameter(name,value);}
float FxController::getEffectValue(FxEffects::EffectType effect){return effects_[int(effect)]/10.0f;}
void FxController::setEffectValue(FxEffects::EffectType effect,float value){static const char* names[]={"fidelity","ambience","surround","dynamicBoost","bass"};effects_.set(int(effect),value);parameter(names[int(effect)],value);}
int FxController::getNumEqBands(){return bands_;}
void FxController::setNumEqBands(int count){client_.send("setNumEqBands","count",count);}
float FxController::getEqBandFrequency(int index){return frequencies_[index];}
void FxController::setEqBandFrequency(int index,float value){frequencies_.set(index,value);parameter("freq"+String(index+1),value);}
float FxController::getEqBandBoostCut(int index){return gains_[index];}
void FxController::setEqBandBoostCut(int index,float value){gains_.set(index,value);parameter("eq"+String(index+1),value);}
void FxController::getEqBandFrequencyRange(int index,float* low,float* high){auto ranges=state_["controls"]["frequencyRanges"];if(ranges.isArray() && index<ranges.size()){*low=float(ranges[index][0]);*high=float(ranges[index][1]);}else{*low=20;*high=20000;}}
void FxController::getSpectrumBandValues(Array<float>& result){result.clear();auto data=state_["controls"]["spectrum"];if(data.isArray())for(auto& v:*data.getArray())result.add(float(v));else result.insertMultiple(0,0.0f,10);}
float FxController::getMasterGain(){return control("masterGain");}
void FxController::setMasterGain(float v){parameter("masterGain",v);}
float FxController::getBalance(){return control("balance");}
void FxController::setBalance(float v){parameter("balance",v);}
float FxController::getVolumeLeveling(){return control("volumeLeveling");}
void FxController::setVolumeLeveling(float v){parameter("volumeLeveling",v);}
float FxController::getFilterQ(){return control("filterQ",1);}
void FxController::setFilterQ(float v){parameter("filterQ",v);}
