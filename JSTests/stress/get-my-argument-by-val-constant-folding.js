function test() {
  for (var i = 0; i < testLoopCount; ++i) {
    try {
      (function () {
        return arguments[-9];
      })(42);
    } catch (e) {}
  }
}
noInline(test);

try {
  test(42);
} catch (e) {}
