#include <v8.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <list>
#include <string>
#include <map>
#include <iostream>

#include "dbus_library.h"
#include "dbus_introspect.h"

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
    std::cerr<<"Failed to open connection to bus: "<<error->message<<"\n";
    g_error_free (error);
    return connection;
  }

  return connection;
}

class DBusMethodContainer {
public:
  DBusGConnection *connection;
  std::string  service;
  std::string  path;
  std::string  interface;
  std::string  method;
  std::string  signature;
};

class DBusSignalContainer {
public:
  DBusGConnection *connection;
  std::string service;
  std::string path;
  std::string interface;
  std::string signal;
  std::string match;
};

//static object
v8::Persistent<v8::ObjectTemplate> DBusExtension::g_conn_template_;
static bool g_is_signal_filter_attached_ = false;

DBusExtension::DBusExtension() {
  //the consctruction call
}

DBusExtension::~DBusExtension() {
  //the desctruction call
}

static v8::Handle<v8::Value> LogCallback(const v8::Arguments& args) {
  v8::HandleScope scope;
  v8::Handle<v8::Value> arg = args[0];
  v8::String::Utf8Value value(arg);
  
  std::cout<< *value <<"\n";
  return v8::Undefined();
}

static v8::Handle<v8::Value> GetMethod(v8::Local<v8::Object>& iface,
                                         DBusGConnection *connection,
                                         const char *service_name,
                                         const char *object_path,
                                         const char *interface_name,
                                         BusMethod *method);

static v8::Handle<v8::Value> GetSignal(v8::Local<v8::Object>& iface,
                                         DBusGConnection *connection,
                                         const char *service_name,
                                         const char *object_path,
                                         const char *interface_name,
                                         BusSignal *signal);
                                         
                                         
//the signal map stores v8 Persistent object by the signal string
typedef std::map<std::string, v8::Handle<v8::Object> > SignalMap;
SignalMap g_signal_object_map;

std::string GetSignalMatchRule(DBusSignalContainer *container) {
  std::string result;
  
  result = container->interface + "." + container->signal;
  
  //std::cout<<"GetSignalMatchRule:"<<result;
  return result;
}

std::string GetSignalMatchRuleByString(std::string interface,
                                       std::string signal) {
  std::string result;
  
  result = interface + "." + signal;
  
  //std::cout<<"GetSignalMatchRuleByString:"<<result;
  return result;
}

v8::Handle<v8::Value> GetSignalObject(DBusSignalContainer *signal) {
  std::string match_rule = GetSignalMatchRule(signal);
 
  SignalMap::iterator ite = g_signal_object_map.find(match_rule);
  if (ite == g_signal_object_map.end() ) {
    return v8::Undefined();
  }
   
  return ite->second;
}

v8::Handle<v8::Value> GetSignalObjectByString(std::string match_rule) {
  //std::cout<<"GetSignalObjectByString:"<<match_rule; 
  SignalMap::iterator ite = g_signal_object_map.find(match_rule);
  if (ite == g_signal_object_map.end() ) {
    return v8::Undefined();
  }
  
  //std::cout<<"Find the match"; 
  return ite->second;
}

void AddSignalObject(DBusSignalContainer *signal,
                           v8::Handle<v8::Object> signal_obj) {
  std::string match_rule = GetSignalMatchRule(signal);
  
  SignalMap::iterator ite = g_signal_object_map.find(match_rule);
  if (ite == g_signal_object_map.end() ) {
    std::cout<<"We are to add the signal object";
    g_signal_object_map.insert( make_pair(match_rule,
          signal_obj));
  }
}

void RemoveSignalObject(DBusSignalContainer *signal) {
  std::string match_rule = GetSignalMatchRule(signal);
  // remove the matching item from signal object map
  std::cout<<"We are going to remove the object map item";
  g_signal_object_map.erase(match_rule);
}


/// dbus_signal_weak_callback: MakeWeak callback for signal Persistent
///    object,
static void dbus_signal_weak_callback(v8::Persistent<v8::Value> value, 
                                      void* parameter) {
  std::cout<<"dbus_signal_weak_callback";

  DBusSignalContainer *container = (DBusSignalContainer*) parameter;
  std::cout<<"Get the container obejct:" ;//<< (int) container;
  if (container != NULL) {
    //Remove the matching objet map item from the map
    RemoveSignalObject(container); 
    delete container;
  }

  value.Dispose();
  value.Clear();
}                                       

/// dbus_method_weak_callback: MakeWeak callback for method Persistent
///   obect
static void dbus_method_weak_callback(v8::Persistent<v8::Value> value,
                                      void* parameter) {
  std::cout<<"dbus_method_weak_callback";
  DBusMethodContainer *container = (DBusMethodContainer*) parameter;
  std::cout<<"Get the container object:"; //<< (int) container;
  if (container != NULL)
    delete container;

  value.Dispose();
  value.Clear();
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
  v8::Local<v8::FunctionTemplate> main_loop_t
          = v8::FunctionTemplate::New(DBusExtension::MainLoop);

  target->Set(v8::String::New("SessionBus"), session_bus_t->GetFunction());
  target->Set(v8::String::New("SystemBus"), system_bus_t->GetFunction());
  target->Set(v8::String::New("GetInterface"), get_interface_t->GetFunction());
  target->Set(v8::String::New("GetMethod"), get_method_t->GetFunction());
  target->Set(v8::String::New("MainLoop"), main_loop_t->GetFunction());
  target->Set(v8::String::New("Log"), 
                  v8::FunctionTemplate::New(LogCallback)->GetFunction());
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

// /To receive signal, need to start message loop and dispatch signals
v8::Handle<v8::Value> DBusExtension::MainLoop(const v8::Arguments& args) {
  v8::HandleScope scope;
  
  v8::Local<v8::Object> bus_object = args[0]->ToObject();
  DBusGConnection *connection = (DBusGConnection*) v8::External::Unwrap(
                                      bus_object->GetInternalField(0));

  std::cout<<"Get the conneciont object and start message loop\n";
  
  while (true) {
//    dbus_connection_read_write_dispatch(
//          dbus_g_connection_get_connection(connection), -1);

    g_main_context_iteration(g_main_context_default(), false);
  } 

  return v8::Undefined();
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
  std::string introspect_data(iface_data);
  g_free(iface_data);
  
  //Parser the Introspect and get the D-Bus node object
  Parser *parser = ParseIntrospcect(introspect_data.c_str(), 
                                    introspect_data.length());
  
  if (parser == NULL)
    return v8::Undefined();

  //get the dest inteface obejct
  BusInterface *interface = ParserGetInterface(parser, *interface_name);
  if (interface == NULL) {
    std::cerr<<"Error, No such interface\n";
    return v8::Undefined();
  }
  //create the Interface object to return
  v8::Local<v8::Object> interface_object = v8::Object::New();
  interface_object->Set(v8::String::New("xml_source"), v8::String::New(iface_data));

  //get all methods
  std::list<BusMethod*>::iterator method_ite;
  for( method_ite = interface->methods_.begin();
        method_ite != interface->methods_.end();
        method_ite++) {
    BusMethod *method = *method_ite;
    //get method object    
    dbus_library::GetMethod(interface_object, connection, *service_name, *object_path,
               *interface_name, method);
  }

  //get all interface
  std::list<BusSignal*>::iterator signal_ite;
  for( signal_ite = interface->signals_.begin();
         signal_ite != interface->signals_.end();
         signal_ite++ ) {
    BusSignal *signal = *signal_ite;
    //get siangl object
    dbus_library::GetSignal(interface_object, connection, *service_name,
              *object_path, *interface_name, signal);
  }

  ParserRelease(&parser);

  return scope.Close(interface_object);
}

/// dbus_message_size: get the size of DBusMessage struct
///   use DBusMessageIter to iterate on the message and count the 
///   size of the message
static int dbus_messages_size(DBusMessage *message) {
  DBusMessageIter iter;
  int msg_count = 0;
  
  dbus_message_iter_init(message, &iter);

  if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_ERROR) {
    const char *error_name = dbus_message_get_error_name(message);
    if (error_name != NULL) {
      std::cerr<<"Error message: "<<error_name;
    }
    return 0;
  }

  while ((dbus_message_iter_get_arg_type(&iter)) != DBUS_TYPE_INVALID) {
    msg_count++;
    //go the next
    dbus_message_iter_next (&iter);
  }

  return msg_count; 
}

/// dbus_messages_iter_size: get the size of DBusMessageIter
///   iterate the iter from the begin to end and count the size
static int dbus_messages_iter_size(DBusMessageIter *iter) {
  int msg_count = 0;
  
  while( dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_INVALID) {
    msg_count++;
    dbus_message_iter_next(iter);
  }
  return msg_count;
}


/// decode_reply_message_by_iter: decode each message to v8 Value/Object
///   check the type of "iter", and then create the v8 Value/Object 
///   according to the type
static v8::Handle<v8::Value> decode_reply_message_by_iter(
                                                DBusMessageIter *iter) {

  switch (dbus_message_iter_get_arg_type(iter)) {
    case DBUS_TYPE_BOOLEAN: {
      dbus_bool_t value = false;
      dbus_message_iter_get_basic(iter, &value);
      std::cout<<"DBUS_TYPE_BOOLEAN: "<<value<<std::endl;
      return v8::Boolean::New(value);
      break;
    }
    case DBUS_TYPE_BYTE:
    case DBUS_TYPE_INT16:
    case DBUS_TYPE_UINT16:
    case DBUS_TYPE_INT32:
    case DBUS_TYPE_UINT32:
    case DBUS_TYPE_INT64:
    case DBUS_TYPE_UINT64: {
      dbus_uint64_t value = 0; 
      dbus_message_iter_get_basic(iter, &value);
      std::cout<<"DBUS_TYPE_NUMERIC: "<< value<<std::endl;
      return v8::Integer::New(value);
      break;
    }
    case DBUS_TYPE_DOUBLE: {
      double value = 0;
      dbus_message_iter_get_basic(iter, &value);
      std::cout<<"DBUS_TYPE_DOUBLE: "<< value<<std::endl;
      return v8::Number::New(value);
      break;
    }
    case DBUS_TYPE_OBJECT_PATH:
    case DBUS_TYPE_SIGNATURE:
    case DBUS_TYPE_STRING: {
      const char *value;
      dbus_message_iter_get_basic(iter, &value); 
      std::cout<<"DBUG_TYPE_STRING: "<<value<<std::endl;
      return v8::String::New(value);
      break;
    }
    case DBUS_TYPE_ARRAY:
    case DBUS_TYPE_STRUCT: {
      std::cout<<"DBUS_TYPE_ARRAY\n";
      DBusMessageIter internal_iter, internal_temp_iter;
      int count = 0;         
     
      //count the size of the array
      dbus_message_iter_recurse(iter, &internal_temp_iter);
      count = dbus_messages_iter_size(&internal_temp_iter);
      
      //create the result object
      v8::Local<v8::Array> resultArray = v8::Array::New(count);
      count = 0;
      dbus_message_iter_recurse(iter, &internal_iter);

      do {
        std::cout<<"for each item\n";
        //this is dict entry
        if (dbus_message_iter_get_arg_type(&internal_iter) 
                      == DBUS_TYPE_DICT_ENTRY) {
          //Item is dict entry, it is exactly key-value pair
          std::cout<<"  DBUS_TYPE_DICT_ENTRY\n";
          DBusMessageIter dict_entry_iter;
          //The key 
          dbus_message_iter_recurse(&internal_iter, &dict_entry_iter);
          v8::Handle<v8::Value> key 
                        = decode_reply_message_by_iter(&dict_entry_iter);
          //The value
          dbus_message_iter_next(&dict_entry_iter);
          v8::Handle<v8::Value> value 
                        = decode_reply_message_by_iter(&dict_entry_iter);
          
          //set the property
          resultArray->Set(key, value); 
        } else {
          //Item is array
          v8::Handle<v8::Value> itemValue 
                  = decode_reply_message_by_iter(&internal_iter);
          resultArray->Set(count, itemValue);
          count++;
        }
      } while(dbus_message_iter_next(&internal_iter));
      //return the array object
      return resultArray;
    }
    case DBUS_TYPE_VARIANT: {
      std::cout<<"DBUS_TYPE_VARIANT\n";
      DBusMessageIter internal_iter;
      dbus_message_iter_recurse(iter, &internal_iter);
      
      v8::Handle<v8::Value> result 
                = decode_reply_message_by_iter(&internal_iter);
      return result;
    }
    case DBUS_TYPE_DICT_ENTRY: {
      std::cout<<"DBUS_TYPE_DICT_ENTRY"<< ":should Never be here.\n";
    }
    case DBUS_TYPE_INVALID: {
      std::cout<<"DBUS_TYPE_INVALID\n";
    } 
    default: {
      //should return 'undefined' object
      return v8::Undefined();
    }      
  } //end of swith
  //not a valid type, return undefined value
  return v8::Undefined();
}


/// decode_reply_messages: Decode the d-bus reply message to v8 Object
///  it iterate on the DBusMessage struct and wrap elements into an array
///  of v8 Object. the return type is a v8::Array object, from the js
///  viewport, the type is Array([])
static v8::Handle<v8::Value> decode_reply_messages(DBusMessage *message) {
  DBusMessageIter iter;
  int type;
  int argument_count = 0;
  int count;

  if ((count=dbus_messages_size(message)) <=0 ) {
    return v8::Undefined();
  }     

  std::cout<<"Return Message Count: "<<count<<std::endl;
  v8::Local<v8::Array> resultArray = v8::Array::New(count);
  std::cout<<"dbus_message_iter_init "<<count<<std::endl;
  dbus_message_iter_init(message, &iter);
  std::cout<<"dbus_message_get_type "<<count<<std::endl;

  //handle error
  if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_ERROR) {
    const char *error_name = dbus_message_get_error_name(message);
    if (error_name != NULL) {
      std::cerr<<"Error message: "<<error_name<<std::endl;
    }
  }

  std::cout<<"dbus_message_get_type "<<count<<std::endl; 
  while ((type=dbus_message_iter_get_arg_type(&iter)) != DBUS_TYPE_INVALID) {
    std::cerr<<"Decode message"<<std::endl;
    v8::Handle<v8::Value> valueItem = decode_reply_message_by_iter(&iter);
    resultArray->Set(argument_count, valueItem);

    //for next message
    dbus_message_iter_next (&iter);
    argument_count++;
  } //end of while loop
  
  return resultArray; 
}


static char* get_signature_from_v8_type(v8::Local<v8::Value>& value) {
  //guess the type from the v8 Object
  if (value->IsTrue() || value->IsFalse() || value->IsBoolean() ) {
    return const_cast<char*>(DBUS_TYPE_BOOLEAN_AS_STRING);
  } else if (value->IsInt32()) {
    return const_cast<char*>(DBUS_TYPE_INT32_AS_STRING);
  } else if (value->IsUint32()) {
    return const_cast<char*>(DBUS_TYPE_UINT32_AS_STRING);
  } else if (value->IsNumber()) {
    return const_cast<char*>(DBUS_TYPE_DOUBLE_AS_STRING);
  } else if (value->IsString()) {
    return const_cast<char*>(DBUS_TYPE_STRING_AS_STRING);
  } else if (value->IsArray()) {
    return const_cast<char*>(DBUS_TYPE_ARRAY_AS_STRING);
  } else {
    return NULL;
  }
}


/// Encode the given argument from JavaScript into D-Bus message
///  append the data to DBusMessage according to the type and value
static bool encode_to_message_with_objects(v8::Local<v8::Value> value, 
                                           DBusMessageIter *iter, 
                                           char* signature) {
  DBusSignatureIter siter;
  int type;
  dbus_signature_iter_init(&siter, signature);
  
  switch ((type=dbus_signature_iter_get_current_type(&siter))) {
    //the Boolean value type 
    case DBUS_TYPE_BOOLEAN:  {
      dbus_bool_t data = value->BooleanValue();  
      if (!dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN, &data)) {
        std::cerr<<"Error append boolean\n";
        return false;
      }
      break;
    }
    //the Numeric value type
    case DBUS_TYPE_INT16:
    case DBUS_TYPE_INT32:
    case DBUS_TYPE_INT64:
    case DBUS_TYPE_UINT16:
    case DBUS_TYPE_UINT32:
    case DBUS_TYPE_UINT64:
    case DBUS_TYPE_BYTE: {
      dbus_uint64_t data = value->IntegerValue();
      if (!dbus_message_iter_append_basic(iter, type, &data)) {
        std::cerr<<"Error append numeric\n";
        return false;
      }
      break; 
    }
    case DBUS_TYPE_STRING: 
    case DBUS_TYPE_OBJECT_PATH:
    case DBUS_TYPE_SIGNATURE: {
      v8::String::Utf8Value data_val(value->ToString());
      char *data = *data_val;
      if (!dbus_message_iter_append_basic(iter, type, &data)) {
        std::cerr<<"Error append string\n";
        return false;
      }
      break;
    }
    case DBUS_TYPE_DOUBLE: {
      double data = value->NumberValue();
      if (!dbus_message_iter_append_basic(iter, type, &data)) {
        std::cerr<<"Error append double\n";
        return false;
      }
      break;
    } 
    case DBUS_TYPE_ARRAY: {

      if (dbus_signature_iter_get_element_type(&siter) 
                                    == DBUS_TYPE_DICT_ENTRY) {
        //This element is a DICT type of D-Bus
        if (! value->IsObject()) {
          std::cerr<<"Error, not a Object type for DICT_ENTRY\n";
          return false;
        }
        v8::Local<v8::Object> value_object = value->ToObject();
        DBusMessageIter subIter;
        DBusSignatureIter dictSiter, dictSubSiter;
        char *dict_sig;

        dbus_signature_iter_recurse(&siter, &dictSiter);
        dict_sig = dbus_signature_iter_get_signature(&dictSiter);

        if (!dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
                                dict_sig, &subIter)) {
          std::cerr<<"Can't open container for ARRAY-Dict type\n";
          dbus_free(dict_sig); 
          return false; 
        }
        dbus_free(dict_sig);
        
        v8::Local<v8::Array> prop_names = value_object->GetPropertyNames();
        int len = prop_names->Length();

        //for each signature
        dbus_signature_iter_recurse(&dictSiter, &dictSubSiter); //the key
        dbus_signature_iter_next(&dictSubSiter); //the value
        
        bool no_error_status = true;
        for(int i=0; i<len; i++) {
          DBusMessageIter dict_iter;

          if (!dbus_message_iter_open_container(&subIter, 
                                DBUS_TYPE_DICT_ENTRY,
                                NULL, &dict_iter)) {
            std::cerr<<"  Can't open container for DICT-ENTTY\n";
            return false;
          }

          v8::Local<v8::Value> prop_name = prop_names->Get(i);
          //TODO: we currently only support 'string' type as dict key type
          //      for other type as arugment, we are currently not support
          v8::String::Utf8Value prop_name_val(prop_name->ToString());
          char *prop_str = *prop_name_val;
          //append the key
          dbus_message_iter_append_basic(&dict_iter, 
                                      DBUS_TYPE_STRING, &prop_str);
          
          //append the value 
          char *cstr = dbus_signature_iter_get_signature(&dictSubSiter);
          if ( ! encode_to_message_with_objects(
                    value_object->Get(prop_name), &dict_iter, cstr)) {
            no_error_status = false;
          }

          //release resource
          dbus_free(cstr);
          dbus_message_iter_close_container(&subIter, &dict_iter); 
          //error on encode message, break and return
          if (!no_error_status) break;
        }
        dbus_message_iter_close_container(iter, &subIter);
        //Check errors on message
        if (!no_error_status) 
          return no_error_status;
      } else {
        //This element is a Array type of D-Bus 
        if (! value->IsArray()) {
          std::cerr<<"Error!, not a Array type for array argument";
          return false;
        }
        DBusMessageIter subIter;
        DBusSignatureIter arraySIter;
        char *array_sig = NULL;
      
        dbus_signature_iter_recurse(&siter, &arraySIter);
        array_sig = dbus_signature_iter_get_signature(&arraySIter);
        std::cout<<"Array Signature: "<<array_sig<<"\n"; 
      
        if (!dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, 
                                              array_sig, &subIter)) {
          std::cerr<<"Can't open container for ARRAY type\n";
          g_free(array_sig); 
          return false; 
        }
        
        v8::Local<v8::Array> arrayData = v8::Local<v8::Array>::Cast(value);
        bool no_error_status = true;
        for (unsigned int i=0; i < arrayData->Length(); i++) {
          std::cerr<<"  Argument Arrary Item:"<<i<<"\n";
          v8::Local<v8::Value> arrayItem = arrayData->Get(i);
          if ( encode_to_message_with_objects(arrayItem, 
                                          &subIter, array_sig) ) {
            no_error_status = false;
            break;
          }
        }
        dbus_message_iter_close_container(iter, &subIter);
        g_free(array_sig);
        return no_error_status;
      }
      break;
    }
    case DBUS_TYPE_VARIANT: {
      std::cout<<"DBUS_TYPE_VARIANT\n";
      DBusMessageIter sub_iter;
      DBusSignatureIter var_siter;
       //FIXME: the variable stub
      char *var_sig = get_signature_from_v8_type(value);
      
      std::cout<<" Guess the variable type is: "<<var_sig<<"\n";

      dbus_signature_iter_recurse(&siter, &var_siter);
      
      if (!dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, 
                              var_sig, &sub_iter)) {
        std::cout<<"Can't open contianer for VARIANT type\n";
        return false;
      }
      
      //encode the object to dbus message 
      if (!encode_to_message_with_objects(value, &sub_iter, var_sig)) { 
        dbus_message_iter_close_container(iter, &sub_iter);
        return false;
      }
      dbus_message_iter_close_container(iter, &sub_iter);

      break;
    }
    case DBUS_TYPE_STRUCT: {
      std::cerr<<"DBUS_TYPE_STRUCT";
      DBusMessageIter sub_iter;
      DBusSignatureIter struct_siter;

      if (!dbus_message_iter_open_container(iter, DBUS_TYPE_STRUCT, 
                              NULL, &sub_iter)) {
        std::cerr<<"Can't open contianer for STRUCT type\n";
        return false;
      }
      
      v8::Local<v8::Object> value_object = value->ToObject();

      v8::Local<v8::Array> prop_names = value_object->GetPropertyNames();
      int len = prop_names->Length(); 
      bool no_error_status = true;

      dbus_signature_iter_recurse(&siter, &struct_siter);
      for(int i=0 ; i<len; i++) {
        char *sig = dbus_signature_iter_get_signature(&struct_siter);
        v8::Local<v8::Value> prop_name = prop_names->Get(i);

        if (!encode_to_message_with_objects(value_object->Get(prop_name), 
                                          &sub_iter, sig) ) {
          no_error_status = false;
        }
        
        dbus_free(sig);
        
        if (!dbus_signature_iter_next(&struct_siter) || !no_error_status) {
          break;
        }
      }
      dbus_message_iter_close_container(iter, &sub_iter);
      return no_error_status;
    }
    default: {
      std::cerr<<"Error! Try to append Unsupported type\n";
      return false;
    }
  }
  return true; 
}


v8::Handle<v8::Value> DBusMethod(const v8::Arguments& args){
  std::cout<<"DBueMethod Called\n";
  
  v8::HandleScope scope;
  v8::Local<v8::Value> this_data = args.Data();
  void *data = v8::External::Unwrap(this_data);

  DBusMethodContainer *container= (DBusMethodContainer*) data;
  std::cout<<"Calling method: "<<container->method<<"\n"; 
  
  bool no_error_status = true;
  DBusMessage *message = dbus_message_new_method_call (
                               container->service.c_str(), 
                               container->path.c_str(),
                               container->interface.c_str(),
                               container->method.c_str() );
  //prepare the method arguments message if needed
  if (args.Length() >0) {
    //prepare for the arguments
    const char *signature = container->signature.c_str();
    DBusMessageIter iter;
    int count = 0;
    DBusSignatureIter siter;
    DBusError error;

    dbus_message_iter_init_append(message, &iter); 

    dbus_error_init(&error);        
    if (!dbus_signature_validate(signature, &error)) {
      std::cerr<<"Invalid signature "<<error.message<<"\n";
    }
    
    dbus_signature_iter_init(&siter, signature);
    do {
      char *arg_sig = dbus_signature_iter_get_signature(&siter);
      std::cout<<"ARG: "<<arg_sig<<" Length:"<<args.Length() <<" Count:"<<count;
      //process the argument sig
      if (count >= args.Length()) {
        std::cerr<<"Arguments Not Enough\n";
        break;
      }
      
      //encode to message with given v8 Objects and the signature
      if (! encode_to_message_with_objects(args[count], &iter, arg_sig)) {
        dbus_free(arg_sig);
        no_error_status = false; 
      }

      dbus_free(arg_sig);  
      count++;
    } while (dbus_signature_iter_next(&siter));
  }
  
  //check if there is error on encode dbus message
  if (!no_error_status) {
    if (message != NULL)
      dbus_message_unref(message);
    return v8::Undefined();
  }

  //call the dbus method and get the returned message, and decode to 
  //target v8 object
  DBusMessage *reply_message;
  DBusError error;    
  v8::Handle<v8::Value> return_value = v8::Undefined();

  dbus_error_init(&error); 
  //send message and call sync dbus_method
  reply_message = dbus_connection_send_with_reply_and_block(
          dbus_g_connection_get_connection(container->connection),
          message, -1, &error);
  if (reply_message != NULL) {
    if (dbus_message_get_type(reply_message) == DBUS_MESSAGE_TYPE_ERROR) {
      std::cerr<<"Error reply message\n";

    } else if (dbus_message_get_type(reply_message) 
                  == DBUS_MESSAGE_TYPE_METHOD_RETURN) {
      std::cerr<<"Reply Message OK!\n";
      //method call return ok, decoe the messge to v8 Value 
      return_value = decode_reply_messages(reply_message);

    } else {
      std::cerr<<"Unkonwn reply\n";
    }
    //free the reply message of dbus call
    dbus_message_unref(reply_message);
  } else {
      std::cerr<<"Error calling sync method:"<<error.message<<"\n";
      dbus_error_free(&error);
  }
 
  //free the input dbus message if needed
  if (message != NULL) {
    dbus_message_unref(message);
  } 

  return return_value;
}


/// dbus_signal_filter: the static message filter of dbus messages
///   this filter receives all dbus signal messages and then find the 
///   corressponding signal handler from the global hash map, and 
///   then call the signal handler callback with the arguments from the
///   dbus message 
static DBusHandlerResult dbus_signal_filter(DBusConnection* connection,
                                            DBusMessage* message,
                                            void *user_data) {
  std::cout<<"SIGNAL FILTER"<<std::endl;
  if (message == NULL || 
        dbus_message_get_type(message) != DBUS_MESSAGE_TYPE_SIGNAL) {
    std::cout<<"Not a valid signal"<<std::endl;
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
  //get the interface name and signal name
  const char *interface_str = dbus_message_get_interface(message);
  const char *signal_name_str = dbus_message_get_member(message);
  if (interface_str == NULL || signal_name_str == NULL ) {
    std::cout<<"Not valid signal parameter"<<std::endl;
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }  

  std::string interface = dbus_message_get_interface(message);
  std::string signal_name = dbus_message_get_member(message);

  //std::cout<<"Interface"<<interface<<"  "<<signal_name;
  //get the signal matching rule
  std::string match = GetSignalMatchRuleByString(interface, signal_name);
 
  //get the signal handler object
  v8::Handle<v8::Value> value = GetSignalObjectByString(match);
  if (value == v8::Undefined()) {
    //std::cout<<"No Matching Rule";
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  } 

  //create the execution context since its in new context
  v8::Handle<v8::Context> context = v8::Context::New();
  v8::Context::Scope ctxScope(context); 
  v8::HandleScope scope;
  v8::TryCatch try_catch;

  //get the enabled property and the onemit callback
  v8::Handle<v8::Object> object = value->ToObject();
  v8::Local<v8::Value> callback_enabled 
                          = object->Get(v8::String::New("enabled"));
  v8::Local<v8::Value> callback_v 
                          = object->Get(v8::String::New("onemit"));

  
  if ( callback_enabled == v8::Undefined() 
                      || callback_v == v8::Undefined()) {
    std::cout<<"Callback undefined\n";
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
  if (! callback_enabled->ToBoolean()->Value()) {
    std::cout<<"Callback not enabled\n";
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }
  if (! callback_v->IsFunction()) {
    std::cout<<"The callback is not a Function\n";
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  }

  v8::Local<v8::Function> callback 
                          = v8::Local<v8::Function>::Cast(callback_v);

  //Decode reply message as argument
  v8::Handle<v8::Value> args[1];
  v8::Handle<v8::Value> arg0 = decode_reply_messages(message);
  args[0] = arg0; 

  //Do call the callback
  std::cout<<"To call the callback"<<std::endl;
  callback->Call(callback, 1, args);

  if (try_catch.HasCaught()) {
    std::cout<<"Ooops, Exception on call the callback"<<std::endl;
  } 

  return DBUS_HANDLER_RESULT_HANDLED;
}


v8::Handle<v8::Value> GetSignal(v8::Local<v8::Object>& interface_object,
                                         DBusGConnection *connection,
                                         const char *service_name,
                                         const char *object_path,
                                         const char *interface_name,
                                         BusSignal *signal) {
  v8::HandleScope scope;

  if (!g_is_signal_filter_attached_ ) {
    std::cout<<"attach signal filter\n";
    dbus_connection_add_filter(dbus_g_connection_get_connection(connection),
                             dbus_signal_filter, NULL, NULL);
    g_is_signal_filter_attached_ = true;
  }

  DBusError error;
  dbus_error_init (&error); 
  dbus_bus_add_match ( dbus_g_connection_get_connection(connection),
          "type='signal'",
          &error);
  if (dbus_error_is_set (&error)) {
    std::cout<<"Error Add match:"<<error.message<<"\n";
  }

  //create the object
  v8::Local<v8::ObjectTemplate> signal_obj_temp = v8::ObjectTemplate::New();
  signal_obj_temp->SetInternalFieldCount(1);
  
  v8::Persistent<v8::Object> signal_obj 
      = v8::Persistent<v8::Object>::New(signal_obj_temp->NewInstance());
  signal_obj->Set(v8::String::New("onemit"), v8::Undefined());
  signal_obj->Set(v8::String::New("enabled"), v8::Boolean::New(false));
   
  DBusSignalContainer *container = new DBusSignalContainer;
  container->service= service_name;
  container->path = object_path;
  container->interface = interface_name;
  container->signal = signal->name_; 

  AddSignalObject(container, signal_obj); 
  signal_obj->SetInternalField(0, v8::External::New(container));
  std::cout<<"Set the container object\n"; 
  //make the signal handle weak and set the callback
  signal_obj.MakeWeak(container, dbus_signal_weak_callback);


  interface_object->Set(v8::String::New(signal->name_.c_str()),
                          signal_obj);

  return v8::Undefined();
}


v8::Handle<v8::Value> GetMethod(
                          v8::Local<v8::Object>& interface_object,
                          DBusGConnection *connection,
                          const char *service_name,
                          const char *object_path,
                          const char *interface_name,
                          BusMethod *method) {
  v8::HandleScope scope;
  
  DBusMethodContainer *container = new DBusMethodContainer;
  container->connection = connection;
  container->service = service_name;
  container->path = object_path;
  container->interface = interface_name;
  container->method = method->name_;
  container->signature = method->signature_;
  

  //store the private data (container) with the FunctionTempalte
  v8::Local<v8::FunctionTemplate> func_template 
        = v8::FunctionTemplate::New(dbus_library::DBusMethod, 
                                    v8::External::New((void*)container));  
  v8::Local<v8::Function> func_obj = func_template->GetFunction();
  v8::Persistent<v8::Function> p_func_obj = v8::Persistent<v8::Function>::New(func_obj);
  
  //MakeWeak for GC
  p_func_obj.MakeWeak(container, dbus_method_weak_callback);

  interface_object->Set(v8::String::New(container->method.c_str()), 
                        p_func_obj);
  return v8::Undefined();
}

v8::Handle<v8::Value> DBusExtension::GetMethod(const v8::Arguments& args) {
  return v8::Undefined();
}

v8::Handle<v8::Value> DBusExtension::GetSignal(const v8::Arguments& args) {
  return v8::Undefined();
}

}

