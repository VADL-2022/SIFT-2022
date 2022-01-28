#include <python3.8/Python.h>
#include <iostream>

namespace common {
  void connect_Python() {
    using namespace std;

    cout << "Python: Connecting" << endl;

    Py_Initialize();
    int ret = PyRun_SimpleString("import sys; #print(sys.path); \n\
print('Python version:', sys.version, 'with executable at', sys.executable) \n\
for p in "
				 #ifdef __APPLE__
				 "['', '/nix/store/kqqwh1xz3ri47dvg8ikj7w2yl344amw3-python3-3.7.11/lib/python37.zip', '/nix/store/kqqwh1xz3ri47dvg8ikj7w2yl344amw3-python3-3.7.11/lib/python3.7', '/nix/store/kqqwh1xz3ri47dvg8ikj7w2yl344amw3-python3-3.7.11/lib/python3.7/lib-dynload', '/nix/store/kqqwh1xz3ri47dvg8ikj7w2yl344amw3-python3-3.7.11/lib/python3.7/site-packages', '/nix/store/smc86ndziwyi2vzjzvhh9h2dyscvh2rx-python3-3.7.11-env/lib/python3.7/site-packages', \n"
				 #else
				 "['', '/nix/store/aynlwrqdx1wxya1whshgz0spx75x1w30-python3-3.8.11/lib/python3.8/site-packages', '/nix/store/qpxyykbyccdz5rr7vwibbjc6hh3lc27w-python3-3.8.11-env/lib/python3.8/site-packages', '/nix/store/aynlwrqdx1wxya1whshgz0spx75x1w30-python3-3.8.11/lib/python38.zip', '/nix/store/aynlwrqdx1wxya1whshgz0spx75x1w30-python3-3.8.11/lib/python3.8', '/nix/store/aynlwrqdx1wxya1whshgz0spx75x1w30-python3-3.8.11/lib/python3.8/lib-dynload', \n" \

				 #endif
				 "\n\
]: \n\
    sys.path.append(p); \n\
print(sys.path)");
    if (ret == -1) {
      cout << "Failed to setup Python path" << endl;
      exit(1);
    }

    cout << "Python: Connected" << endl;
  }
  
} // namespace common
