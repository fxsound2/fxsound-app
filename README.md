# FxSound

FxSound is a digital audio program built for Windows PC's. The background processing, built on the highest-fidelity audio engine possible, acts as a sort of digital soundcard for your system. This means that your signals will have the cleanest possible passthrough when FxSound is active. There are active effects for shaping and boosting your sound's volume, timbre, and equalization included on top of this clean processing, allowing you to customize and enhance your sound.

## General Information
* Website: https://www.fxsound.com
* Installer: https://download.fxsound.com/fxsoundlatest
* Source code: https://github.com/fxsound2/fxsound-app
* Issue tracker: https://github.com/fxsound2/fxsound-app/issues
* Forum: https://forum.fxsound.com

## Build Instructions
### Prerequisites
* Download and install the [latest version of FxSound](https://download.fxsound.com/fxsoundlatest)
* Install [Visual Studio 2022](https://visualstudio.microsoft.com/vs)
* Install [Windows SDK](https://developer.microsoft.com/en-us/windows/downloads/windows-sdk)
* Install [JUCE framework version 6.1.6](https://github.com/juce-framework/JUCE/releases/tag/6.1.6)
  
FxSound application requires FxSound Audio Enhancer virtual audio driver. So, to run FxSound application built from source, we need to install FxSound which installs the audio driver.
While building with JUCE 7.x.x version we ran into an issue that the application CPU utilisation goes high when the display is off. So, we are building FxSound with JUCE 6.1.6.

### Build FxSound from Visual Studio
* Open [fxsound/Project/FxSound.sln](https://github.com/fxsound2/fxsound-app/blob/main/fxsound/Project/FxSound.sln) in Visual Studio
* Build the required configuration and platform and run
  
## Contribution Guidelines

## License
[GPL v3.0](https://github.com/fxsound2/fxsound-app/blob/main/LICENSE)
