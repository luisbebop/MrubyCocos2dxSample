#MrubyCocos2dxSample

A sample demonstrating how to use Cocos2dx and mruby.

##Requirements
[Cocos2d-x v2.2.1](http://www.cocos2d-x.org/download)

This sample was compiled using Mac OS X 10.8.5 and XCode 4.6.3

##Instructions

Open the MrubyCocos2dxSample.xcodeproj inside proj.ios folder.
You can change the mruby script that is been executed changing the script name inside Classes/AppDelegate.cpp. To change the script go to the Resources/hello.rb. Just compile this project as a normal iOS project.

##How to generate a new project from scratch

1. Go to the path where you unzipped the Cocos2d-x v2.2.1

2. Go to the folder tools/project-creator/

3. Execute the command
```shell
./create_project.py -project MyGame -package com.MyCompany.AwesomeGame -language cpp
```

4. There will be a new project inside cocos2d-x-2.2.1/projects/MyGame. Open the Xcode project proj.ios/MyGame.xcodeproj

5. Create a new folder inside this project named libs. Copy the mruby_binding directory from [https://github.com/ktaobo/cocos2dx-mruby](https://github.com/ktaobo/cocos2dx-mruby) . Add it to the Xcode project too.

6. Replace the files inside the folder MyGame/Classes by the files inside the example/Classes from here [https://github.com/ktaobo/cocos2dx-mruby](https://github.com/ktaobo/cocos2dx-mruby) . Add it to the Xcode project too.

7. Add the files inside from example/Resources from [https://github.com/ktaobo/cocos2dx-mruby](https://github.com/ktaobo/cocos2dx-mruby) to MyGame/Resources . Add it to the Xcode project too.

8. On Xcode and add on Build Settings > Search Paths > User Header Search Paths the value "$(SRCROOT)/../libs/mruby_binding/include" . Check if Build Settings > Search Paths > Always Search User Paths is 'No'

9. On the same configuration Xcode screen find Build Settings > Apple LLVM compiler 4.2 - Preprocessing and add the define DISABLE_GEMS on Debug and Release.

##Further information

[https://github.com/ktaobo/cocos2dx-mruby](https://github.com/ktaobo/cocos2dx-mruby)

[http://ktaobo.blogspot.jp/2013/09/cocos2dx-mruby.html](http://ktaobo.blogspot.jp/2013/09/cocos2dx-mruby.html)