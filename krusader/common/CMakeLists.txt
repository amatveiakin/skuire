set(common_SRCS
    vfs/copy.cpp
    vfs/virt.cpp
    module.cpp
    actionsbase.cpp
    viewtype.cpp
    viewfactory.cpp
    abstractview.cpp
    abstractlistpanel.cpp)

kde4_add_library(common STATIC ${common_SRCS})
