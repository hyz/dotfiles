
### https://developer.android.google.cn/studio/intro/studio-config#offline

    https://developer.android.google.cn/r/studio-offline/downloads

- 在 Windows 上：%USER_HOME%/.android/manual-offline-m2/
- 在 macOS 和 Linux 上：~/.android/manual-offline-m2/

- 删除 manual-offline-m2/ 目录中的内容。
- 重新下载离线组件。
- 将所下载的 ZIP 文件的内容解压缩到 manual-offline-m2/ 目录中。

- 使用以下路径和文件名创建一个空文本文件：
    - 在 Windows 上：%USER_HOME%/.gradle/init.d/offline.gradle
    - 在 macOS 和 Linux 上：~/.gradle/init.d/offline.gradle

    - 打开该文本文件并添加以下脚本：

        def reposDir = new File(System.properties['user.home'], ".android/manual-offline-m2")
            def repos = new ArrayList()
            reposDir.eachDir {repos.add(it) }
            repos.sort()

            allprojects {
              buildscript {
                repositories {
                  for (repo in repos) {
                    maven {
                      name = "injected_offline_${repo.name}"
                      url = repo.toURI().toURL()
                    }
                  }
                }
              }
              repositories {
                for (repo in repos) {
                  maven {
                    name = "injected_offline_${repo.name}"
                    url = repo.toURI().toURL()
                  }
                }
              }
            }

    - 保存该文本文件。
    - （可选）如果您想要验证离线组件是否运行正常，请从项目的 build.gradle 文件中移除在线代码库（如下所示）。在确认项目不使用这些代码库也能正确编译之后，您可以将它们放回到 build.gradle 文件中。

        buildscript {
                repositories {
                    // Hide these repositories to test your build against
                    // the offline components. You can include them again after
                    // you've confirmed that your project builds ‘offline’.
                    // google()
                    // jcenter()
                }
                ...
            }
            allprojects {
                repositories {
                    // google()
                    // jcenter()
                }
                ...
            }


https://www.cnblogs.com/diyishijian/p/7751407.html
C:\Users\wood\.android\manual-offline-m2\android-gradle-plugin-3.5.0-beta01\org\jetbrains\kotlin\kotlin-stdlib-jdk8
C:\Users\wood\.android\manual-offline-m2\gmaven_stable\com\android\tools\build\gradle



