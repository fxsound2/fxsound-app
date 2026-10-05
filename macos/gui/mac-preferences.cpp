#include "mac-controller.h"
#include "FxMainWindow.h"
std::function<void()> DeviceConfig::onDeviceConfigsUpdate;
void FxController::showView(){if(main_window_){if(view_==Pro)main_window_->showProView();else main_window_->showLiteView();main_window_->show();}}
void FxController::switchView(){view_=view_==Pro?Lite:Pro;settings_.setInt("view",view_);showView();}
ViewType FxController::getCurrentView(){return view_;}
void FxController::hideMainWindow(){if(main_window_)main_window_->setVisible(false);}
void FxController::showMainWindow(){showView();}
bool FxController::isMainWindowVisible(){return main_window_ && main_window_->isVisible();}
FxWindow* FxController::getMainWindow(){return main_window_;}
void FxController::setMenuClicked(bool clicked){FxModel::getModel().setMenuClicked(clicked);}
Point<int> FxController::getSystemTrayWindowPosition(int width,int height){auto area=Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;return {area.getCentreX()-width/2,area.getCentreY()-height/2};}
bool FxController::exit(){client_.finish();return false;}
void FxController::saveWindowPosition(int x,int y){settings_.setInt("windowX",x);settings_.setInt("windowY",y);}
void FxController::getWindowPosition(int& x,int& y){auto area=Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;x=settings_.getInt("windowX",area.getCentreX()-520);y=settings_.getInt("windowY",area.getCentreY()-300);}
bool FxController::isAlwaysOnTop(){return settings_.getBool("alwaysOnTop");}
void FxController::setAlwaysOnTop(bool value){settings_.setBool("alwaysOnTop",value);if(main_window_)main_window_->setAlwaysOnTop(value);}
FxThemeMode FxController::getThemeMode(){return FxTheme::getThemeMode();}
void FxController::setThemeMode(FxThemeMode mode){settings_.setInt("theme",mode);FxTheme::setThemeMode(mode);auto& theme=dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());theme.loadFont(getLanguage());if(main_window_){main_window_->setLookAndFeel();main_window_->repaint();}}
bool FxController::isHelpTooltipsHidden(){return settings_.getBool("hideTooltips");}
void FxController::setHelpTooltipsHidden(bool value){settings_.setBool("hideTooltips",value);}
bool FxController::isNotificationsHidden(){return settings_.getBool("hideNotifications");}
void FxController::setNotificationsHidden(bool value){settings_.setBool("hideNotifications",value);}
bool FxController::isLaunchOnStartup(){return false;}
void FxController::setLaunchOnStartup(bool){FxModel::getModel().pushMessage("Add FxSound in System Settings > General > Login Items.");}
bool FxController::getAutoUpdates(){return false;}
void FxController::setAutoUpdates(bool){FxModel::getModel().pushMessage("Install a newer FxSound build to update this personal version.");}
void FxController::checkUpdates(){FxModel::getModel().pushMessage("This personal build is updated by installing a newer FxSound app.");}
void FxController::enableHotkeys(bool){FxModel::getModel().setHotkeySupport(false);}
bool FxController::getHotkey(String,int& mod,int& vk){mod=vk=0;return false;}
bool FxController::setHotkey(const String&,int,int){return false;}
bool FxController::isValidHotkey(int,int){return false;}
Array<DeviceConfig> FxController::getDeviceConfigs(){return device_configs_;}
void FxController::saveDeviceConfigs(const Array<DeviceConfig>& devices){device_configs_=devices;}
bool FxController::isOutputDeviceConnected(const String& name){return isOutputDevicePresent(name);}
bool FxController::isOutputDevicePresent(const String& name){return FxModel::getModel().getOutputNames().contains(name);}
bool FxController::isNewOutputPrioritized(){return false;}
void FxController::setNewOutputPrioritized(bool){FxModel::getModel().pushMessage("Choose the output device in the FxSound output menu.");}
const String& FxController::getOutputName(){return output_device_name_;}
void FxController::setOutputName(const String& name){output_device_name_=name;}
