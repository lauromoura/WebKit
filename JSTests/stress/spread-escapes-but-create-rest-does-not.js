function assert(b) {
    if (!b)
        throw new Error;
}
noInline(assert);

function getProperties(obj) {
    let properties = [];
    for (let name of Object.getOwnPropertyNames(obj)) {
        properties.push(name);
    }
    return properties;
}

function theFunc(obj, index, ...args) {
    let functions = getProperties(obj);
    let func = functions[index % functions.length];
    obj[func](...args);
}

let o = {};
let obj = {
    valueOf: function (x, y) {
        assert(x === 42);
        assert(y === o);
        try {
        } catch (e) {}
    }
};

for (let i = 0; i < testLoopCount; ++i) {
    for (let _i = 0; _i < 100; _i++) {
    }
    theFunc(obj, 897989, 42, o);
}
