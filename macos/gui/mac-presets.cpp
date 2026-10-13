#include "mac-controller.h"
File FxController::userPresets() const {return File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("Application Support/FxSound/Presets");}
void FxController::initPresets(){
 Array<FxModel::Preset> presets;
 auto add=[&](File folder,FxModel::PresetType type){for(auto file:folder.findChildFiles(File::findFiles,false,"*.fac")){
  if(file.getSize()>1024*1024)continue;FileInputStream stream(file);if(stream.failedToOpen())continue;
  auto header=stream.readNextLine();stream.readNextLine();auto name=stream.readNextLine().trim();
  if(header.startsWith("CLASS1") && name.isNotEmpty())presets.add({name,file.getFullPathName(),type,false});
 }};
 add(fxgui::EngineClient::bundle().getChildFile("Resources/Factsoft"),FxModel::AppPreset);add(userPresets(),FxModel::UserPreset);
 auto& model=FxModel::getModel();model.initPresets(presets);
 String name=settings_.getString("preset");if(name.isEmpty())name="General";
 for(int i=0;i<presets.size();++i)if(preset_pending_.isNotEmpty()?presets[i].path==preset_pending_:presets[i].name==name)model.selectPreset(i);
 if(preset_confirmed_.isEmpty())preset_confirmed_=model.getPreset(model.getSelectedPreset()).path;
}
bool FxController::setPreset(const String& name,bool notify){auto& model=FxModel::getModel();for(int i=0;i<model.getPresetCount();++i)if(model.getPreset(i).name==name)return setPreset(i,notify);return false;}
bool FxController::setPreset(int index,bool notify){
 auto& model=FxModel::getModel();auto preset=model.getPreset(index);if(preset.path.isEmpty())return false;
 preset_pending_=preset.path;model.selectPreset(index,notify);applyPendingPreset();return true;
}
void FxController::applyPendingPreset(){
 if(!ready_ || smoke_ || preset_pending_.isEmpty() || preset_in_flight_.isNotEmpty())return;
 preset_in_flight_=preset_pending_;client_.send("setPreset","path",preset_in_flight_);
}
void FxController::presetResult(const fxgui::Event& event){
 auto path=event.sent["path"].toString();if(path!=preset_in_flight_)return;
 preset_in_flight_.clear();auto& model=FxModel::getModel();
 if(event.ok){
  preset_confirmed_=path;
  for(int i=0;i<model.getPresetCount();++i)if(model.getPreset(i).path==path){settings_.setString("preset",model.getPreset(i).name);model.setPresetModified(i,false);break;}
  if(preset_pending_==path)preset_pending_.clear();
 }else if(preset_pending_==path){preset_pending_.clear();model.pushMessage(event.error);}
 if(preset_pending_.isEmpty())for(int i=0;i<model.getPresetCount();++i)if(model.getPreset(i).path==preset_confirmed_){model.selectPreset(i);break;}
 applyPendingPreset();
}
void FxController::savePreset(const String& requested){
 auto& model=FxModel::getModel();auto preset=model.getPreset(model.getSelectedPreset());auto name=requested.isNotEmpty()?requested:preset.name;
 if(name.isEmpty() || name.containsAnyOf("/\\:") || name=="." || name==".."){pending_remove_.clear();return;}
 if(userPresets().createDirectory().failed()){pending_remove_.clear();model.pushMessage("Could not create your presets folder.");return;}
 pending_save_=name;client_.send("savePreset","path",userPresets().getChildFile(name+".fac").getFullPathName());
}
void FxController::renamePreset(const String& name){
 auto& m=FxModel::getModel();auto preset=m.getPreset(m.getSelectedPreset());if(preset.type!=FxModel::UserPreset || name.containsAnyOf("/\\:") || name.isEmpty())return;
 if(userPresets().getChildFile(name+".fac").existsAsFile()){m.pushMessage("A preset with that name already exists.");return;}
 pending_remove_=preset.path;savePreset(name);
}
void FxController::deletePreset(){auto& m=FxModel::getModel();auto preset=m.getPreset(m.getSelectedPreset());if(preset.type==FxModel::UserPreset && File(preset.path).deleteFile()){initPresets();setPreset(0);}}
void FxController::undoPreset(){setPreset(FxModel::getModel().getSelectedPreset());}
void FxController::resetPresets(){undoPreset();}
void FxController::autoSaveModifiedPreset() {auto path=File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("Application Support/FxSound/user-session.fac");if(path.getParentDirectory().createDirectory().wasOk())client_.send("savePreset","path",path.getFullPathName());}
bool FxController::exportPresets(const Array<FxModel::Preset>& presets){auto folder=File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("FxSound/Presets/Export");if(folder.createDirectory().failed())return false;bool ok=true;for(auto preset:presets)ok=File(preset.path).copyFileTo(folder.getChildFile(File(preset.path).getFileName())) && ok;if(ok)folder.revealToUser();return ok;}
bool FxController::importPresets(const Array<File>& files,StringArray& imported,StringArray& skipped){if(userPresets().createDirectory().failed())return false;for(auto file:files){auto target=userPresets().getChildFile(file.getFileName());if(!file.hasFileExtension("fac") || target.exists() || !file.copyFileTo(target))skipped.add(file.getFileName());else imported.add(file.getFileName());}initPresets();return !imported.isEmpty();}
int FxController::getMaxUserPresets() const{return 1000;}
