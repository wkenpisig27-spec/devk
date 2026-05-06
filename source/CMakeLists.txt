# ============================================================================
# PKO Server - Linux Build System (CMake)
# ============================================================================
# This file is the LINUX build system. For Windows, use source.sln with
# Visual Studio 2022.
#
# Quick start:
#   ./scripts/build-linux.sh           (recommended)
#
# Manual:
#   mkdir -p out/linux && cd out/linux
#   cmake ../.. -DCMAKE_BUILD_TYPE=Release
#   make -j$(nproc)
#
# Output: out/linux/bin/{AccountServer,GateServer,GroupServer,GameServer}
#
# Prerequisites (Ubuntu/Debian):
#   apt install build-essential cmake libunixodbc-dev libssl-dev
#   apt install libicu-dev libluajit-5.1-dev libbotan-2-dev
#
# Or build third-party libraries from source in ../third_party/
# ============================================================================

cmake_minimum_required(VERSION 3.16)
project(PKOServer VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ============================================================================
# Platform detection and flags
# ============================================================================
if(NOT UNIX)
    message(FATAL_ERROR "This CMake build is for Linux only. Use Visual Studio .sln on Windows.")
endif()

add_compile_definitions(
    NDEBUG
    _LIB
    _SERVER
    NOMINMAX
    BOTAN_DLL=
    BOTAN_FUTURE_INTERNAL_HEADER_OK
)

add_compile_options(
    -fexceptions
    -ffunction-sections
    -fdata-sections
    -fPIC
    -Wno-multichar
    -Wno-write-strings
    -Wno-format-security
)

# Linker flags for dead-code elimination
add_link_options(-Wl,--gc-sections)

# ============================================================================
# Find system packages
# ============================================================================
find_package(Threads REQUIRED)
find_package(PkgConfig QUIET)

# ODBC (unixODBC)
find_path(ODBC_INCLUDE_DIR sql.h PATHS /usr/include /usr/local/include)
find_library(ODBC_LIBRARY NAMES odbc PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
if(NOT ODBC_INCLUDE_DIR OR NOT ODBC_LIBRARY)
    message(FATAL_ERROR "unixODBC not found. Install: apt install unixodbc-dev")
endif()

# OpenSSL (for LicenseValidator)
find_package(OpenSSL REQUIRED)

# ICU
if(PKG_CONFIG_FOUND)
    pkg_check_modules(ICU QUIET icu-uc icu-i18n icu-io)
endif()
if(NOT ICU_FOUND)
    find_path(ICU_INCLUDE_DIR unicode/unistr.h
        PATHS /usr/include /usr/local/include ${CMAKE_SOURCE_DIR}/include/unicode)
    find_library(ICU_UC_LIB NAMES icuuc)
    find_library(ICU_IN_LIB NAMES icui18n)
    find_library(ICU_DT_LIB NAMES icudata)
    if(ICU_UC_LIB AND ICU_IN_LIB AND ICU_DT_LIB)
        set(ICU_LIBRARIES ${ICU_UC_LIB} ${ICU_IN_LIB} ${ICU_DT_LIB})
        set(ICU_INCLUDE_DIRS ${ICU_INCLUDE_DIR})
    else()
        message(FATAL_ERROR "ICU not found. Install: apt install libicu-dev")
    endif()
endif()

# Botan
find_library(BOTAN_LIBRARY NAMES botan-2 botan
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
          ${CMAKE_SOURCE_DIR}/lib/x64/Release)
find_path(BOTAN_INCLUDE_DIR botan/botan.h
    PATHS /usr/include/botan-2 /usr/local/include/botan-2
          ${CMAKE_SOURCE_DIR}/include/botan)
if(NOT BOTAN_LIBRARY)
    message(WARNING "Botan library not found. Trying to continue without it.")
    set(BOTAN_LIBRARY "")
endif()

# LuaJIT
find_library(LUAJIT_LIBRARY NAMES luajit-5.1 luajit lua51
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
          ${CMAKE_SOURCE_DIR}/lib/x64/Release)
find_path(LUAJIT_INCLUDE_DIR luajit.h
    PATHS /usr/include/luajit-2.1 /usr/include/luajit-2.0 /usr/local/include/luajit-2.1
          ${CMAKE_SOURCE_DIR}/include/luajit)
if(NOT LUAJIT_LIBRARY)
    message(FATAL_ERROR "LuaJIT not found. Install: apt install libluajit-5.1-dev")
endif()

# ============================================================================
# Common include directories (available to all targets)
# ============================================================================
set(PKO_INCLUDE_DIRS
    ${ICU_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include/linux_compat
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/serversdk
    ${CMAKE_SOURCE_DIR}/include/common
    ${CMAKE_SOURCE_DIR}/include/util
    ${CMAKE_SOURCE_DIR}/include/enclib
    ${CMAKE_SOURCE_DIR}/include/icuhelper
    ${CMAKE_SOURCE_DIR}/include/infonet
    ${CMAKE_SOURCE_DIR}/include/status
    ${CMAKE_SOURCE_DIR}/include/ipss
    ${CMAKE_SOURCE_DIR}/include/botan
    ${CMAKE_SOURCE_DIR}/include/luajit
    ${CMAKE_SOURCE_DIR}/src
    ${ODBC_INCLUDE_DIR}
    ${BOTAN_INCLUDE_DIR}
)

# ============================================================================
# Library: EncLib (encryption)
# ============================================================================
add_library(EncLib STATIC
    src/enclib/d3des.cpp
    src/enclib/enclib.cpp
)
target_include_directories(EncLib PRIVATE
    ${CMAKE_SOURCE_DIR}/include/enclib
    ${CMAKE_SOURCE_DIR}/src/enclib
    ${CMAKE_SOURCE_DIR}/include
)

# ============================================================================
# Library: ICUHelper
# ============================================================================
add_library(ICUHelper STATIC
    src/icuhelper/pi_Alloc.cpp
    src/icuhelper/pi_Memory.cpp
    src/icuhelper/ResourceBundleManage.cpp
)
target_include_directories(ICUHelper PRIVATE
    ${CMAKE_SOURCE_DIR}/include/icuhelper
    ${CMAKE_SOURCE_DIR}/src/icuhelper
    ${CMAKE_SOURCE_DIR}/include/linux_compat
    ${CMAKE_SOURCE_DIR}/include/serversdk
)
# On Linux: use -iquote for source/include so it only applies to #include "..."
# but NOT to #include <unicode/...>, letting system ICU headers be found first.
# IMPORTANT: -iquote for system ICU path MUST come first because system ICU headers
# internally use quoted includes (#include "unicode/utypes.h") which would otherwise
# resolve to the bundled ICU 76 headers via -iquote source/include, creating a
# version mismatch with the system ICU 74 libraries.
if(NOT WIN32)
    target_compile_options(ICUHelper PRIVATE
        "SHELL:-iquote ${ICU_INCLUDE_DIRS}"
        "SHELL:-iquote ${CMAKE_SOURCE_DIR}/include"
    )
else()
    target_include_directories(ICUHelper PRIVATE ${CMAKE_SOURCE_DIR}/include)
endif()

# ============================================================================
# Library: ServerSDK (LIBDBC)
# ============================================================================
add_library(ServerSDK STATIC
    src/serversdk/AcceptConnect.cpp
    src/serversdk/Comm.cpp
    src/serversdk/CommRPC.cpp
    src/serversdk/DataSocket.cpp
    src/serversdk/DBCCommon.cpp
    src/serversdk/IniFile.cpp
    src/serversdk/Packet.cpp
    src/serversdk/PacketEncryption.cpp
    src/serversdk/PacketQueue.cpp
    src/serversdk/Point.cpp
    src/serversdk/Receiver.cpp
    src/serversdk/RWMutex.cpp
    src/serversdk/Sender.cpp
    src/serversdk/TaskQueue.cpp
    src/serversdk/ThreadPool.cpp
    src/serversdk/Timer.cpp
)
target_include_directories(ServerSDK PRIVATE
    ${PKO_INCLUDE_DIRS}
)
target_link_libraries(ServerSDK PRIVATE
    ${BOTAN_LIBRARY}
    Threads::Threads
)

# ============================================================================
# Library: Util
# ============================================================================
add_library(Util STATIC
    src/util/algo.cpp
    src/util/db.cpp
    src/util/db3.cpp
    src/util/md5c.c
    src/util/ErrorHandler.cpp
    src/util/LanguageRecord.cpp
    src/util/log.cpp
    src/util/log2.cpp
    src/util/logutil.cpp
    src/util/MapDataVer.cpp
    src/util/MapUtil.cpp
    src/util/SectionData.cpp
    src/util/se_translator.cpp
    src/util/Stacktrace.cpp
    src/util/sym_engine.cpp
    src/util/util2.cpp
)
target_include_directories(Util PRIVATE
    ${PKO_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src/util
    ${CMAKE_SOURCE_DIR}/include/ipss
    ${CMAKE_SOURCE_DIR}/include/sdl
)
target_link_libraries(Util PRIVATE
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# ============================================================================
# Library: Common
# ============================================================================
add_library(Common STATIC
    src/common/ChaAttr.cpp
    src/common/TableObfuscation.cpp
    src/common/ChaMask.cpp
    src/common/CharacterRecord.cpp
    src/common/CommFunc.cpp
    src/common/EventRecord.cpp
    src/common/ForgeRecord.cpp
    src/common/HairRecord.cpp
    src/common/ItemAttr.cpp
    src/common/ItemContent.cpp
    src/common/ItemRecord.cpp
    src/common/JobInitEquip.cpp
    src/common/Kitbag.cpp
    src/common/LevelRecord.cpp
    src/common/LifeLvRecord.cpp
    src/common/LogFile.cpp
    src/common/MonRefRecord.cpp
    src/common/NpcRecord.cpp
    src/common/SailLvRecord.cpp
    src/common/ShipSet.cpp
    src/common/SkillRecord.cpp
    src/common/SkillStateRecord.cpp
    src/common/StringLib.cpp
    src/common/SwitchMapRecord.cpp
    src/common/TerrainAttrib.cpp
    src/common/LicenseValidator.cpp
)
target_include_directories(Common PRIVATE
    ${PKO_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include/calua
)
target_link_libraries(Common PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
)

# ============================================================================
# Library: InfoNet (GameServer only)
# ============================================================================
add_library(InfoNet STATIC
    src/infonet/InfoNet.cpp
    src/infonet/lock.cpp
    src/infonet/log.cpp
    src/infonet/thread.cpp
)
target_include_directories(InfoNet PRIVATE
    ${CMAKE_SOURCE_DIR}/include/infonet
    ${CMAKE_SOURCE_DIR}/src/infonet
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/linux_compat
    ${CMAKE_SOURCE_DIR}/include/serversdk
)
target_compile_definitions(InfoNet PRIVATE __INFO_NET__)

# ============================================================================
# Library: Status (GateServer only)
# ============================================================================
add_library(Status STATIC
    src/status/UdpClient.cpp
    src/status/UdpManage.cpp
    src/status/UdpSocket.cpp
)
target_include_directories(Status PRIVATE
    ${CMAKE_SOURCE_DIR}/include/status
    ${CMAKE_SOURCE_DIR}/src/status
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/linux_compat
    ${CMAKE_SOURCE_DIR}/include/serversdk
)

# ============================================================================
# Executable: AccountServer
# ============================================================================
add_executable(AccountServer
    src/accountserver/AccountServer2.cpp
    src/accountserver/DataBaseCtrl.cpp
    src/accountserver/GlobalVariable.cpp
    src/accountserver/main.cpp
    src/accountserver/MyThread.cpp
    src/accountserver/stdafx.cpp
    src/accountserver/auth/AuthConfig.cpp
    src/accountserver/auth/RateLimiter.cpp
    src/accountserver/auth/AuthServiceLegacy.cpp
)
target_include_directories(AccountServer PRIVATE
    ${PKO_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include/AccountServer
    ${CMAKE_SOURCE_DIR}/src/accountserver
    ${CMAKE_SOURCE_DIR}/include/ipss
)
target_compile_definitions(AccountServer PRIVATE
    NOAUTHDLL
    BOTAN_FUTURE_INTERNAL_HEADER_OK
)
target_link_libraries(AccountServer PRIVATE
    ServerSDK Common Util ICUHelper EncLib
    ${ODBC_LIBRARY}
    ${ICU_LIBRARIES}
    ${BOTAN_LIBRARY}
    OpenSSL::SSL OpenSSL::Crypto
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# ============================================================================
# Executable: GateServer
# ============================================================================
add_executable(GateServer
    src/gateserver/GateServer.cpp
    src/gateserver/main.cpp
    src/gateserver/ToClient.cpp
    src/gateserver/ToGameServer.cpp
    src/gateserver/ToGroupServer.cpp
)
target_include_directories(GateServer PRIVATE
    ${PKO_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include/GateServer
    ${CMAKE_SOURCE_DIR}/src/gateserver
)
target_compile_definitions(GateServer PRIVATE
    NOCRYPT
    NARROW
    BOTAN_FUTURE_INTERNAL_HEADER_OK
)
target_link_libraries(GateServer PRIVATE
    ServerSDK Common Util ICUHelper Status EncLib
    ${ODBC_LIBRARY}
    ${ICU_LIBRARIES}
    ${BOTAN_LIBRARY}
    OpenSSL::SSL OpenSSL::Crypto
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# ============================================================================
# Executable: GroupServer
# ============================================================================
add_executable(GroupServer
    src/groupserver/DBConnect.cpp
    src/groupserver/DBPool.cpp
    src/groupserver/Friend.cpp
    src/groupserver/GroupServerApp.cpp
    src/groupserver/GroupServerAppChat.cpp
    src/groupserver/GroupServerAppFrnd.cpp
    src/groupserver/GroupServerAppGuld.cpp
    src/groupserver/GroupServerAppInit.cpp
    src/groupserver/GroupServerAppMaster.cpp
    src/groupserver/GroupServerAppPrsn.cpp
    src/groupserver/GroupServerAppServ.cpp
    src/groupserver/GroupServerAppTeam.cpp
    src/groupserver/GroupSMain.cpp
    src/groupserver/Guild.cpp
    src/groupserver/GuildMember.cpp
    src/groupserver/Invitation.cpp
    src/groupserver/LuaFunc.cpp
    src/groupserver/Master.cpp
    src/groupserver/Parser.cpp
    src/groupserver/Player.cpp
    src/groupserver/Script.cpp
    src/groupserver/SessionChat.cpp
    src/groupserver/stdafx.cpp
    src/groupserver/Team.cpp
    src/groupserver/TeamMember.cpp
)
target_include_directories(GroupServer PRIVATE
    ${PKO_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include/GroupServer
    ${CMAKE_SOURCE_DIR}/src/groupserver
    ${LUAJIT_INCLUDE_DIR}
)
target_link_libraries(GroupServer PRIVATE
    ServerSDK Common Util ICUHelper EncLib
    ${ODBC_LIBRARY}
    ${ICU_LIBRARIES}
    ${LUAJIT_LIBRARY}
    OpenSSL::SSL OpenSSL::Crypto
    Threads::Threads
    ${CMAKE_DL_LIBS}
    m   # math library
)

# ============================================================================
# Executable: GameServer
# ============================================================================
# NOTE: USE_IOCP is NOT defined — Linux uses select()-based GameServerApp path
add_executable(GameServer
    src/gameserver/Action.cpp
    src/gameserver/Attachable.cpp
    src/gameserver/AttachManage.cpp
    src/gameserver/Auction.cpp
    src/gameserver/AuctionItem.cpp
    src/gameserver/Birthplace.cpp
    src/gameserver/BossLastHit.cpp
    src/gameserver/BossTimer.cpp
    src/gameserver/cfl_iocp.cpp
    src/gameserver/cfl_lock.cpp
    src/gameserver/Character.cpp
    src/gameserver/CharacterAI.cpp
    src/gameserver/CharacterAIState.cpp
    src/gameserver/CharacterCmd.cpp
    src/gameserver/CharacterPacket.cpp
    src/gameserver/CharacterPrl.cpp
    src/gameserver/CharacterRun.cpp
    src/gameserver/CharacterSuperCmd.cpp
    src/gameserver/CharBoat.cpp
    src/gameserver/CharForge.cpp
    src/gameserver/CharScript.cpp
    src/gameserver/CharStall.cpp
    src/gameserver/OfflineStall.cpp
    src/gameserver/CharTrade.cpp
    src/gameserver/Config.cpp
    src/gameserver/Entity.cpp
    src/gameserver/EntityAlloc.cpp
    src/gameserver/EntityScript.cpp
    src/gameserver/EntitySpawn.cpp
    src/gameserver/EventCalculate.cpp
    src/gameserver/EventEntity.cpp
    src/gameserver/EventHandler.cpp
    src/gameserver/EyeshotCell.cpp
    src/gameserver/FightAble.cpp
    src/gameserver/FindPath.cpp
    src/gameserver/GameApp.cpp
    src/gameserver/GameAppNet.cpp
    src/gameserver/GameDB.cpp
    src/gameserver/GameServerApp.cpp
    src/gameserver/GameSMain.cpp
    src/gameserver/gmsvr.cpp
    src/gameserver/Guild.cpp
    src/gameserver/HarmRec.cpp
    src/gameserver/Item.cpp
    src/gameserver/lua_gamectrl.cpp
    src/gameserver/MapEntry.cpp
    src/gameserver/MapRes.cpp
    src/gameserver/MapTrigger.cpp
    src/gameserver/Mission.cpp
    src/gameserver/MoveAble.cpp
    src/gameserver/NPC.cpp
    src/gameserver/NpcScript.cpp
    src/gameserver/Parser.cpp
    src/gameserver/PicSet.cpp
    src/gameserver/Picture.cpp
    src/gameserver/Player.cpp
    src/gameserver/Script.cpp
    src/gameserver/StackWalker.cpp
    src/gameserver/StateCell.cpp
    src/gameserver/stdafx.cpp
    src/gameserver/SubMap.cpp
    src/gameserver/SystemDialog.cpp
    src/gameserver/TradeLogDB.cpp
    src/gameserver/Weather.cpp
    src/gameserver/WorldEudemon.cpp
    src/gameserver/xString.cpp
)
target_include_directories(GameServer PRIVATE
    ${PKO_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include/GameServer
    ${CMAKE_SOURCE_DIR}/src/gameserver
    ${CMAKE_SOURCE_DIR}/include/calua
    ${LUAJIT_INCLUDE_DIR}
)
target_compile_definitions(GameServer PRIVATE
    WINDOWS_IGNORE_PACKING_MISMATCH
    __INFO_NET__
    BOTAN_DLL=
)
target_link_libraries(GameServer PRIVATE
    ServerSDK Common Util ICUHelper InfoNet EncLib
    ${ODBC_LIBRARY}
    ${ICU_LIBRARIES}
    ${BOTAN_LIBRARY}
    ${LUAJIT_LIBRARY}
    OpenSSL::SSL OpenSSL::Crypto
    Threads::Threads
    ${CMAKE_DL_LIBS}
    m   # math library
)

# ============================================================================
# Install targets
# ============================================================================
install(TARGETS AccountServer GateServer GroupServer GameServer
    RUNTIME DESTINATION bin
)

message(STATUS "")
message(STATUS "PKO Server Linux Build Configuration:")
message(STATUS "  Build type:  ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  ODBC:         ${ODBC_LIBRARY}")
message(STATUS "  ICU:          ${ICU_LIBRARIES}")
message(STATUS "  Botan:        ${BOTAN_LIBRARY}")
message(STATUS "  LuaJIT:       ${LUAJIT_LIBRARY}")
message(STATUS "  OpenSSL:      ${OPENSSL_LIBRARIES}")
message(STATUS "")
