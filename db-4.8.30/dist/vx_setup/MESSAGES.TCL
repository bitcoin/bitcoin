# MESSAGES.TCL - All setup strings.

# modification history
# --------------------
# 03q,20apr99,bjl  added release notes message for backward compatibility 
#                  page.
# 03p,12apr99,wmd  Add word about simulator in message about the drivers
#                  object product.
# 03o,03mar99,tcy  Adjust setup directory size based on platform (fix for
#                  SPR 25228)
# 03n,24feb99,tcy  modified DLL update messages 
# 03m,22feb99,tcy  modified to align messages
# 03l,17feb99,tcy  modified message in the finish page for program group
#                  installation
# 03k,11feb99,tcy  added messages for backward compatibility page
# 03j,25jan99,tcy  added messages from INSTW32.TCL
# 03i,25jan99,wmd  Reword the message for 5010_DRIVERS_INFO.
# 03h,09dec98,bjl  added messages about manufacturers updating patches.
# 03g,01dec98,wmd  Fix typos.
# 03f,23nov98,tcy  warn user to disable virus protection on Welcome screen
# 03e,19nov98,wmd  fixed minor nits in wording.
# 03d,19nov98,bjl  added web site locations for patchinfo.
# 03c,18nov98,bjl  added formatted patch messages for patchinfo file.  
# 03b,12nov98,tcy  added message for not saving installation key
# 03a,10nov98,tcy  added warning message for space in destination directory
#                  removed message for checking temporary disk space
# 02z,27oct98,bjl  added recommended patch messages, modified required msg.
# 02y,26oct98,tcy  added message for checking temporary disk space
# 02x,22oct98,wmd  fix messages for clarity.
# 02w,21oct98,wmd  fix message for drv/obj.
# 02v,20oct98,tcy  added message for updating system and changed dcom message
# 02u,20oct98,bjl  added tornado registry name entry message. 
# 02t,19oct98,bjl  added tornado registry description message.
# 02s,16oct98,wmd  add new message for driver product warning.
# 02r,16oct98,wmd  fixed README.TXT description.
# 02q,12oct98,tcy  removed extraneous "the" from messages
# 02p,06oct98,tcy  added CD description to Welcome page 
# 02o,29sep98,bjl  added required patches message 5000_PATCHES_TEXT.
# 02n,29sep98,wmd  add text for readme page
# 02m,29sep98,tcy  refined DLL registration page text
# 02l,29sep98,tcy  changed message for DCOM 
# 02k,26sep98,tcy  added messages for DLL and DCOM pages
# 02j,24sep98,tcy  removed "following" from 1080_WARN_4 message.
# 02i,17sep98,tcy  added comment on size of SETUP files to 1140_COMP_SELECT.
# 02h,17sep98,wmd  reword message 1080_WARN_4.
# 02g,14sep98,tcy  changed 1210_FINISH and 1550_USAGE messages
# 02f,08sep98,tcy  warn user library update may take several minutes
# 02e,01sep98,wmd  reword message for installing over tree. 
#		   added new messages for license agreement pages.
# 02d,20aug98,wmd  added message for license agreeement.
# 02c,18aug98,tcy  added message for zip-file dialog box
# 02d,04aug98,wmd  added newer/older duplicate file warnings.
# 02c,24jul98,tcy  added system check messages
# 02b,16jul98,wmd  add new messages for T-2.
# 02a,22jul98,tcy  moved license messages to LICW32.TCL;
#                  removed portMapper messages
# 01n,09feb98,pdn  updated string 1080_WARN_4
# 01m,08apr97,pdn  added new string for remote icon installing
#                  fixed spr#8334
# 01l,08mar97,tcy  fixed language in string id 3340 
# 01k,07mar97,tcy  added string id 3340
# 01j,10feb97,pdn  added more license messages.
# 01i,09feb97,pdn  implemented variable argument list for strTableGet(),
#                  clean up.
# 01h,17jan97,jmo  fixed language in strings
# 01g,12dec96,tcy  merged in TEXT-only strings
# 01f,12dec96,pdn  added 1080_WARN_4 string warning that CD-ROM 
#                  revision is older than expected.
# 01e,27nov96,sj   added string for warning against installing in
#                  the root of windows drive.
# 01d,18nov96,tcy  added strings for text-based installation script 
# 01c,14nov96,pdn  substituted function for some global variables
# 01b,14nov96,sj   added strings from Windows installation script
# 01a,11nov96,pdn  written

proc strTableGet {strId args} {
    global strTable
    global setupVals
    global current_file

    if [regexp {^format.*$} $strTable($strId) junk] {
        return [eval $strTable($strId)]
    } {
        return $strTable($strId)
    }
}

set strTable(1000_WELCOME_CD) \
    "format %s \"[cdNameGet description]\""

set strTable(1000_WELCOME1) \
    "format %s \"Welcome to the SETUP program.  This program will\
		 install \[cdromDescGet\] on your computer.\""

set strTable(1010_WELCOME2) \
    "It is strongly recommended that you exit all programs and disable virus\
     protection before running this SETUP program."

set strTable(1020_WELCOME3) \
    "At any time, you can quit the SETUP program by clicking the <Cancel>\
     button. You also can go back to previous dialog boxes by clicking the\
     <Back> button. To accept the current settings for a dialog box and go on\
     with the installation process, click the <Next> button."

set strTable(3020_WELCOME3) \
    "format %s \"At any prompt, you can cancel installation \[cdromDescGet\]\
                 by typing \'exit\'.  You can also go to the previous question\
                 by typing \'-\'.  To accept current settings and go on with\
                 the installation process, press <Return>.\""

set strTable(1030_WELCOME4) \
    "WARNING: This program is protected by copyright law and international\
     treaties."

set strTable(1040_WELCOME5) \
    "Unauthorized reproduction or distribution of this program, or any portion\
     of it, may result in severe civil and criminal penalties, and will be\
     prosecuted to the maximum extent possible under law."

set strTable(1050_ROOT_WARN) \
    "format %s \"Installing \[cdromDescGet\] as \[setupId effective user\] is not\
                 recommended.  We suggest that you logoff and logon as a normal\
		 user before running this program.\
		 \n\nClick Next to continue with SETUP anyway.\""

set strTable(3050_ROOT_WARN) \
    "format %s \"Installing \[cdromDescGet\] as \[setupId effective user\]\
                 is not recommended.  We suggest that you logoff and \
                 logon as a normal user before running this program.\
                 \n\nPress <Return> to continue with SETUP anyway.\""

set strTable(1051_ROOT_WARN) \
    "format %s \"Installing \[cdromDescGet\] without System Administrator\
                 privileges is not recommended.  Under your present privileges,\
		 SETUP will not offer certain installation options, such as \
                 the installation of some services, etc.  Also, the software\
		 will be installed as a personal copy and will not be visible\
		 to other users on this machine.\
		 \n\nTo install \[cdromDescGet\] with access to all its\
		 installation features and options, we suggest that you exit\
		 the installation now and rerun it later with System\
		 Administrator\'s privileges.\n\nClick <Next> to continue with\
		 SETUP anyway.\""

set strTable(1060_REGISTRATION) \
    "Below, type your name, the name of your company."

set strTable(1070_WARN_1) \
    "The installation key you entered is invalid.  Please enter a valid\
     installation key."

set strTable(1071_WARN_1) \
    "Please enter the requested information."

set strTable(1080_WARN_2) \
    "You entered a key that was not created for this CD-ROM.  Please verify\
     that you are using the appropriate key.  If this problem persists, contact\
     Wind River Systems Sales department for help."

set strTable(1080_WARN_3) \
    "The installation key you entered is meant for other vendor's CD-ROM.\
     Please contact the vendor who issued the CD-ROM for a proper key."

set strTable(1085_WARN_4) \
    "This CD-ROM does not require an installation key.  Click the \"Next\"\
     button to continue the installation."

set strTable(1090_WARN_3) \
    "format %s \"Can\'t initiate SETUP: \[lindex \$args 0\].  Please correct\
		 the problem then run SETUP again.\""

set strTable(1095_WARN_NO_TCPIP) \
    "SETUP has detected that your system does not have TCP-IP installed.\
     To correct the problem, please contact your administrator and then\
     run SETUP again.\nAborting setup."

set strTable(1097_WARN_NO_LONGFILENAME_SUP) \
    "SETUP has detected that your system does not have long filename\
     support. To correct the problem, please contact your administrator\
     and then run SETUP again.\nAborting setup."

set strTable(1105_FULL_INSTALL) \
    "Installs the Tornado products, tools, compilers, and other optional\
     components that you may have purchased."

set strTable(1107_PROGRAM_GROUP) \
"Installs only the Tornado program group and tools icons for access to\
 Tornado tools installed on a remote server."

set strTable(1100_DEST_DIR) \
    "format %s \"Please type the name of the directory where you want SETUP to\
		 install \[cdromDescGet\].\
		 \n\nClick the <Browse> button to choose the directory\
		 interactively.\""

set strTable(1100_REMOTE_DIR) \
    "format %s \"Please type the name of the directory where Tornado has\
		 already been installed.\
		 \n\nClick the <Browse> button to choose the directory\
		 interactively.\""

set strTable(3100_DEST_DIR) \
    "format %s \"Please type the name of the directory where you want SETUP\
		 to install \[cdromDescGet\].\""

set strTable(1110_DEST_DIR_WARN) \
    "The installation directory you entered does not exist.\
     \nDo you want to create it now?"

set strTable(3110_DEST_DIR_WARN) \
    "The installation directory you entered does not exist."

set strTable(3115_DEST_DIR_QUESTION) \
    "Do you want to create it now? \[y\]"

set strTable(1111_DEST_DIR_WARN) \
    "format %s \"Installing \[cdromDescGet\] in the root directory is not\
		 recommended.\nClick <Yes> to select another directory.\""

set strTable(1120_DEST_DIR_WARN2) \
    "format %s \"Creating \[destDirGet\] failed: file exists.\""

set strTable(1121_DEST_DIR_WARN2) \
    "format %s \"Installing in \[destDirGet\] is not recommended.\
		 \nDo you want to change the installation directory?\""

set strTable(1122_DEST_DIR_WARN2) \
    "format %s \"Unable to create \[destDirGet\].\""

set strTable(1130_DEST_DIR_WARN3) \
    "You do not have permission to write files into the installation directory\
     you entered.\
     \n\nPlease choose a writable directory."

set strTable(1135_DEST_DIR_WARN4) \
    "format %s \"The installation directory you entered contains white\
                 space(s). Please select another directory.\""

set strTable(1137_DUP_PRODUCT_WARN) \
    "format %s \"Reinstalling products may potentially destroy any\
     modifications you may have made to previously installed files.\
     Do you wish to continue with the installation or go back to the\
     '\[strTableGet 1450_TITLE_OPTION\]' page to reconsider your choices?\""

set strTable(3155_COMP_SELECT_QUESTION) \
    "Do you want to go back and specify a directory on a bigger partition?\
     \[y\]"

set strTable(1140_COMP_SELECT) \
    "format %s \"In the option list below, please check all items you wish\
     to install. SETUP files will be copied to your selected directory and\
     take up \[setupSizeGet\] MB of disk space.\n\""

set strTable(3140_COMP_SELECT) \
    "In the option list below, select the item(s) you want to install."

set strTable(3145_COMP_SELECT_CHANGE) \
    "Press <Return> to accept the setting.  To change the setting, enter a\
     list of item numbers separated by spaces."

set strTable(3145_COMP_SELECT_CHANGE_INVALID) \
    "The item number(s) you entered is not valid."

set strTable(1150_COMP_SELECT_WARN) \
    "There is not enough disk space to install the selected component(s).\
     \n\nDo you want to go back and specify a directory on a bigger disk or\
     partition?"

set strTable(3150_COMP_SELECT_WARN) \
    "There is not enough space to install the selected component(s)."

set strTable(1151_COMP_SELECT_WARN) \
    "At least one component must be selected to continue installation."

set strTable(1160_PERMISSION) \
    "SETUP is about to install the component(s) you have requested.\
     \n\nThe selected button(s) below indicate the file permissions which\
     will be set during the installation process.\
     \n\nPlease adjust these to suit your site requirements."

set strTable(3160_PERMISSION) \
    "SETUP is about to install the component(s) you have requested."

set strTable(3162_PERMISSION) \
    "The list below indicates the file permissions which will be set during\
     the installation process. Please adjust these to suit your site\
     requirements."

set strTable(3165_PERMISSION_QUESTION) \
    "Press <Return> to accept the setting.  To change the setting, enter a\
     list of item numbers separated by spaces."

set strTable(1161_FOLDER_SELECT) \
    "SETUP will add program icons to the Program Folder listed below. You may\
     type a new folder name, or select one from the existing Folders list." 

set strTable(1162_FOLDER_SELECT) \
    "Please enter a valid folder name."

set strTable(1170_FILE_COPY) \
    "format %s \"SETUP is copying the selected component(s) to the directory\
                \[destDirGet\].\""

set strTable(1171_FILE_COPY) \
    "format %s \"SETUP cannot read \[setupFileNameGet 0\] from the CD-ROM.\
                 Please ensure that the CD-ROM is properly mounted.\""

set strTable(1180_LIB_UPDATE) \
    "SETUP is updating the VxWorks libraries.  We recommend that you let\
     SETUP finish this step, or the libraries will be in an inconsistent\
     state.  Please be patient as the process may take several minutes. \
     If you want to quit the SETUP program, click <Cancel> and run\
     the SETUP program again at a later time."

set strTable(3180_LIB_UPDATE) \
    "SETUP is updating the VxWorks libraries."

set strTable(1190_REGISTRY_HOST) \
    "The Tornado Registry is a daemon that keeps track of all available\
     targets by name.  Only one registry is required on your network, \
     and it can run on any networked host.\
     \n\nPlease enter the name of the host where the Tornado Registry will\
     be running."

set strTable(1191_REGISTRY_DESC) \
    "The Tornado Registry is a daemon that keeps track of all available\
     targets by name.  Only one registry is required on your network, \
     and it can run on any networked host."

set strTable(1192_REGISTRY_NAME) \
    "Please enter the name of the host where the Tornado Registry will\
     be running."

set strTable(1200_FINISH_WARN) \
    "format %s \"However, there were \[errorCountGet\] error(s) which occured\
	         during the process.  Please review the log file\
		 \[destDirDispGet\]/setup.log for more information.\""

set strTable(1210_FINISH) \
    "format %s \"SETUP has completed installing the selected product(s).\""

set strTable(1212_FINISH) \
    "SETUP has completed installing the program folders and icons."
                    
set strTable(1213_FINISH) \
    "Terminating SETUP program."

set strTable(1360_QUIT_CALLBACK) \
    "format %s \"SETUP is not complete. If you quit the SETUP program now,\
                 \[cdromDescGet\] will not be installed.\n\nYou may run\
                 the SETUP program at a later time to complete the\
                 installation.\
		 \n\nTo continue installing the program, click <Resume>.  \
		 To quit the SETUP program, click <Exit SETUP>.\""

set strTable(3360_QUIT_CALLBACK) \
    "format %s \"SETUP is not complete. If you quit the SETUP program now,\
                 \[cdromDescGet\] will not be installed.\n\nYou may run the\
		 SETUP program at a later time to complete the installation.\
		 \n\nTo continue installing the program,  Press <Return>.  \
		 To quit the SETUP program, type \'exit\'.\""

set strTable(1370_FILE_ACCESS_ERROR) \
    "format %s \"SETUP cannot create/update file \[lindex \$args 0\]:\
		 \[lindex \$args 1\]\""

set strTable(1380_DEFLATE_ERROR) \
    "format %s \"SETUP isn\'t able to deflate \[setupFileNameGet 0\]\
                 \n\nPlease select one of the following options\
                 to continue with the SETUP process.\""

set strTable(1390_MEMORY_LOW) \
    "The system is running out of memory.  To continue, close applications\
     or increase the system swap space."

set strTable(1400_DISK_FULL) \
    "No disk space left.  To continue, free up some disk space."

set strTable(1550_USAGE) \
    "Usage: SETUP /I\[con\]\]\t\n\
            /I : Add standard Tornado icons \n\
                 from a remote installation"

set strTable(1410_TITLE_WELCOME) "Welcome"
set strTable(1420_TITLE_WARNING) "Warning"
set strTable(1430_TITLE_REGISTRATION) "User Registration"
set strTable(1440_TITLE_DESTDIR) "Select Directory"
set strTable(1450_TITLE_OPTION) "Select Products"
set strTable(1460_TITLE_PERMISSION) "Permission"
set strTable(1470_TITLE_FILECOPY) "Copying Files"
set strTable(1480_TITLE_LIBUPDATE) "Update Libraries"
set strTable(1490_TITLE_REGISTRY_HOST) "Tornado Registry"
set strTable(1495_TITLE_BACKWARD_COMPATIBILITY) "Backward Compatibility"
set strTable(1500_TITLE_FINISH) "Finish"
set strTable(1560_TITLE_FOLDER) "Select Folder"
set strTable(1563_TITLE_DLL_REG) "Software Registration"
set strTable(1567_TITLE_DCOM) "DCOM Installation"

set strTable(1570_OPTION_SELECT) \
    "Choose one of the options listed below, then click the\
     <Next> button to continue the installation."

set strTable(1576_OPTION_MANUAL) \
    "Install Tornado Registry manually"

set strTable(1577_OPTION_STARTUP) \
    "Install Tornado Registry locally in the Startup Group"

set strTable(1578_OPTION_SERVICE) \
    "Install Tornado Registry locally as a Service"

set strTable(1579_OPTION_REMOTE) \
    "Configure to use a remote Tornado Registry"

set strTable(1580_OPTION_DESC) \
    "If you plan on running Tornado in a non-networked environment, we\
     recommend that you install the registry in your Startup Group or as an\
     NT Service.  For more information, consult your Tornado User\'s Guide."

set strTable(1581_OPTION_DESC) \
    "If you plan on running Tornado in a non-networked environment, we\
     recommend that you install the registry in your Startup Group.  For more\
     information, consult your Tornado User\'s Guide."

set strTable(3000_RETURN_QUESTION) \
    "Press <Return> to continue"

set strTable(3055_EXIT_QUESTION) \
    "Type \'exit\' to quit the program or press <Return> to continue"

set strTable(3370_BACK_CALLBACK) \
    "Cannot go back further."

set strTable(1080_WARN_4) \
    "The installation key you entered attempted to unlock one or more \
     products that may have been removed from our product line. \
     Please compare the unlocked product list on the\
     \"[strTableGet 1450_TITLE_OPTION]\" screen with your purchased order\
     list, and contact us if you discover any differences."

set strTable(4000_BASE_INSTALL_WARN) \
      "format %s \"Warning! Re-installing Tornado over an existing \
		tree will overwrite any installed patches. \
		If you proceed with the installation, please \
		re-install patches if any.\""

set strTable(4000_BASE_INSTALL_WARN_1) \
    "Select <Install> to overwrite existing Tornado installation,\
     or choose <Select Path> to enable you to back up to the \'Select\
     Directory\' page to enter an alternate path."

set strTable(4010_FILE_EXISTS_OLDER_WARN) \
     "format %s \"The file \'\$current_file\' exists in your destination\
                directory path \'\[destDirGet\]\' and is older.  You can\
                set the policy for handling duplicate files by\
                selecting one of the following buttons.  All files to be\
                overwritten will be backed up.\""

set strTable(4010_FILE_EXISTS_NEWER_WARN) \
     "format %s \"The file \'\$current_file\' exists in your destination\
                directory path \'\[destDirGet\]\' and is newer.  You can\
                set the policy for handling duplicate files by\
                selecting one of the following buttons.  All files to be\
                overwritten will be backed up.\""

set strTable(4010_FILE_EXISTS_WARN_1) \
    "Overwrite the existing file."

set strTable(4010_FILE_EXISTS_WARN_2) \
    "Do not overwrite the existing file."

set strTable(4010_FILE_EXISTS_WARN_3) \
    "Overwrite ALL files, do not show this dialog again."

set strTable(4020_ANALYZING_BANNER) \
    "Analyzing installation files, please wait..."

set strTable(4030_NO_ZIP_FILE) \
    "format %s \"SETUP cannot find the ZIP files for installing\
               \[cdromDescGet\] in the default directory.\n\n\
               Please type the name of the WIND\
               directory containing the ZIP files.\n\nClick the\
               <Browse> button to choose the directory interactively.\""

set strTable(4040_LIC_TEXT) \
   "Attention: By clicking on the \"I accept\" button or by\
   Installing the software you are consenting to be bound by\
   the terms of this agreement (this \"Agreement\").  If you do\
   not agree to all of the terms, click the \"I don't Accept\" button\
   and do not install this software.  A copy of this Agreement can be viewed\
   in the Setup directory under the destination path that you have\
   designated after the installation is completed." 

set strTable(4050_PROJECT_TEXT) \
    "Please enter your project name, and the number of licensed\
    users on the project in the spaces below."

set strTable(4060_LICENSE_TEXT) \
    "By clicking on the \"I accept\" button \
    you are consenting to be bound by the terms of this agreement.\
    If you do not agree to all of the terms, click the \"Cancel\"\
    button and do not install this software."

set strTable(4070_DLL_TEXT) \
    "SETUP is registering software on your machine. This will take a few\
    minutes."

set strTable(4080_DCOM_TEXT) \
    "Setup has detected that your COM/DCOM DLLs must\
     be updated for the correct operation of Tornado 2.0.\
     \n\n\
     Setup will now ask you to run DCOM95 to update your\
     DLLs.\
     \n\n\
     You will have to reboot your system after DLL files have been\
     installed. Please rerun SETUP to continue with installation\
     after your system has rebooted.\
     \n\n\
     Note: The DCOM95 installation programs update your\
     system DLLs. You should save all open documents and close all\
     programs before proceeding.\
     \n\nWould you like to install \"DCOM95\" now?"

set strTable(4082_DCOM95_AND_COMCTL_TEXT) \
    "Setup has detected that your COM/DCOM and Common Control DLLs must\
     be updated for the correct operation of Tornado 2.0.\
     \n\n\
     Setup will now ask you to run DCOM95 and 401comupd.exe to update your\
     DLLs.\
     \n\n\
     You must reboot your system after DLL files have been\
     installed. After rebooting, please rerun SETUP to continue with\
     installation.\
     \n\n\
     Note: 401comupd.exe and DCOM95 installation programs update your\
     system DLLs. You should save all open documents and close all\
     programs before proceeding\
     \n\nWould you like to install \"401comupd.exe\" and \"DCOM95\" now?"

set strTable(4085_COMCTL_UPDATE_TEXT) \
    "Setup has detected that your Common Control DLLs must\
     be updated for the correct operation of Tornado 2.0.\
     \n\n\
     Setup will now ask you to run DCOM95 and 401comupd.exe to update your\
     DLLs.\
     \n\n\
     You will have to reboot your system after DLL files have been\
     installed. Please rerun SETUP to continue with installation\
     after your system has rebooted.\
     \n\n\
     Note: The 401comupd.exe installation program updates your system DLLs. You\
     should save all open documents and close all programs before installing\
     401comupd.exe.\
     \n\nWould you like to install \"401comupd.exe\" now?"

set strTable(4090_README_TEXT) \
    "Please read the README file contents that are displayed below.\
     It contains important information that will enable you to install\
     and successfully run the BerkeleyDB product.  For your convenience\
     this file is copied to your installation directory path."

set strTable(5000_PATCHES_REQUIRED_TEXT) \
    "SETUP has detected that required operating system patches\
     have not been installed on this machine.  These patches are\
     necessary for the correct operation of SETUP and Tornado.  Please refer\
     to the Tornado Release Notes for details.\n\n\
     The following operating system patches must be installed before\
     you can continue with installation:\n\n"

set strTable(5001_PATCHES_RECOMMENDED_TEXT) \
    "\n\nSETUP has also detected that recommended operating system patches\
     have not been installed.  It is recommended that these patches are\
     installed before starting Tornado to ensure correct operation.\n\n\
     The following operating system patches are recommended to be installed:\n\n"

set strTable(5002_PATCHES_RECOMMENDED_TEXT) \
    "SETUP has detected that some operating system patches have not been\
     installed on this machine.  It is recommended that these\
     patches are installed before starting Tornado to ensure correct\
     operation.  Please refer to the Tornado Release Notes\
     for details.\n\n\
     The following operating system patches are recommended to be installed:\n\n"

set strTable(5003_PATCHES_REQUIRED_FORMATTED_TEXT) \
    "\n SETUP has detected that required operating system patches\n\
     have not been installed on this machine.  These patches are\n\
     necessary for the correct operation of SETUP and Tornado.  Please refer\n\
     to the Tornado Release Notes for details.\n\n\
     The following operating system patches must be installed before\n\
     you can continue with installation:\n\n"

set strTable(5004_PATCHES_RECOMMENDED_FORMATTED_TEXT) \
    "\n\n SETUP has also detected that recommended operating system patches\n\
     have not been installed.  It is recommended that these patches are\n\
     installed before starting Tornado to ensure correct operation.\n\n\
     The following operating system patches are recommended to be installed:\n\n"

set strTable(5005_PATCHES_RECOMMENDED_FORMATTED_TEXT) \
    "\n SETUP has detected that some operating system patches have not been\n\
     installed on this machine.  It is recommended that these\n\
     patches are installed before starting Tornado to ensure correct\n\
     operation.  Please refer to the Tornado Release Notes\n\
     for details.\n\n\
     The following operating system patches are recommended to be installed:\n\n"

set strTable(5006_PATCHES_SUN_LOCATION) \
    "\nPatches for Sun machines are available at http://sunsolve.sun.com.\n"

set strTable(5007_PATCHES_HP_LOCATION) \
    "\nPatches for HP machines are available at:\n\
     http://us-support.external.hp.com (US, Canada, Asia-Pacific, and\
     Latin-America)\n\
     http://europe-support.external.hp.com (Europe)\n"

set strTable(5008_PATCHES_UPDATE) \
    "\nNote: System vendors very frequently update and replace patches.\
    If a specific patch is no longer available, please use the\
    replacement patch suggested by the system vendor.\n"

set strTable(5009_PATCHES_UPDATE_FORMATTED) \
    "\n Note: System vendors very frequently update and replace patches.\n\
    If a specific patch is no longer available, please use the\n\
    replacement patch suggested by the system vendor.\n"

set strTable(5010_DRIVERS_INFO) \
    "The installation of the Driver component is required because\n\
     you have selected the basic Tornado product for installation.\n\n\
     If you wish to uncheck this item you must uncheck either the\n\
     basic Tornado and/or Tornado Simulator product(s) or go to the\n\
     'Details' button for Tornado and uncheck both the Simulator and\n\
     the Tornado Object parts."

set strTable(5020_DO_NOT_SAVE_KEY_FOR_FAE) \
    "The installation key you are about to enter will NOT\
     be saved in the system registry.\nIs this what you want?"

set strTable(5030_BACKWARD_COMPATIBILITY) \
    "While the portmapper is not needed for Tornado 2.0, it is\
     included in this release for development environments in\
     which both Tornado 2.0 and Tornado 1.0.1 are in use.\
     \n\nWould you like to use your Tornado 1.0.x tools with Tornado 2.0?"

set strTable(5040_BACKWARD_COMPATIBILITY) \
    "Note:\
     \n\nIf you have selected to install the Tornado Registry as\
     a service, there is no way to retain backward compatibility\
     with Tornado 1.0.x."

set strTable(5050_BACKWARD_COMPATIBILITY) \
    "For more information on backward compatibility,\
     please consult the Tornado 2.0 Release Notes."
