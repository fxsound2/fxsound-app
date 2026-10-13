#include <JuceHeader.h>
#include "FxMainWindow.h"
#include "mac-controller.h"
#include "audio-permission.h"
#include <iostream>
class Tray final : public SystemTrayIconComponent {
public:
 Tray(){auto drawable=Drawable::createFromImageData(FXIMAGE(IconLogo),FXIMAGESIZE(IconLogo));Image image(Image::ARGB,32,32,true);Graphics g(image);drawable->drawWithin(g,{0,0,32,32},RectanglePlacement::centred,1);setIconImage(image,image);setIconTooltip("FxSound");}
 void mouseDown(const MouseEvent&) override {PopupMenu menu;menu.addItem(1,"Open FxSound");menu.addItem(2,"Quit FxSound");menu.showMenuAsync(PopupMenu::Options().withTargetComponent(this),[](int result){if(result==1)FxController::getInstance().showMainWindow();if(result==2)JUCEApplication::getInstance()->systemRequestedQuit();});}
};
class Application final : public JUCEApplication,private Timer {
public:
 const String getApplicationName() override{return "FxSound";}
 const String getApplicationVersion() override{return ProjectInfo::versionString;}
 bool moreThanOneInstanceAllowed() override{return getCommandLineParameters().contains("--smoke-test");}
 void initialise(const String& args) override {
  diagnostic_=args.contains("--audio-permission-status");
  if(diagnostic_){std::cout<<"audioPermissionStatus="<<static_cast<int>(fxgui::audioPermissionStatus())<<std::endl;quit();return;}
  smoke_=args.contains("--smoke-test");preview_=smoke_ && args.contains("--preview");theme_=std::make_unique<FxTheme>();LookAndFeel::setDefaultLookAndFeel(theme_.get());
  window_=std::make_unique<FxMainWindow>();auto& controller=FxController::getInstance();controller.onQuit([]{JUCEApplication::quit();});controller.init(window_.get(),smoke_);tray_=std::make_unique<Tray>();if(smoke_)startTimer(5000);
 }
 void shutdown() override {if(diagnostic_)return;stopTimer();FxController::getInstance().detach();tray_.reset();window_.reset();LookAndFeel::setDefaultLookAndFeel(nullptr);theme_.reset();}
 void systemRequestedQuit() override {if(smoke_)quit();else FxController::getInstance().exit();}
 void anotherInstanceStarted(const String&) override {FxController::getInstance().showMainWindow();}
private:
 void timerCallback() override {stopTimer();if(preview_ && window_){auto image=window_->createComponentSnapshot(window_->getLocalBounds());auto file=File("/private/tmp/fxsound-original-gui-preview.png");auto stream=file.createOutputStream();if(stream){stream->setPosition(0);stream->truncate();PNGImageFormat().writeImageToStream(image,*stream);}else setApplicationReturnValue(1);}quit();}
 bool smoke_=false,preview_=false,diagnostic_=false;std::unique_ptr<FxTheme> theme_;std::unique_ptr<FxMainWindow> window_;std::unique_ptr<Tray> tray_;
};
START_JUCE_APPLICATION(Application)
