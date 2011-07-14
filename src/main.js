session = dbuslib.session_bus();

var iface = dbuslib.get_interface(session, "org.freedesktop.DBus.TestSuitePythonService", "/org/freedesktop/DBus/TestSuitePythonObject", "org.freedesktop.DBus.TestSuiteInterface");

param = {"aaa" : "bbb", "ccc": "ddd"}    
ret = iface.ParamDict(param)

dbuslib.dispose_interface(iface);

//var iface = dbuslib.get_interface(session,"org.designfu.TestService", "/org/designfu/TestService/object", "org.designfu.TestService")

//iface.HelloSignal.onemit= function(args) { Log("xxxx") }
//iface.HelloSignal.enabled= true

//MainLoop(session);
