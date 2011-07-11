#include <v8.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#include <iostream>

#include "dbus_library.h"

namespace dbus_library{

/// The real function that returns the DBusConnecton
/// return session bus or system bus according to the type
static DBusGConnection* GetBusFromType(DBusBusType type) {
  DBusGConnection *connection;
  GError *error;

  g_type_init();
  error = NULL;
  connection = NULL; 
  connection = dbus_g_bus_get(type, &error);
  if (connection == NULL)
  {
    std::cerr<<"Failed to open connection to bus: "<<error->message;
    g_error_free (error);
    return connection;
  }

  //std::cout<<"Connection:"<<reinterpret_cast<int>(connection);
  return connection;
}

//static object
v8::Persistent<v8::ObjectTemplate> DBusExtension::g_conn_template_;

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
  v8::Local<v8::FunctionTemplate> get_interface_t
          = v8::FunctionTemplate::New(DBusExtension::GetInterface);
  v8::Local<v8::FunctionTemplate> get_method_t
          = v8::FunctionTemplate::New(DBusExtension::GetMethod);

  target->Set(v8::String::New("SessionBus"), session_bus_t->GetFunction());
  target->Set(v8::String::New("SystemBus"), system_bus_t->GetFunction());
  target->Set(v8::String::New("GetInterface"), get_interface_t->GetFunction());
  target->Set(v8::String::New("GetMethod"), get_method_t->GetFunction());

}

v8::Handle<v8::Value> DBusExtension::New(const v8::Arguments& args) {
  return v8::Undefined();
}

v8::Handle<v8::Value> DBusExtension::SessionBus(const v8::Arguments& args) {
  v8::HandleScope scope;

  if (g_conn_template_.IsEmpty()) {
    v8::Handle<v8::ObjectTemplate> obj_template = v8::ObjectTemplate::New();
    obj_template->SetInternalFieldCount(1);
    g_conn_template_ = v8::Persistent<v8::ObjectTemplate>::New(obj_template);
  }

  DBusGConnection *connection = GetBusFromType(DBUS_BUS_SESSION);
  if (connection == NULL)  {
    std::cerr<<"Error get session bus\n";
    return v8::Undefined();
  }

  v8::Local<v8::Object> conn_object = g_conn_template_->NewInstance();
  conn_object->SetInternalField(0, v8::External::New(connection));
  
  std::cout<<"return connection object\n";
  return scope.Close(conn_object); 
}

v8::Handle<v8::Value> DBusExtension::SystemBus(const v8::Arguments& args) {
  v8::HandleScope scope;

  if (g_conn_template_.IsEmpty()) {
    v8::Handle<v8::ObjectTemplate> obj_template = v8::ObjectTemplate::New();
    obj_template->SetInternalFieldCount(1);
    g_conn_template_ = v8::Persistent<v8::ObjectTemplate>::New(obj_template);
  }

  DBusGConnection *connection = GetBusFromType(DBUS_BUS_SYSTEM);
  if (connection == NULL)
    return v8::Undefined();

  v8::Local<v8::Object> conn_object = g_conn_template_->NewInstance();
  conn_object->SetInternalField(0, v8::External::New(connection));

  return scope.Close(conn_object); 
}

v8::Handle<v8::Value> DBusExtension::GetInterface(const v8::Arguments& args) {
  v8::HandleScope scope;

  if (args.Length() <4) {
    std::cerr<<"DBusExtension::GetInterface Arguments not enough\n";
    return v8::Undefined();
  }

  v8::Local<v8::Object> bus_object = args[0]->ToObject();
  v8::String::Utf8Value service_name(args[1]->ToString());
  v8::String::Utf8Value object_path(args[2]->ToString());
  v8::String::Utf8Value interface_name(args[3]->ToString());

  DBusGConnection *connection = (DBusGConnection*) v8::External::Unwrap(
                                     bus_object->GetInternalField(0));
  //Start DBus Proxy call
  GError *error;
  DBusGProxy *proxy;
  char *iface_data;
  
  //get the proxy object
  proxy = dbus_g_proxy_new_for_name(connection, *service_name, *object_path,
            "org.freedesktop.DBus.Introspectable");

  //call the proxy method 
  error = NULL;
  if (!dbus_g_proxy_call(proxy, "Introspect", &error, G_TYPE_INVALID, 
          G_TYPE_STRING, &iface_data, G_TYPE_INVALID)) {
    /* Just do demonstrate remote exceptions versus regular GError */
    if (error->domain == DBUS_GERROR
         && error->code == DBUS_GERROR_REMOTE_EXCEPTION)
      g_printerr("Caught remote method exception %s: %s",
                 dbus_g_error_get_name (error),
                 error->message);
     else
       g_printerr ("Error: %s\n", error->message);
     g_error_free (error);
     return v8::Undefined();
  }
  
  std::cout<<iface_data<<std::endl;
  v8::Local<v8::String> iface_string = v8::String::New(iface_data);
  g_free(iface_data);

  return scope.Close(iface_string);
}

v8::Handle<v8::Value> DBusExtension::GetMethod(const v8::Arguments& args) {
  return v8::Undefined();
}

v8::Handle<v8::Value> DBusExtension::GetSignal(const v8::Arguments& args) {
  return v8::Undefined();
}

}

