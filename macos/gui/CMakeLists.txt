set(FXSOUND_JUCE_SOURCE "/private/tmp/fxsound-juce-8" CACHE PATH "Pinned JUCE source directory")
if(NOT TARGET juce::juce_gui_basics)
    add_subdirectory("${FXSOUND_JUCE_SOURCE}" "${CMAKE_BINARY_DIR}/juce" EXCLUDE_FROM_ALL)
endif()
juce_add_gui_app(FxSound PRODUCT_NAME "FxSound" COMPANY_NAME "FxSound"
    BUNDLE_ID "org.fxsound.personal" VERSION 0.1.3 MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "FxSound reads its virtual audio input to enhance sound from your apps. It does not record your physical microphone."
    PLIST_TO_MERGE "<plist><dict><key>NSAudioCaptureUsageDescription</key><string>FxSound processes audio from your apps and sends it to your selected speakers or headphones.</string></dict></plist>")
file(GLOB FXSOUND_IMAGES "../../fxsound/Images/*.svg" "../../fxsound/Images/*.png")
juce_add_binary_data(FxSoundAssets SOURCES ${FXSOUND_IMAGES}
    "../../fxsound/Fonts/Gilroy-Regular.ttf" "../../fxsound/Fonts/Gilroy-Semibold.ttf" "../../fxsound/Fonts/Gilroy-Bold.ttf")
juce_generate_juce_header(FxSound)
file(GLOB ORIGINAL_GUI "../../fxsound/Source/GUI/*.cpp")
list(FILTER ORIGINAL_GUI EXCLUDE REGEX "FxController.cpp|FxSystemTrayView.cpp")
target_sources(FxSound PRIVATE app.cpp audio-permission.mm engine-client.cpp state-validation.cpp mac-controller.cpp mac-presets.cpp mac-preferences.cpp mac-language.cpp
    ${ORIGINAL_GUI} ../../fxsound/Source/Utils/Settings/Settings.cpp
    ../engine/ipc-server.cpp ../engine/audio-devices.cpp)
target_include_directories(FxSound PRIVATE . ../../fxsound/Source/GUI)
target_compile_definitions(FxSound PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
target_compile_options(FxSound PRIVATE -Wno-deprecated-declarations)
target_link_libraries(FxSound PRIVATE FxSoundAssets juce::juce_gui_basics juce::juce_gui_extra
    "-framework CoreAudio" "-framework CoreFoundation" "-framework AVFoundation")
add_dependencies(FxSound fxsound-engine fxsound-supervisor)
add_custom_command(TARGET FxSound POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:FxSound>/Contents/Helpers"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:fxsound-engine>" "$<TARGET_BUNDLE_DIR:FxSound>/Contents/Helpers/fxsound-engine"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:fxsound-supervisor>" "$<TARGET_BUNDLE_DIR:FxSound>/Contents/Helpers/fxsound-supervisor"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../Installer/Resources/Factsoft"
        "$<TARGET_BUNDLE_DIR:FxSound>/Contents/Resources/Factsoft"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../Resources/Strings" "$<TARGET_BUNDLE_DIR:FxSound>/Contents/Resources/Strings"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../fxsound/Fonts" "$<TARGET_BUNDLE_DIR:FxSound>/Contents/Resources/Fonts"
    COMMAND /usr/bin/codesign --force --sign - --options runtime --timestamp=none --entitlements "${CMAKE_CURRENT_SOURCE_DIR}/../packaging/audio-input.entitlements" "$<TARGET_BUNDLE_DIR:FxSound>/Contents/Helpers/fxsound-engine"
    COMMAND /usr/bin/codesign --force --sign - --options runtime --timestamp=none --entitlements "${CMAKE_CURRENT_SOURCE_DIR}/../packaging/audio-input.entitlements" "$<TARGET_BUNDLE_DIR:FxSound>/Contents/Helpers/fxsound-supervisor"
    COMMAND /usr/bin/codesign --force --sign - --options runtime --timestamp=none --entitlements "${CMAKE_CURRENT_SOURCE_DIR}/../packaging/audio-input.entitlements" "$<TARGET_BUNDLE_DIR:FxSound>"
    COMMAND /usr/bin/codesign --verify --strict --deep "$<TARGET_BUNDLE_DIR:FxSound>")

juce_add_console_app(fxsound-gui-state-test PRODUCT_NAME "FxSound GUI state test" VERSION 0.1.0)
target_sources(fxsound-gui-state-test PRIVATE state-validation-test.cpp state-validation.cpp)
target_compile_definitions(fxsound-gui-state-test PRIVATE JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0)
target_link_libraries(fxsound-gui-state-test PRIVATE juce::juce_core)
