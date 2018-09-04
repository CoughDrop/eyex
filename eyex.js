(function () {
    var eyex = {};
    try {
        if (process.arch == 'ia32') {
            eyex = require('eyex/eyex.32');
        } else {
            eyex = require('eyex/eyex.64');
        }
    } catch (e) { }
    module.exports = eyex;
})();