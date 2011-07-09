#include <v8.h>

#include <string>

#include "dbus_library.h"

using namespace std;
using namespace dbus_library;

int main(int argc, char **argv)
{ 
  v8::HandleScope scope;

  v8::Handle<v8::ObjectTemplate> global = ObjectTempalte::New();

  //Initialize the v8 DBus functions 
  DBusExtension::Initialize(global);

  //Load the dbus library scripts 
  v8::Persistent<v8::Context> global_context = v8::Context::New(NULL, global);
  v8::Context::Scope context_scope(global_context);
  
  //load the script file
  string source_file;
  v8::Handle<v8::String> source = ReadFile(source_file);
  v8::Local<v8::Script> library_func = v8::Script::Compile(source); 
  v8::Local<v8::Value>  library_ret = library_func->Run();

  //load the test script file
  string test_file;
  v8::Handle<v8::String> test = ReadFile(test_file);
  v8::Local<v8::Script> test_func = v8::Script::Compile(test);
  v8::Local<v8::Value> test_ret = test_func->Run();
}


