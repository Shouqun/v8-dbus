#include <v8.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#include "dbus_library.cc"

namespace dbus_library{
  
DBusExtension::DBusExtension() {
  //the consctruction call
}

DBusExtension::~DBusExtension() {
  //the desctruction call
}

v8::Handle<v8::Value> DBusExtension::Initialize(
                        v8::Handle<v8::Object>& target) {
  v8::HandleScope scope;

  v8::Local<v8::FunctionTemplate> t 
          = v8::FunctionTemplate::New(DBusExtension::New);
  
  v8::Local<v8::FunctionTemplate> session_bus_t
          = v8::FunctionTemplate::New(DBusExtension::SessionBus);
  v8::Local<v8::FunctionTemplate> system_bus_t
          = v8::FunctionTemplate::New(DBusExtension::SystemBus);
  v8::Local<v8::FuntcionTemplate> get_interface_t
          = v8::FunctionTemplate::New(DBusExtension::GetInterface);
  v8::Local<v8::FunctionTemplate> get_method_t
          = v8::FunctionTemplate::New(DBusExtension::GetMethod);

  target->Set(v8::String::New("SessionBus"), session_bus_t->GetFunction());
  target->Set(v8::String::New("SystemBus"), system_bus_t->GetFunction());
  target->Set(v8::String::New("GetInterface"), get_interface_t->GetFunction());
  target->Set(v8::String::New("GetMethod"), get_method_t->GetFunction());

}

v8::Handle<v8::Value> DBusExtension::New(v8::Arguments& args) {
  
}

v8::Handle<v8::Value> DBusExtension::SessionBus(v8::Arguments& args) {
}

v8::Handle<v8::Value> DBusExtension::SystemBus(v8::Arguments& args) {
}

v8::Handle<v8::Value> DBusExtension::GetInterface(v8::Arguments& args) {
}

v8::Handle<v8::Value> DBusExtension::GetMethod(v8::Arguments& args) {
}

v8::Handle<v8::Value> DBusExtension::GetSignal(v8::Arguments& args) {
}

}

