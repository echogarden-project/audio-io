dlltool --dllname node.exe --def ../definitions/node_api.def --output-lib node_api.lib

clang++ windows-mme-output.cpp node_api.lib -o windows-x64-mme-output.node -I ../include/napi -l winmm -shared -O1 -std=c++17
