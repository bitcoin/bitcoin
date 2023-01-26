rem for windows
set ANDROID_HOME=%APPDATA%\..\Local\Android\Sdk
set ANDROID_NDK_HOME=%ANDROID_HOME%\ndk-bundle
set PATH=%PATH%;%ANDROID_HOME%\tools;%ANDROID_NDK_HOME%\toolchains\arm-linux-androideabi-4.9\prebuilt\windows-x86_64\bin;%ANDROID_NDK_HOME%
set PATH=%PATH%;%ProgramFiles%\android\Android Studio\jre\bin;%ANDROID_HOME%\platform-tools