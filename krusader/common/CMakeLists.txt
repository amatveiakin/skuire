set(common_SRCS
    module.cpp
    viewtype.cpp
    viewfactory.cpp)

kde4_add_library(common STATIC ${common_SRCS})
