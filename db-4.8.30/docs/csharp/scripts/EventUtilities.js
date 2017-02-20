
    // attach a handler to a particular event on an element
    // in a browser-independent way
    function registerEventHandler (element, event, handler) {
        if (element.attachEvent) {
            // MS registration model
            element.attachEvent('on' + event, handler);
        } else if (element.addEventListener) {
            // NN (W4C) regisration model
            element.addEventListener(event, handler, false);
        } else {
            // old regisration model as fall-back
            element[event] = handler;
        }
    }

    // get a delegate that refers to an instance method
    function getInstanceDelegate (obj, methodName) {
        return( function(e) {
            e = e || window.event;
            return obj[methodName](e);
        } );
    }
