dlltool --dllname node.exe --def ../definitions/node_api.def --output-lib node_api.lib

cl.exe windows-mme-output.cpp node_api.lib /I"../include/napi" /Fe"windows-x64-mme-output.node" /std:c++17 /EHsc /O1 /LD /link "winmm.lib"

del *.obj *.exp windows-x64-mme-output.lib
