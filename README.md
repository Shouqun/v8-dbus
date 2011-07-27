v8-dbus is a JavaScript? D-Bus binding for v8
===
How to build

The v8-dbus depends on the v8 engine to build. Checkout v8 code from http://code.google.com/p/v8/ or https://github.com/v8/v8, and then put it into "deps" directory. In the root directory run the following command to build:
    $tools/waf-light configure build

The build relies on scons http://www.scons.org/ to build v8 and waf http://code.google.com/p/waf/ to build the v8-dbus. However, they have already in the source tree and in tools directory and no need to download them. After build finish, the binary is in "build/default" directory.

