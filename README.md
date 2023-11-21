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

### Build after exporting the project form Projucer
FxSound application has three components. 
1. FxSound GUI application which uses JUCE framework
2. Audiopassthru module which is used by the application to interact with the audio devices
3. DfxDsp module which is the DSP for processing audio

Due to the some limitations with Projucer, after exporting the Visual Studio solution from Projucer, few changes have to be made in the solution to build FxSound.
1. Since the audiopassthru and DfxDsp dependency projects cannot be added to the solution when FxSound.sln is exported, open fxsound/Project/FxSound.sln in Visual Studio and add the existing projects audiopassthru/audiopassthru.vcxproj, dsp/DfxDsp.vcxproj.
2. From FxSound_App project, add reference to audiopassthru and DfxDsp.
3. By default obly x64 platform configuration is created in the exported FxSound_App project. To build 32 bit, add a 32 bit Win32 configuration from Visual Studio Configuration Manager as a new platform and choose x64 in the "Copy settings from:" option.
4. If you run FxSound from Visual Studio, to let the application to use the presets, set the Working Directory to ```$(SolutionDir)..\..\bin\$(PlatformTarget)``` in FxSound_App Project->Properties->Debugging setting.
   
## Contribution Guidelines
Issues are tracked in [GitHub issues](https://github.com/fxsound2/fxsound-app/issues). To make contributions to the project create a branch from an issue and submit a Pull Request with your code changes.
Please follow the coding style as per the module (FxSound_App, audiopassthru, DfxDsp) that you are contributing to. FxSound_App which is developed in C++ follows [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).

## License
[GPL v3.0](https://github.com/fxsound2/fxsound-app/blob/main/LICENSE)
