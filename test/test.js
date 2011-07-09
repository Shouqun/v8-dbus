session = dbuslib.session_buss();
iface = dbuslib.interface(session, "org.freedesktop.DBus", "/org/freedesktop/DBbus", "org.freedesktop.DBus");
iface.CallMethod()

var dict = {}
dict["key1"] = "value1"
dict["key2"] = "value2"
iface.CallWithDict(dict);

iface.SignalCallback.onemit = function (args) {
	console.log(args)
}

iface.SignalCallback.enabled = true;
