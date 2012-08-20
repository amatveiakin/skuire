set(common_SRCS
    module.cpp
    actionsbase.cpp
    viewtype.cpp
    viewfactory.cpp
    abstractview.cpp
    abstractlistpanel.cpp)

kde4_add_library(common STATIC ${common_SRCS})
