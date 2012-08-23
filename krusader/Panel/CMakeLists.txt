set(Panel_SRCS
    listpanelmodule.cpp
    viewconfigtab.cpp
    selectionmodeconfigtab.cpp
    calcspacethread.cpp
    urlrequester.cpp
    kritemview.cpp
    quickfilter.cpp
    viewactions.cpp
    listpanelactions.cpp
    dirhistoryqueue.cpp
    krsort.cpp
    krlayoutfactory.cpp
    krerrordisplay.cpp
    krpreviews.cpp
    krpreviewjob.cpp
    krcolorcache.cpp 
    krcalcspacedialog.cpp 
    krpopupmenu.cpp 
    krpreviewpopup.cpp 
    viewmodule.cpp
    viewexpplaceholders.cpp
    krview.cpp 
    panelfunc.cpp
    listpanel.cpp
    listpanelframe.cpp
    panelpopup.cpp
    krselectionmode.cpp 
    krvfsmodel.cpp
    krinterview.cpp
    krinterbriefview.cpp
    krinterdetailedview.cpp
    krinterviewitemdelegate.cpp
    krviewfactory.cpp
    krmousehandler.cpp)

kde4_add_library(Panel STATIC ${Panel_SRCS})

target_link_libraries(Panel common Dialogs GUI KViewer ${KDE4_KFILE_LIBS} )


if(LIBKONQ_FOUND)
    target_link_libraries( Panel ${LIBKONQ_LIBRARY} )
endif(LIBKONQ_FOUND)
