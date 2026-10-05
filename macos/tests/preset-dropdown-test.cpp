#include <JuceHeader.h>

#define private public
#define protected public
#include "mac-controller.h"
#include "audio-permission.h"
#include "FxProView.h"
#include "FxLiteView.h"
#undef private
#undef protected
#undef NDEBUG
#include <iostream>
#include <cassert>
std::string presetTestDefaultUID();
std::string presetTestDevicesJSON();
int main() {
 ScopedJuceInitialiser_GUI gui;
 FxTheme theme;LookAndFeel::setDefaultLookAndFeel(&theme);
 int failures=0;
 {
  auto& controller=FxController::getInstance();auto& model=FxModel::getModel();
  assert(!controller.client_.isThreadRunning());
  PropertiesFile::Options options;options.millisecondsBeforeSaving=-1;
  PropertiesFile isolated(File("/private/tmp/fxsound-preset-tests/isolated.settings"),options);
  auto* previous=controller.settings_.user_settings_;controller.settings_.user_settings_=&isolated;
  controller.settings_.setString("preset","General");
  const auto outputBefore=presetTestDefaultUID();
  FxProView pro;FxLiteView lite;
  controller.initPresets();model.setPowerState(false);
  MessageManager::getInstance()->runDispatchLoopUntil(50);
  int factory=0;StringArray names;
  for(int i=0;i<model.getPresetCount();++i){auto p=model.getPreset(i);if(p.type==FxModel::AppPreset)++factory;names.add(p.name);}
  assert(factory==13 && names.contains("General") && names.contains("Gaming"));
  assert(pro.preset_list_.getNumItems()==model.getPresetCount());
  assert(lite.preset_list_.getNumItems()==model.getPresetCount());
  Image image(Image::ARGB,1040,588,true);Graphics graphics(image);
  pro.paint(graphics);lite.paint(graphics);
  std::cout<<"real installed factory catalog="<<factory<<" visiblepro="<<pro.preset_list_.getNumItems()<<" visiblelite="<<lite.preset_list_.getNumItems()<<" power="<<model.getPowerState()<<" ready="<<controller.ready_<<" enabledpro="<<pro.preset_list_.isEnabled()<<" enabledlite="<<lite.preset_list_.isEnabled()<<std::endl;
  failures+=!pro.preset_list_.isEnabled();failures+=!lite.preset_list_.isEnabled();
  if(!failures){
   const int selected=names.indexOf("Gaming");
   pro.preset_list_.setSelectedItemIndex(selected,sendNotificationSync);
   MessageManager::getInstance()->runDispatchLoopUntil(50);
   assert(model.getSelectedPreset()==selected && !model.getPowerState() && !controller.requested_power_);
   assert(controller.client_.commands_.empty());
   assert(controller.settings_.getString("preset")=="General");
   assert(controller.preset_pending_==model.getPreset(selected).path && controller.preset_in_flight_.isEmpty());
   assert(lite.preset_list_.getSelectedItemIndex()==selected);
   controller.initPresets();MessageManager::getInstance()->runDispatchLoopUntil(50);
   assert(model.getPreset(model.getSelectedPreset()).name=="Gaming");
   auto actualDevices=JSON::parse(presetTestDevicesJSON());controller.updateDevices(actualDevices);
   if(controller.driver_ && fxgui::audioPermissionStatus()==fxgui::AudioPermissionStatus::granted){
    assert(controller.client_.commands_.size()==1);
    assert(controller.client_.commands_[0].first=="start");
    assert(controller.client_.commands_[0].second["preset"].toString()==controller.preset_pending_);
    controller.client_.commands_.clear();
    std::cout<<"Actual device discovery queues startup with deferred Gaming path; worker never started\n";
   }
   controller.start_pending_=false;
   assert(!model.getPowerState() && !controller.requested_power_);
   assert(presetTestDefaultUID()==outputBefore);
   assert(!controller.client_.isThreadRunning());
   auto reply=[&](const String& path,bool ok){auto sent=fxgui::commandObject("setPreset");sent.getDynamicObject()->setProperty("path",path);controller.receive({"setPreset",{},sent,ok,ok?String():String("rejected preset")});};
   const auto gaming=model.getPreset(names.indexOf("Gaming")).path;
   const auto music=model.getPreset(names.indexOf("Music")).path;
   const auto voice=model.getPreset(names.indexOf("Voice")).path;
   controller.ready_=true;controller.applyPendingPreset();
   assert(controller.client_.commands_.size()==1 && controller.preset_in_flight_==gaming);
   reply(gaming,true);assert(controller.settings_.getString("preset")=="Gaming");
   assert(controller.preset_pending_.isEmpty() && controller.preset_in_flight_.isEmpty());
   controller.client_.commands_.clear();
   assert(controller.setPreset("Music") && controller.setPreset("Gaming"));
   assert(controller.client_.commands_.size()==1 && controller.preset_in_flight_==music);
   reply(voice,true);assert(controller.preset_in_flight_==music && controller.client_.commands_.size()==1);
   reply(music,true);assert(model.getPreset(model.getSelectedPreset()).name=="Gaming");
   assert(controller.preset_in_flight_==gaming && controller.client_.commands_.size()==2);
   reply(gaming,true);assert(controller.settings_.getString("preset")=="Gaming");
   assert(controller.preset_pending_.isEmpty() && controller.preset_in_flight_.isEmpty());
   assert(controller.setPreset("Music"));reply(music,false);
   assert(model.getPreset(model.getSelectedPreset()).name=="Gaming" && controller.settings_.getString("preset")=="Gaming");
   assert(controller.preset_pending_.isEmpty() && controller.preset_in_flight_.isEmpty());
   reply(music,true);assert(controller.settings_.getString("preset")=="Gaming");
   for(const auto& command:controller.client_.commands_)assert(command.first=="setPreset");
   assert(!model.getPowerState() && !controller.requested_power_ && !controller.client_.isThreadRunning());
   assert(presetTestDefaultUID()==outputBefore);
   std::cout<<"Actual controller event-state units:ready-deferred apply/latest ACK/stale wrong-path/NACK rollback/persist-only ACK/no route or bypass commands passed; no HAL readiness claim\n";
   std::cout<<"Actual controller deferred Gaming selection with no ready engine/no command/no power request passed\n";
  }
  controller.client_.commands_.clear();controller.driver_=controller.ready_=true;
  controller.requested_power_=controller.power_waiting_=true;controller.permission_pending_=true;
  controller.permissionResult(false);
  assert(!controller.permission_pending_ && !controller.requested_power_ && !controller.power_waiting_);
  assert(!model.getPowerState() && controller.client_.commands_.empty());
  controller.requested_power_=false;controller.permission_pending_=true;
  controller.permissionResult(true);
  assert(!controller.permission_pending_ && controller.client_.commands_.empty());
  controller.smoke_=true;controller.setPowerState(true);
  assert(!controller.requested_power_ && controller.client_.commands_.empty());controller.smoke_=false;
  if(fxgui::audioPermissionStatus()==fxgui::AudioPermissionStatus::granted){
   controller.requested_power_=controller.power_waiting_=true;controller.restore_pending_=1;
   controller.advancePower();assert(controller.client_.commands_.empty());
   controller.restore_pending_=0;controller.preset_in_flight_="actual pending ACK";
   controller.advancePower();assert(controller.client_.commands_.empty());controller.preset_in_flight_.clear();
   controller.advancePower();controller.advancePower();
   assert(controller.client_.commands_.size()==1 && controller.bypass_pending_);
   assert(controller.client_.commands_[0].first=="setBypass" && !bool(controller.client_.commands_[0].second["value"]));
   auto bypass=controller.client_.commands_[0].second;controller.receive({"setBypass",{},bypass,true,{}});
   assert(controller.client_.commands_.size()==2 && controller.routing_pending_);
   assert(controller.client_.commands_[1].first=="activateRouting" && bool(controller.client_.commands_[1].second["value"]));
   auto route=controller.client_.commands_[1].second;controller.receive({"activateRouting",{},route,false,"unit NACK"});
   assert(!controller.requested_power_ && !model.getPowerState() && !controller.routing_pending_);
   controller.client_.commands_.clear();controller.requested_power_=controller.power_waiting_=true;
   controller.advancePower();assert(controller.client_.commands_.size()==1);
   bypass=controller.client_.commands_[0].second;controller.setPowerState(false);
   controller.receive({"setBypass",{},bypass,true,{}});
   assert(controller.client_.commands_.size()==3 && !controller.requested_power_);
   for(auto& command:controller.client_.commands_)if(command.first=="activateRouting")assert(!bool(command.second["value"]));
   std::cout<<"Real native granted context: restore/preset ACK barriers, one bypass then routing ACK ordering, NACK/cancel and OFF before ACK passed with inactive worker\n";
  }
  assert(!controller.client_.isThreadRunning() && presetTestDefaultUID()==outputBefore);
  std::cout<<"Explicit rejected permission-result/smoke/no-longer-requested completion units passed; no native authorization substitution\n";
  controller.settings_.user_settings_=previous;
 }
 LookAndFeel::setDefaultLookAndFeel(nullptr);DeletedAtShutdown::deleteAll();
 return failures?1:0;
}
