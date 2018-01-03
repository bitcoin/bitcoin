# RavenBinaries Mac Download Instructions

1) Download and copy raven.dmg to desired folder 

2) Double click the raven.dmg

3) Drag Raven Core icon to the Applications 

4) Launch Raven Core

Note: On Raven Core launch if you get this error

```
Dyld Error Message:
  Library not loaded: @loader_path/libboost_system-mt.dylib
  Referenced from: /Applications/Raven-Qt.app/Contents/Frameworks/libboost_thread-mt.dylib
  Reason: image not found
```
You will need to copy libboost_system.dylib to libboost_system-mt.dylib in the /Applications/Raven-Qt.app/Contents/Frameworks folder  
  