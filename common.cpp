#include "common.hpp"

#include <python3.7m/Python.h>
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
				 "['', '/nix/store/yc5ilbccacp9npyjr6dayq904w9p49mc-opencv-4.5.2/lib/python3.7/site-packages', '/nix/store/2ylqgllg47w0hjylln7gh8xw1d1lx77c-python3.7-numpy-1.20.3/lib/python3.7/site-packages', '/nix/store/xsvipsgllvyg9ys19pm2pz9qpgfhzmp9-python3-3.7.11/lib/python3.7/site-packages', '/nix/store/yr3gsrcqziplz4c0kgp27r08pc9c7iq8-python3-3.7.11-env/lib/python3.7/site-packages', '/nix/store/xsvipsgllvyg9ys19pm2pz9qpgfhzmp9-python3-3.7.11/lib/python37.zip', '/nix/store/xsvipsgllvyg9ys19pm2pz9qpgfhzmp9-python3-3.7.11/lib/python3.7', '/nix/store/xsvipsgllvyg9ys19pm2pz9qpgfhzmp9-python3-3.7.11/lib/python3.7/lib-dynload', '/nix/store/irr2k540vpdxqj13m25hv4ckk2qnlg84-python3.7-smbus2-0.4.1/lib/python3.7/site-packages', '/nix/store/6806q91s3w717wlbx6h20gmg5n0gw88x-python3.7-rpi_gpio-0.7.0/lib/python3.7/site-packages', \n \
"
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
