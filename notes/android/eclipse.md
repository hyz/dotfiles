
### https://stackoverflow.com/questions/27449206/what-is-the-final-version-of-the-adt-bundle

https://stackoverflow.com/questions/34301997/how-to-install-last-version-of-adt-plug-in-for-eclipse

adt: https://dl-ssl.google.com/android/eclipse/ https://github.com/khaledev/ADT

linux 64 bit: http://dl.google.com/android/adt/adt-bundle-linux-x86_64-20140702.zip
linux 32 bit: http://dl.google.com/android/adt/adt-bundle-linux-x86-20140702.zip
MacOS X: http://dl.google.com/android/adt/adt-bundle-mac-x86_64-20140702.zip
Win32: http://dl.google.com/android/adt/adt-bundle-windows-x86-20140702.zip
Win64: http://dl.google.com/android/adt/adt-bundle-windows-x86_64-20140702.zip

### 关于如何正确地在android项目中添加第三方jar包

http://stackoverflow.com/questions/3642928/adding-a-library-jar-to-an-eclipse-android-project

- Create a new folder, libs, in your Eclipse/Android project.
- Right-click libs and choose Import -> General -> File System, then Next, Browse in the filesystem to find the library's parent directory (i.e.: where you downloaded it to).
- Click OK, then click the directory name (not the checkbox) in the left pane, then check the relevant JAR in the right pane. This puts the library into your project (physically).
- Right-click on your project, choose Build Path -> Configure Build Path, then click the Libraries tab, then Add JARs..., navigate to your new JAR in the libs directory and add it. (This, incidentally, is the moment at which your new JAR is converted for use on Android.)

### 导入已经存在的应用

- 右键点击“Package Explorer”并选择“Import…”
- 当出现对话框时，选择“Exsiting Android project into workspace”（将现有Android项目导入工作区）。

依赖库, 参考cocos2d-x
这是因为缺少Cocos2d-x Android JNI桥接库（bridge library）。
解决方法：导入另一个Android项目，项目位置为“C:\cocos2d-x-2.2.0\cocos2dx\platform\android”。导入项目之后，错误即会消失，如下图所示。

