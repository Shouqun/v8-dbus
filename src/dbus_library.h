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
  static v8::Handle<v8::Value> New(const v8::Arguments& args);

	static v8::Handle<v8::Value> SessionBus(const v8::Arguments& args);
  
  static v8::Handle<v8::Value> SystemBus(const v8::Arguments& args);

  static v8::Handle<v8::Value> GetInterface(const v8::Arguments& args);

  static v8::Handle<v8::Value> GetMethod(const v8::Arguments& args);

  static v8::Handle<v8::Value> GetSignal(const v8::Arguments& args);

public:
  static v8::Persistent<v8::ObjectTemplate> g_conn_template_;
};

}

#endif


