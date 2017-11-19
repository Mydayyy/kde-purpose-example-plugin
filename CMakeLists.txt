cmake_minimum_required(VERSION 2.8)

set(KF5_VERSION "5.8.0") # handled by release scripts
set(KF5_DEP_VERSION "5.7.0") # handled by release scripts
set(REQUIRED_QT_VERSION "5.2")
set(KDE_EXPERIMENTAL_VERSION ${KF5_DEP_VERSION})

project(MydayyyImageUpload)

find_package(ECM 1.7.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Core Qml Gui Widgets Network Test)
find_package(KF5 ${KF5_DEP_VERSION} REQUIRED COMPONENTS CoreAddons I18n Config KIO)
find_package(KDEExperimentalPurpose)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)



kcoreaddons_add_plugin("MydayyyImageUpload" SOURCES "mydayyyimageupload.cpp" JSON "mydayyyimageupload.json" INSTALL_NAMESPACE "purpose")

target_link_libraries(MydayyyImageUpload Qt5::Network Qt5::Widgets Qt5::Core KF5::CoreAddons KDEExperimental::Purpose KF5::KIOCore KF5::I18n)
