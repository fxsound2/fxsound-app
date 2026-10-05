#include <JuceHeader.h>
#define private public
#define protected public
#include "mac-controller.h"
#include "FxMainWindow.h"
#undef private
#undef protected
#include <iostream>
std::string presetTestDefaultUID();

namespace {
Image render(FxPowerButton& button) {
 return button.createComponentSnapshot(button.getLocalBounds());
}
bool equal(const Image& a,const Image& b) {
 if(a.getBounds()!=b.getBounds())return false;
 for(int y=0;y<a.getHeight();++y)
  for(int x=0;x<a.getWidth();++x)
   if(a.getPixelAt(x,y)!=b.getPixelAt(x,y))return false;
 return true;
}
}
int main() {
 ScopedJuceInitialiser_GUI gui;
 FxTheme theme;LookAndFeel::setDefaultLookAndFeel(&theme);
 int failures=0;
 {
  auto& controller=FxController::getInstance();auto& model=FxModel::getModel();
  if(controller.client_.isThreadRunning())return 2;
  PropertiesFile::Options options;options.millisecondsBeforeSaving=-1;
  PropertiesFile isolated(File("/private/tmp/fxsound-power-button-tests/isolated.settings"),options);
  auto* previous=controller.settings_.user_settings_;controller.settings_.user_settings_=&isolated;
  const auto outputBefore=presetTestDefaultUID();
  model.setPowerState(false);
  {
   FxMainWindow window;
   for(auto view:{ViewType::Pro,ViewType::Lite}) {
    controller.view_=view;
    if(view==ViewType::Pro)window.showProView();else window.showLiteView();
    std::cout<<(view==ViewType::Pro?"Pro":"Lite")<<" original toolbar regression"<<std::endl;
    model.setPowerState(false);window.update();
    auto& button=window.power_button_;
    FxPowerButton reference("power reference");
    reference.setSize(button.getWidth(),button.getHeight());
    reference.setImageWidth(button.getImageWidth());
    reference.setEnabled(button.isEnabled());
    reference.setPowerState(false);auto off=render(reference);
    reference.setPowerState(true);auto on=render(reference);
    if(equal(off,on)){std::cerr<<"Original ON/OFF assets are indistinguishable\n";++failures;}
    auto check=[&](bool expected,const char* phase){
     auto image=render(button);bool pixels=equal(image,expected?on:off);
     bool cache=button.getPowerState()==expected;
     std::cout<<phase<<": model="<<model.getPowerState()<<" cache="<<button.getPowerState()<<" original pixels match="<<pixels<<std::endl;
     if(!cache || !pixels)++failures;
    };
    check(false,"initial OFF");
    model.setPowerState(true);window.update();check(true,"ON before mouse/message pump");
    model.setPowerState(false);window.update();check(false,"OFF before mouse/message pump");
    for(bool state:{true,false,true}){model.setPowerState(state);window.update();check(state,"rapid transition before pump");}
    MessageManager::getInstance()->runDispatchLoopUntil(100);
    check(true,"latest ON after delayed notifications");
    model.setPowerState(false);window.update();check(false,"final OFF before pump");
    MessageManager::getInstance()->runDispatchLoopUntil(100);
    check(false,"final OFF after delayed notifications");
   }
   if(controller.client_.isThreadRunning() || !controller.client_.commands_.empty()
      || presetTestDefaultUID()!=outputBefore){
    std::cerr<<"Unexpected worker/command activity\n";++failures;
   }
  }
  controller.settings_.user_settings_=previous;
 }
 LookAndFeel::setDefaultLookAndFeel(nullptr);DeletedAtShutdown::deleteAll();
 std::cout<<"Real FxMainWindow/original SVG power rendering: "<<failures<<" failures; no helpers, IPC worker, consent or audio IO; default output unchanged\n";
 return failures?1:0;
}
