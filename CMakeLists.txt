cmake_minimum_required( VERSION 3.20 )

project(QtSnippingTool VERSION 1.0 LANGUAGES CXX)

include(FetchContent)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

IF(DEFINED ENV{QT_PATH})
    SET(QT_SDK_DIR $ENV{QT_PATH})
ELSE()
    SET(QT_SDK_DIR D:/Qt/Qt-6.9.0-VC2022-X64-D-MD-OCI-2025-04-21 CACHE PATH "QT SDK DIR" FORCE)
ENDIF()

message(STATUS ${QT_SDK_DIR})

SET(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/Install CACHE PATH "Installation path" FORCE)

list(APPEND CMAKE_PREFIX_PATH ${QT_SDK_DIR})

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets LinguistTools)

option(ELAPACKETIO_BUILD_STATIC_LIB "Build static library." ON)
option(ELAWIDGETTOOLS_BUILD_STATIC_LIB "Build static library." ON)

add_definitions(-DELAPACKETIO_STATIC_LIBRARY)
add_definitions(-DELAWIDGETTOOLS_STATIC_LIBRARY)

FetchContent_Declare(
        ElaWidgetTools
        GIT_REPOSITORY https://github.com/jgh0721/ElaWidgetTools.git
        GIT_PROGRESS   TRUE
        GIT_TAG        2d66ddc2
)

FetchContent_MakeAvailable(ElaWidgetTools)

FILE(GLOB ORIGIN src/*.cpp src/*.hpp)
set( PROJECT_SOURCES ${ORIGIN}
     src/snippingTool.cpp
     src/snippingTool.hpp
     src/dxgiMgr.hpp
     src/dxgiMgr.cpp )

qt_add_executable( ${PROJECT_NAME} MANUAL_FINALIZATION ${PROJECT_SOURCES} )

target_link_libraries( ${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets )
target_link_libraries(${PROJECT_NAME} PRIVATE ElaWidgetTools)

set_target_properties(${PROJECT_NAME} PROPERTIES
                      ${BUNDLE_ID_OPTION}
                      MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
                      MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
                      MACOSX_BUNDLE TRUE
                      WIN32_EXECUTABLE TRUE
                      )

if (QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif ()

if (WIN32)
    include(GNUInstallDirs)
endif()
#
#if (${QT_VERSION_MAJOR} GREATER_EQUAL 6)
#    qt_add_executable(${PROJECT_NAME}
#                      MANUAL_FINALIZATION
#                      ${PROJECT_SOURCES}
#                      )
#    #遍历所有资源文件
#    file(GLOB_RECURSE RES_PATHS *.png *.jpg *.svg *.ico *.ttf *.webp *.js)
#    foreach (filepath ${RES_PATHS})
#        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" filename ${filepath})
#        list(APPEND resource_files ${filename})
#    endforeach (filepath)
#
#    qt_add_resources(${PROJECT_NAME} "ElaWidgetToolsExample"
#                     RESOURCES PREFIX "/"
#                     FILES
#                     ${resource_files}
#                     )
#else ()
#    qt5_add_big_resources(PROJECT_SOURCES
#                          ElaWidgetToolsExample.qrc
#                          )
#    add_executable(${PROJECT_NAME}
#                   ${PROJECT_SOURCES}
#                   )
#endif ()
#
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${PROJECT_NAME})
#
#IF(ELAPACKETIO_BUILD_STATIC_LIB)
#    add_definitions(-DELAPACKETIO_STATIC_LIBRARY)
#ENDIF()
#if (ELAWIDGETTOOLS_BUILD_STATIC_LIB)
#    add_definitions(-DELAWIDGETTOOLS_STATIC_LIBRARY)
#ENDIF()
#
#if (WIN32)
#    target_include_directories(${PROJECT_NAME} PUBLIC
#                               ExamplePage
#                               ModelView
#                               )
#    target_link_libraries(${PROJECT_NAME} PRIVATE
#                          Qt${QT_VERSION_MAJOR}::Widgets
#                          ElaWidgetTools
#                          ElaPacketIO
#                          )
#else ()
#    target_include_directories(${PROJECT_NAME} PUBLIC
#                               ExamplePage
#                               ModelView
#                               )
#    target_link_libraries(${PROJECT_NAME} PRIVATE
#                          Qt${QT_VERSION_MAJOR}::Widgets
#                          ElaWidgetTools
#                          )
#endif ()
#
#if (${QT_VERSION} VERSION_LESS 6.1.0)
#    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.${PROJECT_NAME})
#endif ()
#
#if (WIN32)
#    include(GNUInstallDirs)
#    install(TARGETS ${PROJECT_NAME}
#            LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}
#            RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}
#            )
#    if (MSVC)
#        install(
#                FILES $<TARGET_PDB_FILE:${PROJECT_NAME}>
#                DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME} OPTIONAL)
#    endif ()
#    # QT依赖安装 只做Release安装 Debug自行打包
#    install(FILES ${QT_SDK_DIR}/plugins/platforms/qwindows.dll DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/platforms)
#    install(FILES ${QT_SDK_DIR}/plugins/generic/qtuiotouchplugin.dll DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/generic)
#    install(
#            FILES ${QT_SDK_DIR}/plugins/imageformats/qgif.dll
#            FILES ${QT_SDK_DIR}/plugins/imageformats/qicns.dll
#            FILES ${QT_SDK_DIR}/plugins/imageformats/qico.dll
#            FILES ${QT_SDK_DIR}/plugins/imageformats/qjpeg.dll
#            FILES ${QT_SDK_DIR}/plugins/imageformats/qtga.dll
#            FILES ${QT_SDK_DIR}/plugins/imageformats/qtiff.dll
#            FILES ${QT_SDK_DIR}/plugins/imageformats/qwbmp.dll
#            FILES ${QT_SDK_DIR}/plugins/imageformats/qwebp.dll
#            DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/imageformats
#            )
#    FILE(GLOB STYLES ${QT_SDK_DIR}/plugins/styles/*.dll)
#    install(FILES ${STYLES} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/styles)
#    FILE(GLOB TRANSLATIONS ${QT_SDK_DIR}/translations_install/*.qm)
#    install(FILES ${TRANSLATIONS} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/translations)
#    install(
#            FILES ${QT_SDK_DIR}/bin/Qt${QT_VERSION_MAJOR}Widgets.dll
#            FILES ${QT_SDK_DIR}/bin/Qt${QT_VERSION_MAJOR}Core.dll
#            FILES ${QT_SDK_DIR}/bin/Qt${QT_VERSION_MAJOR}Gui.dll
#            FILES ${QT_SDK_DIR}/bin/Qt${QT_VERSION_MAJOR}Network.dll
#            FILES ${QT_SDK_DIR}/bin/Qt${QT_VERSION_MAJOR}Xml.dll
#            DESTINATION ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}
#            )
#endif ()
#
