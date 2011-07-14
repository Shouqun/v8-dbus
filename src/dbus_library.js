//var dbuslib = dbuslib || {};
//
//if (typeof(dbuslib.dbus) == 'undefined') {
//    dbuslib.dbus = {}
//}
//
//dbuslib.dbus.connect = function() {
//    native function Connect();
//    return Connect();
//}


//  expected types
//    connection = dbuslib.session_bus();  //dbuslib.system_bus();
//    proxy = dbuslib.proxy_for_name(connection, bus_name, object_path, interface);
//    ret = dbuslib.dbus_proxy_call(proxy,"Name", IN1_Type, IN1, IN2_Type, IN2, INVALID, OUT1_Type, OUT2_Type, INVALID ); 
//    the return value is a tuple, first element is the  JSON object{Type: return value; Type1: OUT1; Type2, OUT2,...}
//
//    session = dbuslib.session_bus();
//    iface = dbuslib.get_interface(session, "object_name", "obejct_path", "object_interface" );
//    iface.testMethodInterface();
//    iface.testSignal.onemit = function(args) {
//    }
//    

if (typeof(dbuslib) == 'undefined') {
    dbuslib = {};
}

(function(){

    dbuslib.session_bus = 1;
    dbuslib.system_bus = 2;


    dbuslib.session_bus  = function() {
        return SessionBus();
    }    

    dbuslib.system_bus  = function() {
        return SystemBus();
    }    
  
    dbuslib.main_loop = function(bus) {
        return MainLoop(bus);
    }
    
    /*
    dbuslib.dispose_interface = function(iface) {
        
        for (var prop in iface)
        {
            Dispose(iface[prop]);
            iface[prop] = undefined;
        }
    }
    */

    dbuslib.get_interface = function(bus, bus_name, object_path, interface_name) {
       
        var iface = GetInterface(bus, bus_name, object_path, interface_name);
      
        return iface;
    }

       
}) ();

