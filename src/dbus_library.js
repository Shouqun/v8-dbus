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
  
    dbuslib.get_method = function(bus, method_name) {
        var method = function() {
            //the method
            this.method_name;  
        }
        method.method_name = method_name;
        return method;
    }

    dbuslib.main_loop = function(bus) {
        return MainLoop(bus);
    }

    dbuslib.call_signal= function(bus) {
        return CallSignal(bus);
    }

    dbuslib.dispose_interface = function(iface) {
        
        for (var prop in iface)
        {
            Dispose(iface[prop]);
            iface[prop] = undefined;
        }
    }

    dbuslib.get_interface = function(bus, bus_name, object_path, interface_name) {
        var ret = {};
        
        var iface = GetInterface(bus, bus_name, object_path, interface_name);
      
        var doc = (new DOMParser()).parseFromString(iface, "text/xml");
        var ifaces = doc.getElementsByTagName("interface");
        
        for (var i=0; i<ifaces.length; i++)
        {
            iface_node = ifaces[i];
            if (iface_node.getAttribute("name") != interface_name) { continue; }
 
            var methods = iface_node.getElementsByTagName("method") 
            for (var m=0; m < methods.length; m++) 
            {
                var name = methods[m].getAttribute("name");
                var args = methods[m].getElementsByTagName("arg");
                var types = [];
                
                for (var a=0; a< args.length; a++) 
                {
                    if (args[a].getAttribute("direction") =="in")
                        types.push(args[a].getAttribute("type"));
                }
                 
                ret[name] = GetMethod(bus, bus_name, object_path, interface_name, name, types.join("")); 
             }       
            
             var signals = iface_node.getElementsByTagName("signal"); 
             for (var s=0; s <signals.length; s++)
             {
                var name = signals[s].getAttribute("name");
                
                ret[name] = GetSignal(bus, bus_name, object_path, interface_name, name);
                //ret[name].onemit = null;
             }

             ret.xml_source = (new XMLSerializer()).serializeToString(doc);
             return ret; 
        }

        return null;
    }

       
}) ();

