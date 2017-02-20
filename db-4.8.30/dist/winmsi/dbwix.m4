<!-- $Id$
 - This file is included by WiX input files to define m4 macros.
 - m4 is tricky.  It has NO notion of XML comments, so
 - take care using these names in comments after they are defined,
 - since they will be expanded (probably what you don't want).
 -
 - Note that this file is shared by multiple installers.
 - If you want to change a definition to customize an individual project,
 - consider redefining the macro in a local file.
 -->

<!-- Some basic UI characteristics -->
m4_define(`DIALOG_WIDTH', `390')
m4_define(`DIALOG_HEIGHT', `320')
m4_define(`BOTTOMSTRIPE_Y', `m4_eval(DIALOG_HEIGHT-32)')
m4_define(`NAVBUTTON_Y', `m4_eval(DIALOG_HEIGHT-23)')
m4_define(`NAVBUTTON_DIM', `X="`$1'" Y="NAVBUTTON_Y" Width="66" Height="17"')

<!-- _YPOS is a running total of the current Y position -->
m4_define(`_YPOS', `0')
m4_define(`SETY', `m4_define(`_YPOS', `$1')')
m4_define(`INCY', `SETY(m4_eval(_YPOS+(`$1')))')

<!-- PARTIALHEIGHT(yheight [, gap=10 ]) -->
m4_define(`PARTIALHEIGHT', `Y="_YPOS" Height="`$1'" INCY(`$1') INCY(_GETGAP(`$2'))')
m4_define(`_GETGAP', `m4_ifelse(`',`$1', 10, `$1')')
m4_define(`FULLHEIGHT', `Y="_YPOS" Height="m4_eval(BOTTOMSTRIPE_Y - _YPOS - 10)"')

<!-- BOTTOM_Y:  bottom of the usable area before nav buttons -->
m4_define(`BOTTOM_Y', `m4_eval(BOTTOMSTRIPE_Y - 10)')

m4_define(`DIALOGPROP', `Width="DIALOG_WIDTH" Height="DIALOG_HEIGHT"
        Title="[ProductName] - Installer" NoMinimize="yes"')

m4_define(`TOPSTRIPE', `
        SETY(`$1')
        INCY(10)
<!-- stripe bitmap removed for now until we get better quality bitmaps.
        <Control Id="TopStripe" Type="Bitmap"
                 X="0" Y="0" Width="DIALOG_WIDTH" Height="`$1'" Text="Stripe" />
-->
        <Control Id="TopStripeBorder" Type="Line"
                 X="0" Y="`$1'" Width="DIALOG_WIDTH" Height="0" />
        <Control Id="TopTitle" Type="Text"
                 X="8" Y="6" Width="292" Height="25" Transparent="yes">
          <Text>{&amp;MSSansBold8}`$2'</Text>
        </Control>
        <Control Id="TopText" Type="Text"
                 X="16" Y="23" Width="m4_eval(DIALOG_WIDTH-34)"
                 Height="m4_eval(`$1' - 19)" Transparent="yes">
          <Text>`$3'</Text>
        </Control>')

m4_define(`TEXTCONTROL', `
        <Control Id="`$1'" Type="Text"
            X="20" Y="_YPOS" Width="m4_eval(DIALOG_WIDTH-42)" Height="`$2'">
          <Text>`$3'</Text>
        </Control>
        INCY(`$2')')

<!-- TEXTCONTROL2 (name,height,text). No newline -->
m4_define(`TEXTCONTROL2', `
        <Control Id="`$1'" Type="Text"
            X="20" Y="_YPOS" Width="m4_eval(DIALOG_WIDTH-42)" Height="`$2'" Transparent="yes">
          <Text>`$3'</Text>
        </Control>')

m4_define(`BOTTOMSTRIPE', `
        <Control Id="BottomStripeBorder" Type="Line"
                 X="0" Y="BOTTOMSTRIPE_Y" Width="DIALOG_WIDTH" Height="0" />')

m4_define(`NEWDIALOGEVENT', `
          <Publish Event="NewDialog" Value="`$1'">
                   <![CDATA[1]]>
          </Publish>')

m4_define(`SPAWNDIALOGEVENT', `
          <Publish Event="SpawnDialog" Value="`$1'">
                   <![CDATA[1]]>
          </Publish>')

<!-- use an arrow only if the text is Back, like this: "< Back" -->
m4_define(`BACKBUTTON_GENERIC', `
        <Control Id="`$1'" Type="PushButton"
                 NAVBUTTON_DIM(170)
                 Text="m4_ifelse(Back,`$1',&lt; )&amp;`$1'" `$2'>
           `$3'
        </Control>')

<!-- use an arrow only if the text is Next, like this: "Next >" -->
m4_define(`NEXTBUTTON_GENERIC', `
        <Control Id="`$1'" Type="PushButton"
                 NAVBUTTON_DIM(236)
                 Default="yes" Text="&amp;`$1'm4_ifelse(Next,`$1', &gt;)" `$2'>
           `$3'
        </Control>')

m4_define(`CANCELBUTTON_GENERIC', `
        <Control Id="`$1'" Type="PushButton"
                 NAVBUTTON_DIM(308)
                 Text="`$1'" `$2'>
          `$3'
        </Control>')

m4_define(`NEXTBUTTON_NOTDEFAULT', `
        <Control Id="`$1'" Type="PushButton"
                 NAVBUTTON_DIM(236)
                 Default="no" Text="&amp;`$1'm4_ifelse(Next,`$1', &gt;)" `$2'>
           `$3'
        </Control>')

<!-- typical usages -->
m4_define(`BACKBUTTON_DISABLED', `BACKBUTTON_GENERIC(Back, Disabled="yes")')
m4_define(`BACKBUTTON', `BACKBUTTON_GENERIC(Back, , NEWDIALOGEVENT(`$1'))')
m4_define(`NEXTBUTTON_DISABLED', `NEXTBUTTON_GENERIC(Next, Disabled="yes")')
m4_define(`NEXTBUTTON', `NEXTBUTTON_GENERIC(Next, , NEWDIALOGEVENT(`$1'))')
m4_define(`CANCELBUTTON', `CANCELBUTTON_GENERIC(Cancel, Cancel="yes",
                          SPAWNDIALOGEVENT(CancelInstallerDlg))')

<!-- a little (imperfect) magic to create some unique GUIDs. -->
m4_define(`_GUIDSUFFIX', `10000000')
m4_define(`_SETGUID', `m4_define(`_GUIDSUFFIX', `$1')')
m4_define(`_GUIDINC', `_SETGUID(m4_eval(_GUIDSUFFIX+1))')
m4_define(`GUID_CREATE_UNIQUE', `_GUIDINC()WIX_DB_GUID_PREFIX()`'_GUIDSUFFIX()')

<!-- These three defines are data values, used by GUID_CREATE_PERSISTENT -->
m4_define(`_WIXDB_PRODUCT', WIX_DB_PRODUCT_NAME)
m4_define(`_WIXDB_VERSION', WIX_DB_VERSION)
m4_define(`_WIXDB_CURDIR', `unknown')
m4_define(`_WIXDB_CURFILE', `unknown')
m4_define(`_WIXDB_SUBDIR', `')

<!-- These defines set the data values above -->
m4_define(`WIX_DB_SET_PRODUCT', `m4_define(`_WIXDB_PRODUCT', `$1')')
m4_define(`WIX_DB_SET_VERSION', `m4_define(`_WIXDB_VERSION', `$1')')
m4_define(`WIX_DB_SET_CURDIR', `m4_define(`_WIXDB_CURDIR', `$1')')
m4_define(`WIX_DB_SET_CURFILE', `m4_define(`_WIXDB_CURFILE', `$1')')
m4_define(`WIX_DB_SET_SUBDIR', `m4_define(`_WIXDB_SUBDIR', `$1')')

m4_define(`_LASTCHAR', `m4_substr(`$1',m4_eval(m4_len(`$1')-1))')
m4_define(`_LOPCHAR', `m4_substr(`$1',0,m4_eval(m4_len(`$1')-1))')
m4_define(`_CHOPNAME', `m4_ifelse(_LASTCHAR(`$1'),/,`$1',`_CHOPNAME(_LOPCHAR(`$1'))')')
m4_define(`WIX_DB_BEGIN_SUBDIR', `WIX_DB_SET_SUBDIR(_WIXDB_SUBDIR/`$1')')
m4_define(`WIX_DB_END_SUBDIR', `WIX_DB_SET_SUBDIR(_LOPCHAR(_CHOPNAME(_WIXDB_SUBDIR)))')
m4_define(`WIX_DB_CLEAR_SUBDIR', `WIX_DB_SET_SUBDIR()')

<!-- Create a GUID from the current product, directory, file -->
m4_define(`WIX_DB_PERSISTENT_GUID', `m4_esyscmd(echo "_WIXDB_PRODUCT @@ _WIXDB_VERSION @@ _WIXDB_CURDIR @@ _WIXDB_SUBDIR @@ _WIXDB_CURFILE" | openssl md5 | sed -e "s/^\(........\)\(....\)\(....\)\(....\)\(....\)\(............\)/\1-\2-\3-\4-\5/")')

m4_define(`DB_LICENSE_INTRO', `The following license applies to this copy of software you are about to install.  Please read it carefully before proceeding.  Select below the nature of the license by which you will use this product.  For more information about Oracle Corporation&apos;s licensing please contact us at berkeleydb-info_us@oracle.com')

m4_define(`DB_ENVIRONMENT_INTRO', `[ProductName] will need to modify certain environment variables to work properly.  If you elect not to set these variables you may find that some utilities`,' scripts and other parts of [ProductName] won&apos;t work properly.  Please indicate that you skipped this step if you request support help from us.')

m4_define(`COMMON_PROPERTIES', `
    <Property Id="ApplicationUsers"><![CDATA[AnyUser]]></Property>
    <Property Id="LicenseType"><![CDATA[Open]]></Property>

    <!-- The ARP* properties affect the Add/Remove Programs dialog -->
    <Property Id="ARPURLINFOABOUT"><![CDATA[http://www.oracle.com]]></Property>
    <Property Id="ARPCONTACT"><![CDATA[berkeleydb-info_us@oracle.com]]></Property>
    <Property Id="ARPNOMODIFY"><![CDATA[1]]></Property>
    <Property Id="ARPNOREPAIR"><![CDATA[1]]></Property>
    <!-- TODO: this icon does not work here -->
    <Property Id="ARPPRODUCTION"><![CDATA[IconWeb]]></Property>

    <Property Id="INSTALLLEVEL"><![CDATA[200]]></Property>
    <Property Id="FullOrCustom"><![CDATA[Full]]></Property>

    <Property Id="DiscussionCheck" Hidden="yes"><![CDATA[yes]]></Property>
    <Property Id="AnnouncementsCheck" Hidden="yes"><![CDATA[yes]]></Property>
    <Property Id="NewsletterCheck" Hidden="yes"><![CDATA[yes]]></Property>
    <Property Id="EmailAddress" Hidden="yes"></Property>
    <Property Id="SalesContactCheck" Hidden="yes"><![CDATA[yes]]></Property>
    <Property Id="EnvironmentSetCheck" Hidden="yes"><![CDATA[1]]></Property>
    <Property Id="EnvironmentGenCheck" Hidden="yes"><![CDATA[1]]></Property>
<!-- (PBR) We use DebugCheck to track the state of the debug checkbox -->
    <Property Id="DebugCheck" Hidden="yes"><![CDATA[yes]]></Property>

    <!-- Part of the build process creates a program instenv.exe
      -  that is installed into InstUtil and used only by the installer.
      -  When a user wants to generate a file with environment vars,
      -  we launch instreg and that program creates it.
      -
      -  The final location of the instenv.exe program is not known
      -  when we create this property, we set the real value of the
      -  property later.
      -->

    <Property Id="InstEnvironmentProgram"><![CDATA[0]]></Property>
<!-- TODO: should not have to hardwire PATH and CLASSPATH here -->
    <CustomAction Id="InstEnvironment" Property="InstEnvironmentProgram" 
         ExeCommand="[INSTALLDIR]\dbvars.bat PATH=[PATHEscValue] CLASSPATH=[CLASSPATHEscValue]" Return="asyncNoWait"/>

    <!-- Some properties to aid in debugging.
      -  Sometimes creating a msg dialog is the easiest way to see the
      -  value of a property.  To make this work when you hit the next
      -  button, add this to your NEXTBUTTON__GENERIC call:

          <Publish Event="DoAction" Value="InstDebug"><![CDATA[1]]></Publish>

      -->
    <Property Id="DebugUserId">dda</Property>
    <Property Id="DebugShowProgram">msg.exe</Property>

    <!-- tweek me as needed -->
    <CustomAction Id="InstDebug" Property="DebugShowProgram" 
         ExeCommand="[DebugUserId] InstEnvironmentProgram=[InstEnvironmentProgram]= EnvironmentGenCheck=[EnvironmentGenCheck]= AlwaysInstall=[!AlwaysInstall]=" Return="asyncNoWait" />

    <Property Id="NULL" Hidden="yes"></Property>
    <Property Id="FeatureList" Hidden="yes"></Property>
    <Property Id="DoInstallDebug" Hidden="yes">yes</Property>
    <Property Id="DoInstallEnvironment" Hidden="yes">yes</Property>

    <Binary Id="OracleLogo" src="WIX_DB_IMAGEDIR\oracle.jpg" />
    <Binary Id="Stripe" src="WIX_DB_IMAGEDIR\topstripe.ibd" />

    <!-- TODO: does not work yet -->
    <Binary Id="IconWeb" src="WIX_DB_IMAGEDIR\caticon.ibd" />

    <!-- These are 16x16 Windows ico files -->
    <Binary Id="IconCreateDir" src="WIX_DB_IMAGEDIR\foldernew.ibd" />
    <Binary Id="IconUp" src="WIX_DB_IMAGEDIR\folderup.ibd" />

')

m4_define(`COMMON_COMPONENTS', `
        <Component Id="RequiredCommonFiles"
               Guid="545CFE00-93D7-11D9-EAD3-F63F68BDEB1A"
                   KeyPath="yes" SharedDllRefCount="yes"
                   Location="either" DiskId="1">
	      <Registry Id="Ext.Registry" Root="HKCR"
                   Key=".bdbsc"
                   Value="Oracle.InformationalShortcut"
                   Type="string" Action="write" />
	      <Registry Id="Name.Registry" Root="HKCR"
                   Key="Oracle.InformationalShortcut"
                   Value="Oracle Corporation Informational Shortcut"
                   Type="string" Action="write" />
	      <Registry Id="Tip.Registry" Root="HKCR"
                   Key="Oracle.InformationalShortcut" Name="InfoTip"
                   Value="Oracle Corporation Informational Shortcut"
                   Type="string" Action="write" />
	      <Registry Id="NoShow.Registry" Root="HKCR"
                   Key="Oracle.InformationalShortcut" Name="NeverShowExt"
                   Type="string" Action="write" />
	      <Registry Id="Icon.Registry" Root="HKCR"
                   Key="Oracle.InformationalShortcut\DefaultIcon"
                   Value="[INSTALLDIR]\installutil\webicon.ico"
                   Type="string" Action="write" />
	      <Registry Id="Command.Registry" Root="HKCR"
                   Key="Oracle.InformationalShortcut\shell\open\command"
                   Value="rundll32.exe shdocvw.dll,OpenURL %1"
                   Type="string" Action="write" />
	      <Registry Id="Explore.Registry" Root="HKCU"
                   Key="Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.bdbsc\OpenWithProgIds\Oracle.InformationalShortcut"
                   Action="write" />
	      <Registry Id="HklmExt.Registry" Root="HKLM"
                   Key="Software\Classes\.bdbsc"
                   Value="Oracle.InformationalShortcut"
                   Type="string" Action="write" />
	      <Registry Id="HklmCommand.Registry" Root="HKLM"
                   Key="Software\Classes\Oracle.InformationalShortcut\shell\open\command"
                   Value="rundll32.exe shdocvw.dll,OpenURL %1"
                   Type="string" Action="write" />
            </Component>
')

m4_define(`COMMON_UI_TEXT', `
      <!-- These are needed to show various canned text -->
      <UIText Id="AbsentPath" />
      <UIText Id="NewFolder">Fldr|New Folder</UIText>
      <UIText Id="bytes">bytes</UIText>
      <UIText Id="GB">GB</UIText>
      <UIText Id="KB">KB</UIText>
      <UIText Id="MB">MB</UIText>
      <UIText Id="MenuAbsent">This feature will not be installed.</UIText>
      <UIText Id="MenuAllLocal">This feature, and all subfeatures, will be installed.</UIText>
      <UIText Id="MenuLocal">This feature will be installed.</UIText>
      <UIText Id="SelAbsentAbsent">This feature will remain uninstalled.</UIText>
      <UIText Id="SelAbsentLocal">This feature will be installed.</UIText>
      <UIText Id="SelChildCostNeg">This feature frees up [1] on your hard drive.</UIText>
      <UIText Id="SelChildCostPos">This feature requires [1] on your hard drive.</UIText>
      <UIText Id="SelCostPending">Compiling cost for this feature...</UIText>
      <UIText Id="SelLocalAbsent">This feature will be completely removed.</UIText>
      <UIText Id="SelLocalLocal">This feature will remain installed.</UIText>
      <UIText Id="SelParentCostNegNeg">This feature frees up [1] on your hard drive. It has [2] of [3] subfeatures selected. The subfeatures free up [4] on your hard drive.</UIText>
      <UIText Id="SelParentCostNegPos">This feature frees up [1] on your hard drive. It has [2] of [3] subfeatures selected. The subfeatures require [4] on your hard drive.</UIText>
      <UIText Id="SelParentCostPosNeg">This feature requires [1] on your hard drive. It has [2] of [3] subfeatures selected. The subfeatures free up [4] on your hard drive.</UIText>
      <UIText Id="SelParentCostPosPos">This feature requires [1] on your hard drive. It has [2] of [3] subfeatures selected. The subfeatures require [4] on your hard drive.</UIText>
      <UIText Id="TimeRemaining">Time remaining: {[1] min }[2] sec</UIText>
      <UIText Id="VolumeCostAvailable">Available</UIText>
      <UIText Id="VolumeCostDifference">Differences</UIText>
      <UIText Id="VolumeCostRequired">Required</UIText>
      <UIText Id="VolumeCostSize">Disk Size</UIText>
      <UIText Id="VolumeCostVolume">Volume</UIText>
')

m4_define(`COMMON_FEATURES', `
    <!-- Here we list all the features to be installed.
      -  There is one canned feature, the rest of the
      -  features come from features.in, by way of a file
      -  that gets included.
      -->
    <Feature Id="AlwaysInstall" Title="Always Install"
             Description="`$1'" Display="hidden" Level="1"
             AllowAdvertise="no"
             ConfigurableDirectory="INSTALLDIR" Absent="disallow">
      <ComponentRef Id="RequiredFiles" />
      <ComponentRef Id="RequiredCommonFiles" />
    </Feature>

    <!-- <Feature>, <ComponentRef> generated from {features,files}.in -->
    WIX_DB_FEATURE_STRUCTURE()
')

m4_define(`COMMON_EXECUTE_SEQUENCE', `
    <!-- TODO: fix comment
      -  We modify the execute sequence to insert some custom actions:
      -  we want the instenv program to run during install (after files
      -  are installed), and during uninstall (before files are removed).
      -  We set a condition on the custom actions to make this happen.
      -  The "!ident" notation indicates the current action for a feature 
      -  with that identifier.  We use the AlwaysInstall feature because
      -  it is always present in our feature list.  A value of 3 means
      -  it is being installed locally, 1 means it is being uninstalled.
      - TODO: removed for now
      <Custom Action="InstEnvironment" After="PublishProduct">
        <![CDATA[Not Installed]]></Custom>
      -->
    <InstallExecuteSequence>
    </InstallExecuteSequence>
')

<!--
  -  Here are macros for each dialog that is shared by the installers.
  -  The idea for any customization is that each installer
  -  could potentially override a particular dialog.
  -  However, if it is feasible to share code, any of these
  -  dialogs could be parameterized further.
  -
  -  In general, these macros have 3 parameters.
  -  The first is the id name of the dialog.
  -  The second is the id of the previous dialog (for the Back button).
  -  The third is the id of the next dialog (for the Next button).
  -->
m4_define(`DIALOG_WELCOME', `
      <Dialog Id="`$1'" DIALOGPROP>
        BOTTOMSTRIPE()
        BACKBUTTON_DISABLED()
        CANCELBUTTON()
        NEXTBUTTON(`$3')

        TOPSTRIPE(54, `Welcome',
`The Installer will install [ProductName] on your computer.
To continue, click Next.')

        <Control Id="Logo" Type="Bitmap" Text="OracleLogo" 
                 X="0" Width="DIALOG_WIDTH" PARTIALHEIGHT(168) />
      </Dialog>
')

<!-- Takes a 4th parameter, a short product name, like "Berkeley DB" -->
m4_define(`DIALOG_LICENSE', `
      <RadioButtonGroup Property="LicenseType">
        <RadioButton Value="Open"
           X="0" Y="0" Width="310" Height="15"
           Text="I qualify for the &amp;open source license shown above" />
        <RadioButton Value="Commercial"
           X="0" Y="15" Width="310" Height="15"
           Text="I will need a co&amp;mmercial license when I ship my product" />
      </RadioButtonGroup>

      <Dialog Id="`$1'" DIALOGPROP>

        TOPSTRIPE(84, `Open Source License', DB_LICENSE_INTRO(`$4'))
        BOTTOMSTRIPE()
        BACKBUTTON(`$2')
        CANCELBUTTON()
	NEXTBUTTON(`$3')

        <Control Id="LicenseText" Type="ScrollableText" X="8" Width="368"
                  PARTIALHEIGHT(130) Sunken="yes">
          <Text>WIX_DB_LICENSE_RTF()</Text>
        </Control>

        <Control Id="LicenseRadio" Type="RadioButtonGroup" X="8" Width="340"
                 PARTIALHEIGHT(35) Property="LicenseType" />

      </Dialog>
')

m4_define(`DIALOG_TARGET', `

      <RadioButtonGroup Property="ApplicationUsers">
        <RadioButton Value="AnyUser" X="0" Y="0" Width="270" Height="15"
            Text="&amp;Anyone who uses this computer (all users)" />
        <RadioButton Value="CurUser" X="0" Y="15" Width="270" Height="15"
            Text="Only for the current user" />
      </RadioButtonGroup>

      <Dialog Id="`$1'" DIALOGPROP>

        TOPSTRIPE(44, `Installation Folder',
                  `Click Next to install to the default folder.')
        BOTTOMSTRIPE()
        BACKBUTTON(`$2')
        CANCELBUTTON()

        NEXTBUTTON_GENERIC(Next,,
	   NEWDIALOGEVENT(`$3')
          <Publish Event="SetInstallLevel" Value="300">
            <![CDATA[0]]></Publish>
          <Publish Property="ALLUSERS" Value="1">
            <![CDATA[ApplicationUsers = "AnyUser"]]></Publish>
          <Publish Property="ALLUSERS" Value="{}">
            <![CDATA[ApplicationUsers = "CurUser"]]></Publish>
          <Publish Property="SelectedSetupType" Value="Custom">
            <![CDATA[1]]></Publish>

	)

        TEXTCONTROL(InstallToText, 16, `Install [ProductName] to:')

        <Control Id="ChangeFolder" Type="PushButton" X="318" Y="_YPOS"
                 Width="66" Height="17" Text="&amp;Change...">
          <Publish Event="SpawnDialog" Value="ChangeFolderDlg">
            <![CDATA[1]]></Publish>
          <Publish Property="NewInstallDir" Value="INSTALLDIR">
            <![CDATA[1]]></Publish>
        </Control>

        <Control Id="InstallToValue" Type="Text" X="40" Width="250"
           PARTIALHEIGHT(20, 20)
           Property="NewInstallDir" Text="[INSTALLDIR]" />

        TEXTCONTROL(InstallForText, 14, `Install [ProductName] for:')

        <Control Id="InstallForRadio" Type="RadioButtonGroup" PARTIALHEIGHT(50)
             X="40" Width="310" Property="ApplicationUsers" />
      </Dialog>

      <Dialog Id="ChangeFolderDlg" DIALOGPROP>
        TOPSTRIPE(44, `Change the Installation Folder',
                  `Browse to the folder you want to install to.')
        BOTTOMSTRIPE()
        NEXTBUTTON_GENERIC(OK,,
          <Publish Event="SetTargetPath"
            Value="[NewInstallDir]"><![CDATA[1]]></Publish>
          <Publish Event="EndDialog" Value="Return"><![CDATA[1]]></Publish>
        )
        CANCELBUTTON_GENERIC(Cancel, Cancel="yes",
          <Publish Event="Reset" Value="0"><![CDATA[1]]></Publish>
          <Publish Event="EndDialog" Value="Return"><![CDATA[1]]></Publish>
        )

        TEXTCONTROL(LookText, 15, `&amp;Install into:')

        <Control Id="DirCombo" Type="DirectoryCombo"
           X="20" Width="270" Y="_YPOS" Height="80"
           Property="NewInstallDir" Indirect="yes" Removable="yes"
           Fixed="yes" Remote="yes" CDROM="yes" RAMDisk="yes" Floppy="yes" />

        <Control Id="FolderUp" Type="PushButton"
           X="320" Width="19" Y="_YPOS" Height="19"
           ToolTip="Up One Level" Icon="yes" FixedSize="yes"
           IconSize="16" Text="IconUp">
          <Publish Event="DirectoryListUp" Value="0"><![CDATA[1]]></Publish>
        </Control>

        <Control Id="FolderCreate" Type="PushButton"
           X="345" Width="19" Y="_YPOS" Height="19"
           ToolTip="Create New Folder" Icon="yes" FixedSize="yes"
           IconSize="16" Text="IconCreateDir">
          <Publish Event="DirectoryListNew" Value="0"><![CDATA[1]]></Publish>
        </Control>

        INCY(25)
        <Control Id="DirList" Type="DirectoryList"
           X="20" Width="342" PARTIALHEIGHT(100, 5)
           Property="NewInstallDir" Sunken="yes"
           Indirect="yes" TabSkip="no" />

        <Control Id="FolderText" Type="Text"
           X="20" Width="99" PARTIALHEIGHT(14, 1)
           TabSkip="no" Text="&amp;Folder name:" />

        <Control Id="PathEditControl" Type="PathEdit"
           X="20" Width="342" PARTIALHEIGHT(17)
           Property="NewInstallDir" Sunken="yes" Indirect="yes" />

      </Dialog>

')

<!-- Takes a 4th parameter, any extra text (restrictions) for debug libs -->
m4_define(`DIALOG_FEATURE', `
      <Dialog Id="`$1'" DIALOGPROP TrackDiskSpace="yes">

        TOPSTRIPE(36, `Feature Selection',
                  `Select the features of [ProductName] you want. Maximum install size is [MaxInstallSize].')
        BOTTOMSTRIPE()
        CANCELBUTTON
        BACKBUTTON(`$2')

        NEXTBUTTON_GENERIC(Next,,
          <Publish Event="NewDialog" Value="`$3'">
            <![CDATA[OutOfNoRbDiskSpace <> 1]]></Publish>
          <Publish Event="NewDialog" Value="OutOfSpaceDlg">
            <![CDATA[OutOfNoRbDiskSpace = 1]]></Publish>
          <Publish Property="FullOrCustom" Value="Custom">
            <![CDATA[1]]></Publish>

           <!--
            -  This updates the FeatureList property and the
            -  properties like PATHValue that track the value
            -  to be displayed for environment variables.
           -->
            WIX_DB_ENV_FEATURE_SET()
        )


        <!-- TODO: When the debug checkbox is clicked,
          -  we would like to update the disk space usage numbers
          -  as shown in the SelectionTreeControl.  Tried this:
              <Publish Event="DoAction" Value="CostFinalize">
                 <![CDATA[1]]></Publish>
          -  but it made all the numbers zero.  Probably need
          -  to perform a whole sequence, (like CostInitialize,...)
          -->


        TEXTCONTROL(ClickText, 15,
`Click on an icon in the list below to change how a feature is installed.')
        INCY(5)

        <Control Id="SelectionTreeControl" Type="SelectionTree"
                 X="8" Width="220" FULLHEIGHT
                 Property="NewInstallDir" Sunken="yes" TabSkip="no" />

        <Control Id="GroupBoxControl" Type="GroupBox"
                 X="235" Width="131" FULLHEIGHT
                 Text="Feature Description" />
        INCY(15)

        <Control Id="ItemDescription" Type="Text"
                 X="241" Width="120" PARTIALHEIGHT(50) >
          <Text></Text>
          <Subscribe Event="SelectionDescription" Attribute="Text" />
        </Control>

        <Control Id="Size" Type="Text"
                 X="241" Width="120" PARTIALHEIGHT(50)
                 Text="Feature size">
          <Subscribe Event="SelectionSize" Attribute="Text" />
        </Control>

      </Dialog>
')

<!--
 - Note: for Win/9X, Win/ME
 - Here we must do costfinalize whenever leaving
 - this dialog (via Back or Next) because changing whether we have
 - environment enabled or not changes the list of features
 - (which is finalized by costfinalize).
 - Calling costfinalize more than once is apparently not
 - supported on older (9X,ME) systems.
 -->
m4_define(`DIALOG_ENVIRONMENT', `
      <Dialog Id="`$1'" DIALOGPROP>
        TOPSTRIPE(84, `Setting Environment Variables', DB_ENVIRONMENT_INTRO)
        BOTTOMSTRIPE()
        CANCELBUTTON
        BACKBUTTON_GENERIC(Back, ,
           NEWDIALOGEVENT(`$2')
<!--PBR (4/4/2005) I removed this because it resets the feature choices
           <Publish Event="DoAction" Value="CostFinalize">
             <![CDATA[1]]></Publish>
-->
        )
        NEXTBUTTON_GENERIC(Next, , 
           NEWDIALOGEVENT(`$3')
<!--PBR (4/4/2005) I removed this because it resets the feature choices
           <Publish Event="DoAction" Value="CostFinalize">
             <![CDATA[1]]></Publish>
-->
          <Publish Property="DoInstallEnvironment" Value="yes">
                 <![CDATA[EnvironmentSetCheck = "1"]]></Publish>
          <Publish Property="DoInstallEnvironment" Value="no">
                 <![CDATA[EnvironmentSetCheck <> "1"]]></Publish>
        )

        <Control Id="SetEnvBox" Type="CheckBox" PARTIALHEIGHT(15, 5)
             Text="Set the values in the environment variables"
             X="26" Width="250" Property="EnvironmentSetCheck" CheckBoxValue="1"/>
        <Control Id="GenEnvBox" Type="CheckBox" PARTIALHEIGHT(15, 8)
             Text="Generate a dbvars.bat file with the given values"
             X="26" Width="250" Property="EnvironmentGenCheck" CheckBoxValue="1"/>
        INCY(5)

        TEXTCONTROL(ReviewText, 12,
`Here are the new environment values:')

        <Control Id="LargeBox" Type="Text"
                  X="19" Width="340" FULLHEIGHT
                  Disabled="yes" Sunken="yes" Transparent="yes" TabSkip="no" />

        INCY(5)

        <!-- Show the properties for environment -->
        WIX_DB_ENV_FEATURE_SHOW()

      </Dialog>
')

m4_define(`DIALOG_READY', `
      <Dialog Id="`$1'" DIALOGPROP TrackDiskSpace="yes">
        NEXTBUTTON_GENERIC(Install,,
          <Publish Event="NewDialog" Value="OutOfSpaceDlg">
            <![CDATA[OutOfNoRbDiskSpace = 1]]></Publish>
          <Publish Event="EndDialog" Value="Return">
            <![CDATA[OutOfNoRbDiskSpace <> 1]]></Publish>

<!-- Note: we set the name of the instenv now because we do not know
           the installed pathname at the beginning of the execution -->

          <Publish Property="InstEnvironmentProgram"
                 Value="[INSTALLDIR]\installutil\bin\instenv.exe">
                 <![CDATA[1]]></Publish>
        )

        TOPSTRIPE(44, `Ready', `The installer is ready to begin.')
        BOTTOMSTRIPE()
        CANCELBUTTON()
        BACKBUTTON(`$2')

        TEXTCONTROL(ReviewText, 24,
`If you want to review or change any of your installation settings, click Back to the Feature Selection.  Click Cancel to exit the installer.')

        <Control Id="LargeBox" Type="Text"
                  X="19" Width="340" FULLHEIGHT
                  Disabled="yes" Sunken="yes" Transparent="yes" TabSkip="no" />

        INCY(5)

        <Control Id="DestinationText" Type="Text"
                  X="23" Width="316" PARTIALHEIGHT(11, 4)
                  TabSkip="no" Text="Destination Folder:" />

        <Control Id="DestinationValue" Type="Text"
                  X="37" Width="316" PARTIALHEIGHT(13, 8)
                  TabSkip="no" Text="[INSTALLDIR]" />

        <Control Id="FeatureListText" Type="Text"
                  X="23" Width="316" PARTIALHEIGHT(13, 4)
                  TabSkip="no" Text="Features to be installed:" />

        <Control Id="FeatureListValue" Type="Text"
                  X="37" Width="316" PARTIALHEIGHT(30, 8)
                  TabSkip="no" Text="Shortcuts[FeatureList]" />

        <Control Id="EnvironmentText" Type="Text"
                  X="23" Width="316" PARTIALHEIGHT(15, 0)
                  TabSkip="no" Text="Environment Variables:" />

        <Control Id="EnvironmentValue" Type="Text"
                  X="37" Width="316" PARTIALHEIGHT(20, 0)
                  TabSkip="no" Text="[DoInstallEnvironment]" />

      </Dialog>

')

m4_define(`DIALOG_PROGRESS', `
      <Dialog Id="`$1'" DIALOGPROP Modeless="yes">
        BOTTOMSTRIPE()
        BACKBUTTON_DISABLED()
        CANCELBUTTON()
        NEXTBUTTON_DISABLED()

        TOPSTRIPE(44, `Installer Progress', `Installing [ProductName].')

        <Control Id="ActionText" Type="Text"
                  X="59" Y="100" Width="275" Height="12">
          <Subscribe Event="ActionText" Attribute="Text" />
        </Control>

        <Control Id="ActionProgress95" Type="ProgressBar"
                  X="59" Y="113" Width="275" Height="12"
                  ProgressBlocks="yes" Text="Progress done">
          <Subscribe Event="InstallFiles" Attribute="Progress" />
          <Subscribe Event="MoveFiles" Attribute="Progress" />
          <Subscribe Event="RemoveFiles" Attribute="Progress" />
          <Subscribe Event="RemoveRegistryValues" Attribute="Progress" />
          <Subscribe Event="WriteIniValues" Attribute="Progress" />
          <Subscribe Event="WriteRegistryValues" Attribute="Progress" />
          <Subscribe Event="UnmoveFiles" Attribute="Progress" />
          <Subscribe Event="AdminInstallFinalize" Attribute="Progress" />
          <Subscribe Event="SetProgress" Attribute="Progress" />
        </Control>
      </Dialog>
')

<!--
  -  Takes two extra parameters (in addition to the usual dialog parms)
  -  4th: a short product name, like "Berkeley DB"
  -  5th: a description of where to find online info, like "on www.xyz.com"
  -->
m4_define(`DIALOG_SUCCESS', `
      <Dialog Id="`$1'" DIALOGPROP>
        BOTTOMSTRIPE()
        NEXTBUTTON_GENERIC(Finish, Cancel="yes",
          <Publish Event="EndDialog" Value="Exit">
            <![CDATA[1]]></Publish>
        )
        CANCELBUTTON_GENERIC(Cancel, Disabled="yes", )
        BACKBUTTON_DISABLED()

        TOPSTRIPE(44, `Installed', `[ProductName] is now installed.')

        TEXTCONTROL(InstallSuccessText, 80,
`Please go to http://forums.oracle.com/forums/category.jspa?categoryID=18 for any technical issues or contact berkeleydb-info_us@oracle.com for sales and licensing questions.

Information about this product can also be found $5.

Thank you for installing [ProductName].')

        <Control Id="Image" Type="Bitmap" Text="OracleLogo"
                  X="0" Width="DIALOG_WIDTH" FULLHEIGHT TabSkip="no" />

      </Dialog>
')


m4_define(`DIALOG_ADMIN_INTERRUPTED', `
      <Dialog Id="`$1'" DIALOGPROP>
        TOPSTRIPE(44, `Interrupted',
`The installer was interrupted before [ProductName] could be completely installed.')

        BOTTOMSTRIPE()
        NEXTBUTTON_GENERIC(Finish, Cancel="yes",
          <Publish Event="EndDialog" Value="Exit">
            <![CDATA[1]]></Publish>
          <Condition Action="default">
            <![CDATA[NOT UpdateStarted]]></Condition>
        )
        CANCELBUTTON_GENERIC(Cancel, Disabled="yes",
          <Publish Property="Suspend" Value="1"><![CDATA[1]]></Publish>
          <Publish Event="EndDialog" Value="`$2'"><![CDATA[1]]></Publish>
          <Condition Action="disable"><![CDATA[NOT UpdateStarted]]></Condition>
          <Condition Action="enable"><![CDATA[UpdateStarted]]></Condition>
        )
        BACKBUTTON_GENERIC(Back, Disabled="yes",
          <Publish Property="Suspend" Value="{}"><![CDATA[1]]></Publish>
          <Publish Event="EndDialog" Value="`$2'"><![CDATA[1]]></Publish>
          <Condition Action="disable"><![CDATA[NOT UpdateStarted]]></Condition>
          <Condition Action="enable"><![CDATA[UpdateStarted]]></Condition>
          <Condition Action="default"><![CDATA[UpdateStarted]]></Condition>
        )

        <Control Id="NotModifiedText" Type="Text"
                  X="20" Y="_YPOS" Width="228" Height="50" Transparent="yes">
          <Text>Your system has not been modified.  To complete the installation later, please run the installer again.</Text>
          <Condition Action="hide"><![CDATA[UpdateStarted]]></Condition>
          <Condition Action="show"><![CDATA[NOT UpdateStarted]]></Condition>
        </Control>

        <Control Id="YesModifiedText" Type="Text"
                  X="20" Y="_YPOS" Width="228" Height="50" Transparent="yes">
          <Text>The product may be partially installed.  Any installed elements will be removed when you exit.</Text>
          <Condition Action="hide"><![CDATA[NOT UpdateStarted]]></Condition>
          <Condition Action="show"><![CDATA[UpdateStarted]]></Condition>
        </Control>
        INCY(30)

        TEXTCONTROL(FinishText, 25, `Click Finish to exit the install.')

        <Control Id="Image" Type="Bitmap"
                  X="0" Width="DIALOG_WIDTH" PARTIALHEIGHT(168) Text="OracleLogo" />
      </Dialog>
')

m4_define(`DIALOG_ADMIN_CANCEL', `

      <Dialog Id="`$1'" Width="280" Height="90"
              Title="[ProductName] - Installer" NoMinimize="yes">

        SETY(20)
        TEXTCONTROL(CancelText, 24,
`Are you sure you want to cancel [ProductName] installation?')

        <Control Id="YesButton" Type="PushButton"
                  X="60" Y="60" Width="66" Height="17" Text="&amp;Yes">
          <Publish Event="EndDialog" Value="Exit"><![CDATA[1]]></Publish>
        </Control>

        <Control Id="NoButton" Type="PushButton"
                  X="130" Y="60" Width="66" Height="17"
                  Default="yes" Cancel="yes" Text="&amp;No">
          <Publish Event="EndDialog" Value="Return">
                 <![CDATA[1]]></Publish>
        </Control>

      </Dialog>
')

m4_define(`DIALOG_ADMIN_NOSPACE', `
      <Dialog Id="`$1'" DIALOGPROP>
        TOPSTRIPE(44, `Out of Disk Space',
`The disk does not have enough space for the selected features.')
        BOTTOMSTRIPE()

        CANCELBUTTON_GENERIC(OK, Default="yes" Cancel="yes",
          <Publish Event="NewDialog" Value="`$2'">
             <![CDATA[ACTION <> "ADMIN"]]></Publish>
        )

        TEXTCONTROL(NoSpaceText, 43,
`The highlighted volumes (if any) do not have enough disk space for the currently selected features.  You can either remove files from the highlighted volumes, or choose to install fewer features, or choose a different destination drive.')

        <Control Id="VolumeCostListControl" Type="VolumeCostList"
                  X="23" Width="310" FULLHEIGHT
                  Sunken="yes" Fixed="yes" Remote="yes">
          <Text>{120}{70}{70}{70}{70}</Text>
        </Control>
      </Dialog>
')
