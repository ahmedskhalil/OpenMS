### the directory name
set(directory source/VISUAL)

### list all filenames of the directory here
set(sources_list
AxisTickCalculator.cpp
AxisPainter.cpp
AxisWidget.cpp
ColorSelector.cpp
EnhancedTabBar.cpp
HistogramWidget.cpp
LayerData.cpp
MetaDataBrowser.cpp
MultiGradient.cpp
MultiGradientSelector.cpp
ParamEditor.cpp
SpectraViewWidget.cpp
SpectraIdentificationViewWidget.cpp
Spectrum1DCanvas.cpp
Spectrum1DWidget.cpp
Spectrum2DCanvas.cpp
Spectrum2DWidget.cpp
Spectrum3DCanvas.cpp
Spectrum3DOpenGLCanvas.cpp
Spectrum3DWidget.cpp
SpectrumCanvas.cpp
SpectrumWidget.cpp
ListEditor.cpp
TOPPASLogWindow.cpp
TOPPASWidget.cpp
TOPPASScene.cpp
TOPPASVertex.cpp
TOPPASToolVertex.cpp
TOPPASInputFileListVertex.cpp
TOPPASOutputFileListVertex.cpp
TOPPASMergerVertex.cpp
TOPPASEdge.cpp
TOPPASTabBar.cpp
TOPPASTreeView.cpp
TOPPASResource.cpp
TOPPASResources.cpp
TOPPViewBehaviorInterface.cpp
TOPPViewIdentificationViewBehavior.cpp
TOPPViewSpectraViewBehavior.cpp
EnhancedWorkspace.cpp
EnhancedTabBarWidgetInterface.cpp
)

### add path to the filenames
set(sources)
foreach(i ${sources_list})
	list(APPEND sources ${directory}/${i})
endforeach(i)

### pass source file list to the upper instance
set(OpenMSVisual_sources ${OpenMSVisual_sources} ${sources})

### source group definition
source_group("Source Files\\VISUAL" FILES ${sources})


### icons
# add   : icons are added to source/VISUAL/ICONS/resources.qrc
# remove: after removing an icon, you have to rerun 'cmake' to fix the dependencies
QT4_ADD_RESOURCES(qt_resource_file source/VISUAL/ICONS/resources.qrc)
set(OpenMSVisual_sources ${OpenMSVisual_sources} ${qt_resource_file})