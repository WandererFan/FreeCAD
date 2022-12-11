
add_definitions(-DMOD_TECHDRAW_HANDLE_FACES=1)

add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

set(TechDraw_Scripts
    Init.py
    TestTechDrawApp.py
    TestTechDrawGui.py
)

if(BUILD_GUI)
    list (APPEND TechDraw_Scripts InitGui.py)
endif(BUILD_GUI)

set(TechDraw_ToolsScripts
    TechDrawTools/__init__.py
    TechDrawTools/CommandMoveView.py
    TechDrawTools/CommandShareView.py
    TechDrawTools/CommandAxoLengthDimension.py
    TechDrawTools/CommandHoleShaftFit.py
    TechDrawTools/TaskMoveView.py
    TechDrawTools/TaskShareView.py
    TechDrawTools/TaskHoleShaftFit.py
    TechDrawTools/TDToolsUtil.py
    TechDrawTools/TDToolsMovers.py
)

set(TechDraw_DiagramScripts
    TechDrawDiagram/__init__.py
    TechDrawDiagram/CommandAddDiagram.py
    TechDrawDiagram/CommandAddSymbol.py
    TechDrawDiagram/CommandRemoveSymbol.py
    TechDrawDiagram/TaskAddSymbol.py
    TechDrawDiagram/TaskRemoveSymbol.py
    TechDrawDiagram/CommandAddTrace.py
    TechDrawDiagram/CommandRemoveTrace.py
    TechDrawDiagram/TaskAddTrace.py
    TechDrawDiagram/TaskRemoveTrace.py
    TechDrawDiagram/TDDiagramUtil.py
    TechDrawDiagram/TDDiagramWorkers.py
    TechDrawDiagram/TDDiagramTypeManager.py
    TechDrawDiagram/Rez.py
    TechDrawDiagram/TDDiagramPainters/SvgPainter.py
    TechDrawDiagram/TDDiagramPainters/TracePencil.py
    TechDrawDiagram/TDDiagramPainters/PathPainter.py

)

set(TechDraw_ToolsGui
    Gui/TaskMoveView.ui
    Gui/TaskHoleShaftFit.ui
    Gui/DlgPageChooser.ui
)
set(TechDraw_DiagramGui
    Gui/DiagramSymbolAdd.ui
    Gui/DiagramTraceAdd.ui
)

SET(TechDraw_PATFile
    PAT/FCPAT.pat
)

SET(TechDraw_LineGroupFile
    LineGroup/LineGroup.csv
)

add_custom_target(TechDrawScripts ALL SOURCES
    ${TechDraw_Scripts}
    ${TechDraw_tests}
)

add_custom_target(TechDraw_Data ALL
SOURCES ${TechDraw_PATFile} ${TechDraw_LineGroupFile}
)

add_custom_target(TechDraw_Tools ALL
SOURCES ${TechDraw_ToolsScripts} ${TechDraw_ToolsGui}
)

add_custom_target(TechDraw_Diagram ALL
SOURCES ${TechDraw_DiagramScripts} ${TechDraw_DiagramGui}
)

fc_target_copy_resource(TechDrawScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/TechDraw
    ${TechDraw_Scripts}
    ${TechDraw_tests}
)

if(BUILD_GUI)
fc_target_copy_resource(TechDraw_Tools
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/TechDraw
    ${TechDraw_ToolsScripts}
)

fc_target_copy_resource(TechDraw_Tools
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/TechDraw/TechDrawTools
    ${TechDraw_ToolsGui}
)

fc_target_copy_resource(TechDraw_Diagram
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/TechDraw
    ${TechDraw_DiagramScripts}
)

fc_target_copy_resource(TechDraw_Diagram
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/TechDraw/TechDrawDiagram
    ${TechDraw_DiagramGui}
)
endif(BUILD_GUI)

fc_target_copy_resource(TechDraw_Data
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/TechDraw
    ${TechDraw_PATFile})

fc_target_copy_resource(TechDraw_Data
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/TechDraw
    ${TechDraw_LineGroupFile})

INSTALL(
    FILES
        ${TechDraw_Scripts}
    DESTINATION
        Mod/TechDraw
)

INSTALL(
    DIRECTORY
        Templates
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Mod/TechDraw
    FILES_MATCHING
        PATTERN "*.svg*"
)

INSTALL(
    DIRECTORY
        PAT
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Mod/TechDraw
    FILES_MATCHING
        PATTERN "*.pat*"
)

INSTALL(
    DIRECTORY
        LineGroup
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Mod/TechDraw
    FILES_MATCHING
        PATTERN "*.csv*"
)

INSTALL(
    DIRECTORY
        Patterns
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Mod/TechDraw
    FILES_MATCHING
        PATTERN "*.svg*"
)

INSTALL(
    DIRECTORY
        Symbols
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Mod/TechDraw
    FILES_MATCHING
        PATTERN "*.svg*"
)

INSTALL(
    FILES
        ${TechDraw_ToolsScripts}
    DESTINATION
        Mod/TechDraw/TechDrawTools
)

INSTALL(
    FILES
        ${TechDraw_ToolsGui}
    DESTINATION
        Mod/TechDraw/TechDrawTools/Gui
)

INSTALL(
    FILES
        ${TechDraw_DiagramScripts}
    DESTINATION
        Mod/TechDraw/TechDrawDiagram
)

INSTALL(
    FILES
        ${TechDraw_DiagramGui}
    DESTINATION
        Mod/TechDraw/TechDrawDiagram/Gui
)

#unit test files
SET(TDTest_SRCS
    TDTest/__init__.py
    TDTest/DrawHatchTest.py
    TDTest/DrawProjectionGroupTest.py
    TDTest/DrawViewAnnotationTest.py
    TDTest/DrawViewImageTest.py
    TDTest/DrawViewSymbolTest.py
    TDTest/DrawViewDimensionTest.py
    TDTest/DrawViewPartTest.py
    TDTest/DrawViewSectionTest.py
    TDTest/DrawViewBalloonTest.py
    TDTest/DrawViewDetailTest.py
    TDTest/TechDrawTestUtilities.py
)

SET(TDTestFile_SRCS
    TDTest/TestHatch.svg
    TDTest/TestImage.png
    TDTest/TestSymbol.svg
    TDTest/TestTemplate.svg
)

SET(TDAllTest
    ${TDTest_SRCS}
    ${TDTestFile_SRCS}
)

ADD_CUSTOM_TARGET(TDTestTarget ALL
    SOURCES ${TDAllTest}
)

fc_copy_sources(TDTestTarget "${CMAKE_BINARY_DIR}/Mod/TechDraw" ${TDAllTest})

# install Python packages (for make install)
INSTALL(FILES ${TDTest_SRCS} DESTINATION Mod/TechDraw/TDTest)
INSTALL(FILES ${TDTestFile_SRCS} DESTINATION Mod/TechDraw/TDTest)

