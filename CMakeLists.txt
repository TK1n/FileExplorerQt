cmake_minimum_required(VERSION 3.16)
project(FileExplorerApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 🔑 This fixes the vtable/staticMetaObject error
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(SOURCES
    main.cpp
    mainwindow.cpp
)

set(HEADERS
    mainwindow.h
)

qt_add_executable(FileExplorer
    ${SOURCES}
    ${HEADERS}

)

target_link_libraries(FileExplorer
    PRIVATE
        Qt6::Widgets
)
