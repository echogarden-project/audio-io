dlltool --dllname node.exe --def ../definitions/node_api.def --output-lib node_api.lib

cl.exe windows-mme-output.cpp node_api.lib /I"../include/napi" /Fe"windows-x64-mme-output.node" /DNAPI_CPP_EXCEPTIONS /EHsc /std:c++20 /LD /O1 /link "winmm.lib"

del *.obj *.exp windows-x64-mme-output.lib
