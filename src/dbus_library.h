#ifndef _DBUS_LIBRARY_H
#define _DBUS_LIBRARY_H

#include <v8.h>

namespace dbus_library {

class DBusExtension {

public:
  DBusExtension();
  ~DBusExtension();

public:
  static v8::Handle<v8::Value> Initialize(v8::Handle<v8::Object>& handle);

public:
  //New instance
  static v8::Handle<v8::Value> New(const v8::Arguments& args);

  //SessionBus: Return a v8 wrapped D-Bus session bus object
  // arguments not required
	static v8::Handle<v8::Value> SessionBus(const v8::Arguments& args);
 
  //SystemBus: Return a v8 wrapped D-Bus system bus object
  // arguments not required
  static v8::Handle<v8::Value> SystemBus(const v8::Arguments& args);

  //GetInterface: Get v8 interface object by given arguments
  //  args[0]: the bus object returned by SystemBus/SessionBus
  //  args[1]: string object of service name
  //  args[2]: string object of object path
  //  args[3]: string object of interface name
  static v8::Handle<v8::Value> GetInterface(const v8::Arguments& args);

  //GetMethod: Get v8 method object accroding to D-Bus interface
  static v8::Handle<v8::Value> GetMethod(const v8::Arguments& args);

  //GetSignal: Get v8 signal object accroding to D-Bus interface
  static v8::Handle<v8::Value> GetSignal(const v8::Arguments& args);

  //MainLoop: The main message loop to dispatch received signals
  //  To make signal object callback called, this mainloop should be run
  static v8::Handle<v8::Value> MainLoop(const v8::Arguments& args);

public:
  static v8::Persistent<v8::ObjectTemplate> g_conn_template_;
};

}

#endif


