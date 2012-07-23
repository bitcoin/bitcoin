
macdeployqtplus works best on OS X Lion, for Snow Leopard you'd need to install
Python 2.7 and make it your default Python installation.

You will need the appscript package for the fancy disk image creation to work.
Install it by invoking "sudo easy_install appscript".

Ths script should be invoked in the target directory like this:
$source_dir/contrib/macdeploy/macdeployqtplus Bitcoin-Qt.app -add-qt-tr da,de,es,hu,ru,uk,zh_CN,zh_TW -dmg -fancy $source_dir/contrib/macdeploy/fancy.plist -verbose 2

During the process, the disk image window will pop up briefly where the fancy
settings are applied. This is normal, please do not interfere.

You can also set up Qt Creator for invoking the script. For this, go to the
"Projects" tab on the left side, switch to "Run Settings" above and add a
deploy configuration. Next add a deploy step choosing "Custom Process Step".
Fill in the following.

Enable custom process step: [x]
Command: %{sourceDir}/contrib/macdeploy/macdeployqtplus
Working directory: %{buildDir}
Command arguments: Bitcoin-Qt.app -add-qt-tr da,de,es,hu,ru,uk,zh_CN,zh_TW -dmg -fancy %{sourceDir}/contrib/macdeploy/fancy.plist -verbose 2

After that you can start the deployment process through the menu with
Build -> Deploy Project "bitcoin-qt"

