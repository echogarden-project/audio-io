dlltool --dllname node.exe --def ../definitions/node_api.def --output-lib node_api.lib

g++ windows-mme-output.cpp node_api.lib -o windows-x64-mme-output.node -I../include/napi -lwinmm -shared -fPIC -O0 -std=c++17
