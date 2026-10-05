FxSound — bản macOS dùng cá nhân

Bản ứng dụng hiện tại dành cho Mac Apple Silicon (M1/M2/M3/M4 hoặc mới hơn).

1. Mở FxSound-Install.pkg và làm theo macOS Installer.
2. Mở /Applications/FxSound.app. Nếu chưa thấy thiết bị FxSound, khởi động lại máy.
3. Chọn loa/tai nghe vật lý trong FxSound rồi bật nút nguồn.
4. Khi macOS hỏi quyền truy cập âm thanh/Microphone, chọn Allow.
   FxSound dùng quyền này để đọc âm thanh từ thiết bị ảo, không dùng mic tai nghe.
   Nếu đã từ chối, cho phép FxSound tại System Settings → Privacy & Security → Microphone,
   rồi mở lại ứng dụng. Khi chưa có quyền, FxSound giữ âm thanh trực tiếp qua loa/tai nghe.

Bộ cài cài ứng dụng và driver tại /Library/Audio/Plug-Ins/HAL/FxSound.driver.
Dịch vụ âm thanh sẽ khởi động lại sau khi cài, âm thanh có thể ngắt trong chốc lát.
Hãy thoát FxSound trước khi cài lại hoặc gỡ cài đặt.

Bản cá nhân ký ad-hoc, chưa được Apple notarize. Nếu macOS chặn bộ cài/ứng dụng,
mở System Settings → Privacy & Security → Open Anyway cho đúng file FxSound.

Nguồn và giấy phép kèm trong fxsound-personal-source.tar.gz và LICENSES.txt.
Build hiện tại dùng JUCE 8.0.15 và macOS deployment target 14.0.
Build lại từ mã nguồn: giải nén archive, dùng CMake với
  cmake -S macos -B build -DFXSOUND_JUCE_SOURCE="$PWD/JUCE" -DCMAKE_BUILD_TYPE=Release
  cmake --build build --parallel
Build driver và bộ cài theo macos/driver/build.sh và macos/packaging/build-personal.sh.
Không có chứng nhận tương đương số học Windows hay kiểm thử thực tế trên mọi máy.
Phạm vi kiểm tra thực tế được ghi trong tài liệu phát hành; không bảo đảm mọi thiết bị.
Các thiết lập riêng cho Windows (global hotkeys, tự cập nhật và ưu tiên thiết bị
tự động) chưa hỗ trợ trên bản cá nhân. Thêm FxSound vào macOS Login Items nếu
muốn mở ứng dụng khi đăng nhập.

Gỡ cài đặt: thoát FxSound, chọn lại loa/tai nghe trong macOS Sound, rồi chạy
Uninstall-FxSound.command. Thao tác sẽ hỏi quyền quản trị để xóa driver và app.
