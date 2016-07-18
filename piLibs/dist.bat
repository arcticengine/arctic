mkdir dist\include\libApp
mkdir dist\include\libCamera
mkdir dist\include\libDataUtils
mkdir dist\include\libImage
mkdir dist\include\libImage\color
mkdir dist\include\libImage\processing
mkdir dist\include\libLog
mkdir dist\include\libMath
mkdir dist\include\libMesh
mkdir dist\include\libMesh\formats
mkdir dist\include\libMesh\generation
mkdir dist\include\libMisc
mkdir dist\include\libMisc\formats
mkdir dist\include\libMisc\formats\piSexyVideo
mkdir dist\include\libMisc\spline
mkdir dist\include\libMisc\string
mkdir dist\include\libMisc\piGUI
mkdir dist\include\libMisc\piRenderMesh
mkdir dist\include\libMisc\piTextContainer
mkdir dist\include\libMisc\piMidi
mkdir dist\include\libMisc\piTextureUtils
mkdir dist\include\libMisc\piDraw
mkdir dist\include\libMisc\piCamera
mkdir dist\include\libRender
mkdir dist\include\libSound
mkdir dist\include\libSystem
mkdir dist\include\libVR
mkdir dist\include\libNetwork
mkdir dist\include\libNetwork\piHttp
mkdir dist\include\libNetwork\piServerClient


copy src\libApp\piMain.h dist\include\libApp\

copy src\libCamera\piCam.h dist\include\libCamera\

copy src\libDataUtils\piArray.h dist\include\libDataUtils\
copy src\libDataUtils\piTArray.h dist\include\libDataUtils\
copy src\libDataUtils\piTPool.h dist\include\libDataUtils\
copy src\libDataUtils\piStack.h dist\include\libDataUtils\
copy src\libDataUtils\piUOIntMap.h dist\include\libDataUtils\
copy src\libDataUtils\piUOIntIntMap.h dist\include\libDataUtils\
copy src\libDataUtils\piUOStringMap.h dist\include\libDataUtils\
copy src\libDataUtils\piString.h dist\include\libDataUtils\
copy src\libDataUtils\piRadixSort.h dist\include\libDataUtils\

copy src\libImage\piImage.h dist\include\libImage\
copy src\libImage\processing\*.h dist\include\libImage\processing\
copy src\libImage\color\piIColor.h dist\include\libImage\color\

copy src\libMath\piVecTypes.h dist\include\libMath\
copy src\libMath\piNoise.h dist\include\libMath\
copy src\libMath\piVoronoi.h dist\include\libMath\
copy src\libMath\piRandom.h dist\include\libMath\
copy src\libMath\piShading.h dist\include\libMath\
copy src\libMath\piColor.h dist\include\libMath\
copy src\libMath\piFft.h dist\include\libMath\
copy src\libMath\piComplex.h dist\include\libMath\

copy src\libRender\piRenderer.h dist\include\libRender\
copy src\libRender\bin\Win32\Debug\libRender.lib ..\lib\Win32\Debug

copy src\libLog\piLog.h dist\include\libLog\

copy src\libMisc\formats\piJSONWriter.h dist\include\libMisc\formats\
copy src\libMisc\formats\piJSONReader.h dist\include\libMisc\formats\
copy src\libMisc\formats\piXmlWriter.h dist\include\libMisc\formats\
copy src\libMisc\formats\piXmlReader.h dist\include\libMisc\formats\
copy src\libMisc\formats\piTxtReader.h dist\include\libMisc\formats\
copy src\libMisc\formats\piWav.h dist\include\libMisc\formats\
copy src\libMisc\formats\piAvi.h dist\include\libMisc\formats\
copy src\libMisc\formats\piSexyVideo\piSexyVideo.h dist\include\libMisc\formats\piSexyVideo
copy src\libMisc\spline\piSpline.h dist\include\libMisc\spline\
copy src\libMisc\piDraw\piDraw.h dist\include\libMisc\piDraw\
copy src\libMisc\piCamera\piCameraF.h dist\include\libMisc\piCamera\
copy src\libMisc\piGUI\piRenderFont.h dist\include\libMisc\piGUI\
copy src\libMisc\piGUI\piRenderFont2D.h dist\include\libMisc\piGUI\
copy src\libMisc\piGUI\piRenderFont3D.h dist\include\libMisc\piGUI\
copy src\libMisc\piGUI\piRenderIcons.h dist\include\libMisc\piGUI\
copy src\libMisc\piGUI\piGUI.h dist\include\libMisc\piGUI\
copy src\libMisc\piRenderMesh\piRenderMesh.h dist\include\libMisc\piRenderMesh\
copy src\libMisc\piMidi\piMidiIn.h dist\include\libMisc\piMidi\
copy src\libMisc\piTextContainer\piTextContainer.h dist\include\libMisc\piTextContainer\
copy src\libMisc\piTextureUtils\*.h dist\include\libMisc\piTextureUtils\

copy src\libSound\piSound.h dist\include\libSound\

copy src\libSystem\piDebug.h dist\include\libSystem\
copy src\libSystem\piWindow.h dist\include\libSystem\
copy src\libSystem\piFile.h dist\include\libSystem\
copy src\libSystem\piFileName.h dist\include\libSystem\
copy src\libSystem\piFindFiles.h dist\include\libSystem\
copy src\libSystem\piFileMonitor.h dist\include\libSystem\
copy src\libSystem\piDll.h dist\include\libSystem\
copy src\libSystem\piTypes.h dist\include\libSystem\
copy src\libSystem\piTimer.h dist\include\libSystem\
copy src\libSystem\piThread.h dist\include\libSystem\
copy src\libSystem\piSystemInfo.h dist\include\libSystem\
copy src\libSystem\piSystemFolders.h dist\include\libSystem\
copy src\libSystem\piStr.h dist\include\libSystem\
copy src\libSystem\piSpinlock.h dist\include\libSystem\
copy src\libSystem\piShMem.h dist\include\libSystem\
copy src\libSystem\piMutex.h dist\include\libSystem\
copy src\libSystem\piTcpIp.h dist\include\libSystem\
copy src\libSystem\piGfxDialog.h dist\include\libSystem\

copy src\libMesh\piMesh.h dist\include\libMesh\
copy src\libMesh\piSkeleton.h dist\include\libMesh
copy src\libMesh\formats\piMeshObj.h dist\include\libMesh\formats
copy src\libMesh\formats\piMeshPly.h dist\include\libMesh\formats
copy src\libMesh\generation\piMesh_Generate.h dist\include\libMesh\generation
copy src\libMesh\generation\piMesh_PrimSimple.h dist\include\libMesh\generation
copy src\libMesh\generation\piMesh_ModSimple.h dist\include\libMesh\generation
copy src\libMesh\generation\piMesh_IteSimple.h dist\include\libMesh\generation
copy src\libMesh\generation\piMesh_ModComplex.h dist\include\libMesh\generation
copy src\libMesh\generation\piMesh_FaceOps.h dist\include\libMesh\generation

copy src\libVR\piVR.h dist\include\libVR\

copy src\libNetwork\piHttp\piHTTP.h dist\include\libNetwork\piHttp\
copy src\libNetwork\piServerClient\piClient.h dist\include\libNetwork\piServerClient\
copy src\libNetwork\piServerClient\piServer.h dist\include\libNetwork\piServerClient\
copy src\libNetwork\piServerClient\piMessage.h dist\include\libNetwork\piServerClient\

