function foo(a, b) {
    return a & b;
}

noInline(foo);

var things = [1, 2.5, "3", {valueOf: function() { return 4; }}];
var results = [0, 2, 2, 0];

for (var i = 0; i < testLoopCount; ++i) {
    var result = foo(things[i % things.length], 2);
    var expected = results[i % results.length];
    if (result != expected)
        throw "Error: bad result for i = " + i + ": " + result;
}

