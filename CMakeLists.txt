cmake_minimum_required(VERSION 3.14)
include(FetchContent)

project(RootPlotter VERSION 1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


add_compile_options(-Wno-unused-variable)
add_compile_options(-Wall -Wextra -Wpedantic
  #-Wdouble-promotion
  #-Wimplicit-int-conversion
  #-Wimplicit-float-conversion
  #-Wfloat-equal
  #-Wcovered-switch-default
)
add_link_options( -L /usr/local/lib )
add_compile_options( $<$<CONFIG:RELEASE>:-O3>
  $<$<CONFIG:DEBUG>:-O0>
  $<$<CONFIG:DEBUG>:-g>) 


FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG b6f4ceaed0a0a24ccf575fab6c56dd50ccf6f1a9)

FetchContent_Declare(sol2
  GIT_REPOSITORY https://github.com/ThePhD/sol2/
  GIT_TAG eba86625b707e3c8c99bbfc4624e51f42dc9e561)
set(SOL2_ENABLE_INSTALL OFF)

FetchContent_Declare(cli11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11
  GIT_TAG        v2.2.0)
FetchContent_MakeAvailable(fmt sol2 cli11)
find_package(ROOT 2.26 REQUIRED COMPONENTS RIO Core Hist Tree Thread)
if(NOT ${ROOT_FOUND})
  message("Could not find system root, checking local install")
  include(/export/scratch/local/share/root/cmake/ROOTConfig.cmake)
endif()
find_package(Lua 5.3)
if(NOT ${LUA_FOUND})
  FetchContent_Declare( lua URL http://www.lua.org/ftp/lua-5.4.4.tar.gz )
  FetchContent_GetProperties(lua)
  if(NOT lua_POPULATED)
    FetchContent_Populate(lua)
  endif()
  set(srcDir ${lua_SOURCE_DIR}/src)
  set(srcFiles lapi.c lauxlib.c lbaselib.c lcode.c lcorolib.c lctype.c ldblib.c ldebug.c ldo.c
    ldump.c lfunc.c lgc.c linit.c liolib.c llex.c lmathlib.c lmem.c loadlib.c lobject.c lopcodes.c
    loslib.c lparser.c lstate.c lstring.c lstrlib.c ltable.c ltablib.c  ltm.c lundump.c
    lutf8lib.c lvm.c lzio.c)
  set(publicHeaderFiles lauxlib.h lua.h luaconf.h lualib.h)

  list(TRANSFORM srcFiles PREPEND ${srcDir}/)
  list(TRANSFORM publicHeaderFiles PREPEND ${srcDir}/)

  add_library(lua ${srcFiles})
  target_compile_definitions(lua PRIVATE $<$<PLATFORM_ID:Linux>:LUA_USE_LINUX LUA_COMPAT_5_2>)
  target_compile_options(lua
    PRIVATE
    $<$<OR:$<C_COMPILER_ID:AppleClang>,$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:GNU>>:
    -Wextra -Wshadow -Wsign-compare -Wundef -Wwrite-strings -Wredundant-decls
    -Wdisabled-optimization -Waggregate-return -Wdouble-promotion -Wdeclaration-after-statement
    -Wmissing-prototypes -Wnested-externs -Wstrict-prototypes -Wc++-compat -Wold-style-definition>)
  set(includeDir ${lua_BINARY_DIR}/include)
  file(COPY ${publicHeaderFiles} DESTINATION ${includeDir})
  target_link_libraries(lua PUBLIC ${CMAKE_DL_LIBS})
  target_include_directories(lua SYSTEM INTERFACE ${includeDir})

else()
  add_library(lua INTERFACE)
  target_link_libraries(lua INTERFACE ${LUA_LIBRARIES})
  target_include_directories(lua SYSTEM  INTERFACE ${LUA_INCLUDE_DIR})
endif()

#FetchContent_Declare (
#tracy
#GIT_REPOSITORY https://github.com/wolfpld/tracy.git
#GIT_TAG master
#GIT_SHALLOW TRUE
#GIT_PROGRESS TRUE
#)
#FetchContent_MakeAvailable ( tracy )


#set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)

include(GNUInstallDirs)
set(PLOTTER_INSTALL_DATADIR ${CMAKE_INSTALL_FULL_DATADIR}/rootplotter/scripts)


add_compile_definitions(SOL_ALL_SAFETIES_ON)
add_compile_definitions(SOL_PRINT_ERRORS)


configure_file(configure/install_info.h.in ${PROJECT_BINARY_DIR}/config/install_info.h)
add_library(install_config INTERFACE)
target_include_directories(install_config INTERFACE ${PROJECT_BINARY_DIR}/config)

add_executable(plotter
  src/data.cpp
  src/verbosity.cpp
  src/main.cpp
  src/bindings.cpp
  src/plotting.cpp
  src/annotations.cpp
  src/style.cpp
)
target_include_directories(plotter PUBLIC ${CMAKE_BINARY_DIR}/generated)

target_link_libraries(plotter PUBLIC
  fmt::fmt
  ROOT::Core ROOT::Hist ROOT::RIO ROOT::Graf ROOT::Gpad sol2
  lua
  CLI11::CLI11
  install_config
  )



message("Installing scripts to ${CMAKE_INSTALL_FULL_DATADIR}")
install(TARGETS plotter DESTINATION ${GNU_INSTALL_BINDIR})
install(DIRECTORY scripts
  DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/rootplotter)

