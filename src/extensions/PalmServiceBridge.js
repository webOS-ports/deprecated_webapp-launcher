/* PalmServiceBridge */

var __PalmSericeBridgeInstanceCounter = 0;

function PalmServiceBridge() {
    this.onservicecallback = function(msg) { };

    // As we're creating a class here we need to manage mutiple instances on
    // both sites.
    this.instanceId = ++__PalmSericeBridgeInstanceCounter;
    _webOS.exec(unusedCallback, unusedCallback, "PalmServiceBridge", "createInstance", [this.instanceId]);
}

PalmServiceBridge.prototype.version = function() {
    return "1.1";
}

PalmServiceBridge.prototype.destroy = function() {
    _webOS.exec(unusedCallback, unusedCallback, "PalmServiceBridge", "releaseInstance", [this.instanceId]);
}

PalmServiceBridge.prototype.call = function(method, url) {
    var oncomplete = this.onservicecallback;
    function callback(msg) {
        oncomplete(msg);
    }

    _webOS.exec(callback, callback, "PalmServiceBridge", "call", [this.instanceId, method, url]);
}

PalmServiceBridge.prototype.cancel = function() {
    _webOS.exec(unusedCallback, unusedCallback, "PalmServiceBridge", "cancel", [this.instanceId]);
}
