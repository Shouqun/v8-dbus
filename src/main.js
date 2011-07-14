session = dbuslib.session_bus();

//var iface = dbuslib.get_interface(session, "org.freedesktop.DBus.TestSuitePythonService", "/org/freedesktop/DBus/TestSuitePythonObject", "org.freedesktop.DB    us.TestSuiteInterface");

var iface = dbuslib.get_interface(session,"org.designfu.TestService", "/org/designfu/TestService/object", "org.designfu.TestService")

iface.HelloSignal.onemit= function(args) { Log("xxxx") }
iface.HelloSignal.enabled= true

MainLoop(session);
