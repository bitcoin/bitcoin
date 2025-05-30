qt_details_version := 6.7.3
qt_details_download_path := https://download.qt.io/archive/qt/6.7/$(qt_details_version)/submodules
qt_details_suffix := everywhere-src-$(qt_details_version).tar.xz

qt_details_qtbase_file_name := qtbase-$(qt_details_suffix)
qt_details_qtbase_sha256_hash := 8ccbb9ab055205ac76632c9eeddd1ed6fc66936fc56afc2ed0fd5d9e23da3097

qt_details_qttranslations_file_name := qttranslations-$(qt_details_suffix)
qt_details_qttranslations_sha256_hash := dcc762acac043b9bb5e4d369b6d6f53e0ecfcf76a408fe0db5f7ef071c9d6dc8

qt_details_qttools_file_name := qttools-$(qt_details_suffix)
qt_details_qttools_sha256_hash := f03bb7df619cd9ac9dba110e30b7bcab5dd88eb8bdc9cc752563b4367233203f

qt_details_patches_path := $(PATCHES_PATH)/qt

qt_details_top_download_path := https://code.qt.io/cgit/qt/qt5.git/plain
qt_details_top_cmakelists_file_name := CMakeLists.txt
qt_details_top_cmakelists_download_file := $(qt_details_top_cmakelists_file_name)?h=$(qt_details_version)
qt_details_top_cmakelists_sha256_hash := 9fb720a633c0c0a21c31fe62a34bf617726fed72480d4064f29ca5d6973d513f
qt_details_top_cmake_download_path := $(qt_details_top_download_path)/cmake
qt_details_top_cmake_ecmoptionaladdsubdirectory_file_name := ECMOptionalAddSubdirectory.cmake
qt_details_top_cmake_ecmoptionaladdsubdirectory_download_file := $(qt_details_top_cmake_ecmoptionaladdsubdirectory_file_name)?h=$(qt_details_version)
qt_details_top_cmake_ecmoptionaladdsubdirectory_sha256_hash := 97ee8bbfcb0a4bdcc6c1af77e467a1da0c5b386c42be2aa97d840247af5f6f70
qt_details_top_cmake_qttoplevelhelpers_file_name := QtTopLevelHelpers.cmake
qt_details_top_cmake_qttoplevelhelpers_download_file := $(qt_details_top_cmake_qttoplevelhelpers_file_name)?h=$(qt_details_version)
qt_details_top_cmake_qttoplevelhelpers_sha256_hash := 5ac2a7159ee27b5b86d26ecff44922e7b8f319aa847b7b5766dc17932fd4a294
