#include <v8.h>

#include <string>

#include "dbus_library.h"

using namespace std;
using namespace dbus_library;

v8::Handle<v8::String> ReadFile(const string& name) 
{
  FILE* file = fopen(name.c_str(), "rb");
  if (file == NULL) return v8::Handle<v8::String>();

  fseek(file, 0, SEEK_END);
  int size = ftell(file);
  rewind(file);

  char* chars = new char[size + 1];
  chars[size] = '\0';
  for (int i = 0; i < size;) {
    int read = fread(&chars[i], 1, size - i, file);
    i += read;
  }
  fclose(file);
  v8::Handle<v8::String> result = v8::String::New(chars, size);
  delete[] chars;
  return result;
}

int main(int argc, char **argv)
{ 
  v8::HandleScope scope;

  //Load the dbus library scripts 
  v8::Persistent<v8::Context> global_context = v8::Context::New();
  v8::Context::Scope context_scope(global_context);

  v8::Local<v8::Object> global = v8::Context::GetCurrent()->Global();
  //Initialize the v8 DBus functions 
  DBusExtension::Initialize(global);

  //load the script file
  string source_file;
  source_file = "dbus_library.js";
  v8::Handle<v8::String> source = ReadFile(source_file);
  v8::Local<v8::Script> library_func = v8::Script::Compile(source); 
  v8::Local<v8::Value>  library_ret = library_func->Run();

  //load the test script file
  string test_file;
  test_file = "main.js";
  v8::Handle<v8::String> test = ReadFile(test_file);
  v8::Local<v8::Script> test_func = v8::Script::Compile(test);
  v8::Local<v8::Value> test_ret = test_func->Run();
}


