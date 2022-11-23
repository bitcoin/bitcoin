/*!
 * Benchmark.js <https://benchmarkjs.com/>
 * Copyright 2010-2016 Mathias Bynens <https://mths.be/>
 * Based on JSLitmus.js, copyright Robert Kieffer <http://broofa.com/>
 * Modified by John-David Dalton <http://allyoucanleet.com/>
 * Available under MIT license <https://mths.be/mit>
 */
;(function() {
  'use strict';

  /** Used as a safe reference for `undefined` in pre ES5 environments. */
  var undefined;

  /** Used to determine if values are of the language type Object. */
  var objectTypes = {
    'function': true,
    'object': true
  };

  /** Used as a reference to the global object. */
  var root = (objectTypes[typeof window] && window) || this;

  /** Detect free variable `define`. */
  var freeDefine = typeof define == 'function' && typeof define.amd == 'object' && define.amd && define;

  /** Detect free variable `exports`. */
  var freeExports = objectTypes[typeof exports] && exports && !exports.nodeType && exports;

  /** Detect free variable `module`. */
  var freeModule = objectTypes[typeof module] && module && !module.nodeType && module;

  /** Detect free variable `global` from Node.js or Browserified code and use it as `root`. */
  var freeGlobal = freeExports && freeModule && typeof global == 'object' && global;
  if (freeGlobal && (freeGlobal.global === freeGlobal || freeGlobal.window === freeGlobal || freeGlobal.self === freeGlobal)) {
    root = freeGlobal;
  }

  /** Detect free variable `require`. */
  var freeRequire = typeof require == 'function' && require;

  /** Used to assign each benchmark an incremented id. */
  var counter = 0;

  /** Detect the popular CommonJS extension `module.exports`. */
  var moduleExports = freeModule && freeModule.exports === freeExports && freeExports;

  /** Used to detect primitive types. */
  var rePrimitive = /^(?:boolean|number|string|undefined)$/;

  /** Used to make every compiled test unique. */
  var uidCounter = 0;

  /** Used to assign default `context` object properties. */
  var contextProps = [
    'Array', 'Date', 'Function', 'Math', 'Object', 'RegExp', 'String', '_',
    'clearTimeout', 'chrome', 'chromium', 'document', 'navigator', 'phantom',
    'platform', 'process', 'runtime', 'setTimeout'
  ];

  /** Used to avoid hz of Infinity. */
  var divisors = {
    '1': 4096,
    '2': 512,
    '3': 64,
    '4': 8,
    '5': 0
  };

  /**
   * T-Distribution two-tailed critical values for 95% confidence.
   * For more info see http://www.itl.nist.gov/div898/handbook/eda/section3/eda3672.htm.
   */
  var tTable = {
    '1':  12.706, '2':  4.303, '3':  3.182, '4':  2.776, '5':  2.571, '6':  2.447,
    '7':  2.365,  '8':  2.306, '9':  2.262, '10': 2.228, '11': 2.201, '12': 2.179,
    '13': 2.16,   '14': 2.145, '15': 2.131, '16': 2.12,  '17': 2.11,  '18': 2.101,
    '19': 2.093,  '20': 2.086, '21': 2.08,  '22': 2.074, '23': 2.069, '24': 2.064,
    '25': 2.06,   '26': 2.056, '27': 2.052, '28': 2.048, '29': 2.045, '30': 2.042,
    'infinity': 1.96
  };

  /**
   * Critical Mann-Whitney U-values for 95% confidence.
   * For more info see http://www.saburchill.com/IBbiology/stats/003.html.
   */
  var uTable = {
    '5':  [0, 1, 2],
    '6':  [1, 2, 3, 5],
    '7':  [1, 3, 5, 6, 8],
    '8':  [2, 4, 6, 8, 10, 13],
    '9':  [2, 4, 7, 10, 12, 15, 17],
    '10': [3, 5, 8, 11, 14, 17, 20, 23],
    '11': [3, 6, 9, 13, 16, 19, 23, 26, 30],
    '12': [4, 7, 11, 14, 18, 22, 26, 29, 33, 37],
    '13': [4, 8, 12, 16, 20, 24, 28, 33, 37, 41, 45],
    '14': [5, 9, 13, 17, 22, 26, 31, 36, 40, 45, 50, 55],
    '15': [5, 10, 14, 19, 24, 29, 34, 39, 44, 49, 54, 59, 64],
    '16': [6, 11, 15, 21, 26, 31, 37, 42, 47, 53, 59, 64, 70, 75],
    '17': [6, 11, 17, 22, 28, 34, 39, 45, 51, 57, 63, 67, 75, 81, 87],
    '18': [7, 12, 18, 24, 30, 36, 42, 48, 55, 61, 67, 74, 80, 86, 93, 99],
    '19': [7, 13, 19, 25, 32, 38, 45, 52, 58, 65, 72, 78, 85, 92, 99, 106, 113],
    '20': [8, 14, 20, 27, 34, 41, 48, 55, 62, 69, 76, 83, 90, 98, 105, 112, 119, 127],
    '21': [8, 15, 22, 29, 36, 43, 50, 58, 65, 73, 80, 88, 96, 103, 111, 119, 126, 134, 142],
    '22': [9, 16, 23, 30, 38, 45, 53, 61, 69, 77, 85, 93, 101, 109, 117, 125, 133, 141, 150, 158],
    '23': [9, 17, 24, 32, 40, 48, 56, 64, 73, 81, 89, 98, 106, 115, 123, 132, 140, 149, 157, 166, 175],
    '24': [10, 17, 25, 33, 42, 50, 59, 67, 76, 85, 94, 102, 111, 120, 129, 138, 147, 156, 165, 174, 183, 192],
    '25': [10, 18, 27, 35, 44, 53, 62, 71, 80, 89, 98, 107, 117, 126, 135, 145, 154, 163, 173, 182, 192, 201, 211],
    '26': [11, 19, 28, 37, 46, 55, 64, 74, 83, 93, 102, 112, 122, 132, 141, 151, 161, 171, 181, 191, 200, 210, 220, 230],
    '27': [11, 20, 29, 38, 48, 57, 67, 77, 87, 97, 107, 118, 125, 138, 147, 158, 168, 178, 188, 199, 209, 219, 230, 240, 250],
    '28': [12, 21, 30, 40, 50, 60, 70, 80, 90, 101, 111, 122, 132, 143, 154, 164, 175, 186, 196, 207, 218, 228, 239, 250, 261, 272],
    '29': [13, 22, 32, 42, 52, 62, 73, 83, 94, 105, 116, 127, 138, 149, 160, 171, 182, 193, 204, 215, 226, 238, 249, 260, 271, 282, 294],
    '30': [13, 23, 33, 43, 54, 65, 76, 87, 98, 109, 120, 131, 143, 154, 166, 177, 189, 200, 212, 223, 235, 247, 258, 270, 282, 293, 305, 317]
  };

  /*--------------------------------------------------------------------------*/

  /**
   * Create a new `Benchmark` function using the given `context` object.
   *
   * @static
   * @memberOf Benchmark
   * @param {Object} [context=root] The context object.
   * @returns {Function} Returns a new `Benchmark` function.
   */
  function runInContext(context) {
    // Exit early if unable to acquire lodash.
    var _ = context && context._ || require('lodash') || root._;
    if (!_) {
      Benchmark.runInContext = runInContext;
      return Benchmark;
    }
    // Avoid issues with some ES3 environments that attempt to use values, named
    // after built-in constructors like `Object`, for the creation of literals.
    // ES5 clears this up by stating that literals must use built-in constructors.
    // See http://es5.github.io/#x11.1.5.
    context = context ? _.defaults(root.Object(), context, _.pick(root, contextProps)) : root;

    /** Native constructor references. */
    var Array = context.Array,
        Date = context.Date,
        Function = context.Function,
        Math = context.Math,
        Object = context.Object,
        RegExp = context.RegExp,
        String = context.String;

    /** Used for `Array` and `Object` method references. */
    var arrayRef = [],
        objectProto = Object.prototype;

    /** Native method shortcuts. */
    var abs = Math.abs,
        clearTimeout = context.clearTimeout,
        floor = Math.floor,
        log = Math.log,
        max = Math.max,
        min = Math.min,
        pow = Math.pow,
        push = arrayRef.push,
        setTimeout = context.setTimeout,
        shift = arrayRef.shift,
        slice = arrayRef.slice,
        sqrt = Math.sqrt,
        toString = objectProto.toString,
        unshift = arrayRef.unshift;

    /** Used to avoid inclusion in Browserified bundles. */
    var req = require;

    /** Detect DOM document object. */
    var doc = isHostType(context, 'document') && context.document;

    /** Used to access Wade Simmons' Node.js `microtime` module. */
    var microtimeObject = req('microtime');

    /** Used to access Node.js's high resolution timer. */
    var processObject = isHostType(context, 'process') && context.process;

    /** Used to prevent a `removeChild` memory leak in IE < 9. */
    var trash = doc && doc.createElement('div');

    /** Used to integrity check compiled tests. */
    var uid = 'uid' + _.now();

    /** Used to avoid infinite recursion when methods call each other. */
    var calledBy = {};

    /**
     * An object used to flag environments/features.
     *
     * @static
     * @memberOf Benchmark
     * @type Object
     */
    var support = {};

    (function() {

      /**
       * Detect if running in a browser environment.
       *
       * @memberOf Benchmark.support
       * @type boolean
       */
      support.browser = doc && isHostType(context, 'navigator') && !isHostType(context, 'phantom');

      /**
       * Detect if the Timers API exists.
       *
       * @memberOf Benchmark.support
       * @type boolean
       */
      support.timeout = isHostType(context, 'setTimeout') && isHostType(context, 'clearTimeout');

      /**
       * Detect if function decompilation is support.
       *
       * @name decompilation
       * @memberOf Benchmark.support
       * @type boolean
       */
      try {
        // Safari 2.x removes commas in object literals from `Function#toString` results.
        // See http://webk.it/11609 for more details.
        // Firefox 3.6 and Opera 9.25 strip grouping parentheses from `Function#toString` results.
        // See http://bugzil.la/559438 for more details.
        support.decompilation = Function(
          ('return (' + (function(x) { return { 'x': '' + (1 + x) + '', 'y': 0 }; }) + ')')
          // Avoid issues with code added by Istanbul.
          .replace(/__cov__[^;]+;/g, '')
        )()(0).x === '1';
      } catch(e) {
        support.decompilation = false;
      }
    }());

    /**
     * Timer object used by `clock()` and `Deferred#resolve`.
     *
     * @private
     * @type Object
     */
    var timer = {

      /**
       * The timer namespace object or constructor.
       *
       * @private
       * @memberOf timer
       * @type {Function|Object}
       */
      'ns': Date,

      /**
       * Starts the deferred timer.
       *
       * @private
       * @memberOf timer
       * @param {Object} deferred The deferred instance.
       */
      'start': null, // Lazy defined in `clock()`.

      /**
       * Stops the deferred timer.
       *
       * @private
       * @memberOf timer
       * @param {Object} deferred The deferred instance.
       */
      'stop': null // Lazy defined in `clock()`.
    };

    /*------------------------------------------------------------------------*/

    /**
     * The Benchmark constructor.
     *
     * Note: The Benchmark constructor exposes a handful of lodash methods to
     * make working with arrays, collections, and objects easier. The lodash
     * methods are:
     * [`each/forEach`](https://lodash.com/docs#forEach), [`forOwn`](https://lodash.com/docs#forOwn),
     * [`has`](https://lodash.com/docs#has), [`indexOf`](https://lodash.com/docs#indexOf),
     * [`map`](https://lodash.com/docs#map), and [`reduce`](https://lodash.com/docs#reduce)
     *
     * @constructor
     * @param {string} name A name to identify the benchmark.
     * @param {Function|string} fn The test to benchmark.
     * @param {Object} [options={}] Options object.
     * @example
     *
     * // basic usage (the `new` operator is optional)
     * var bench = new Benchmark(fn);
     *
     * // or using a name first
     * var bench = new Benchmark('foo', fn);
     *
     * // or with options
     * var bench = new Benchmark('foo', fn, {
     *
     *   // displayed by `Benchmark#toString` if `name` is not available
     *   'id': 'xyz',
     *
     *   // called when the benchmark starts running
     *   'onStart': onStart,
     *
     *   // called after each run cycle
     *   'onCycle': onCycle,
     *
     *   // called when aborted
     *   'onAbort': onAbort,
     *
     *   // called when a test errors
     *   'onError': onError,
     *
     *   // called when reset
     *   'onReset': onReset,
     *
     *   // called when the benchmark completes running
     *   'onComplete': onComplete,
     *
     *   // compiled/called before the test loop
     *   'setup': setup,
     *
     *   // compiled/called after the test loop
     *   'teardown': teardown
     * });
     *
     * // or name and options
     * var bench = new Benchmark('foo', {
     *
     *   // a flag to indicate the benchmark is deferred
     *   'defer': true,
     *
     *   // benchmark test function
     *   'fn': function(deferred) {
     *     // call `Deferred#resolve` when the deferred test is finished
     *     deferred.resolve();
     *   }
     * });
     *
     * // or options only
     * var bench = new Benchmark({
     *
     *   // benchmark name
     *   'name': 'foo',
     *
     *   // benchmark test as a string
     *   'fn': '[1,2,3,4].sort()'
     * });
     *
     * // a test's `this` binding is set to the benchmark instance
     * var bench = new Benchmark('foo', function() {
     *   'My name is '.concat(this.name); // "My name is foo"
     * });
     */
    function Benchmark(name, fn, options) {
      var bench = this;

      // Allow instance creation without the `new` operator.
      if (!(bench instanceof Benchmark)) {
        return new Benchmark(name, fn, options);
      }
      // Juggle arguments.
      if (_.isPlainObject(name)) {
        // 1 argument (options).
        options = name;
      }
      else if (_.isFunction(name)) {
        // 2 arguments (fn, options).
        options = fn;
        fn = name;
      }
      else if (_.isPlainObject(fn)) {
        // 2 arguments (name, options).
        options = fn;
        fn = null;
        bench.name = name;
      }
      else {
        // 3 arguments (name, fn [, options]).
        bench.name = name;
      }
      setOptions(bench, options);

      bench.id || (bench.id = ++counter);
      bench.fn == null && (bench.fn = fn);

      bench.stats = cloneDeep(bench.stats);
      bench.times = cloneDeep(bench.times);
    }

    /**
     * The Deferred constructor.
     *
     * @constructor
     * @memberOf Benchmark
     * @param {Object} clone The cloned benchmark instance.
     */
    function Deferred(clone) {
      var deferred = this;
      if (!(deferred instanceof Deferred)) {
        return new Deferred(clone);
      }
      deferred.benchmark = clone;
      clock(deferred);
    }

    /**
     * The Event constructor.
     *
     * @constructor
     * @memberOf Benchmark
     * @param {Object|string} type The event type.
     */
    function Event(type) {
      var event = this;
      if (type instanceof Event) {
        return type;
      }
      return (event instanceof Event)
        ? _.assign(event, { 'timeStamp': _.now() }, typeof type == 'string' ? { 'type': type } : type)
        : new Event(type);
    }

    /**
     * The Suite constructor.
     *
     * Note: Each Suite instance has a handful of wrapped lodash methods to
     * make working with Suites easier. The wrapped lodash methods are:
     * [`each/forEach`](https://lodash.com/docs#forEach), [`indexOf`](https://lodash.com/docs#indexOf),
     * [`map`](https://lodash.com/docs#map), and [`reduce`](https://lodash.com/docs#reduce)
     *
     * @constructor
     * @memberOf Benchmark
     * @param {string} name A name to identify the suite.
     * @param {Object} [options={}] Options object.
     * @example
     *
     * // basic usage (the `new` operator is optional)
     * var suite = new Benchmark.Suite;
     *
     * // or using a name first
     * var suite = new Benchmark.Suite('foo');
     *
     * // or with options
     * var suite = new Benchmark.Suite('foo', {
     *
     *   // called when the suite starts running
     *   'onStart': onStart,
     *
     *   // called between running benchmarks
     *   'onCycle': onCycle,
     *
     *   // called when aborted
     *   'onAbort': onAbort,
     *
     *   // called when a test errors
     *   'onError': onError,
     *
     *   // called when reset
     *   'onReset': onReset,
     *
     *   // called when the suite completes running
     *   'onComplete': onComplete
     * });
     */
    function Suite(name, options) {
      var suite = this;

      // Allow instance creation without the `new` operator.
      if (!(suite instanceof Suite)) {
        return new Suite(name, options);
      }
      // Juggle arguments.
      if (_.isPlainObject(name)) {
        // 1 argument (options).
        options = name;
      } else {
        // 2 arguments (name [, options]).
        suite.name = name;
      }
      setOptions(suite, options);
    }

    /*------------------------------------------------------------------------*/

    /**
     * A specialized version of `_.cloneDeep` which only clones arrays and plain
     * objects assigning all other values by reference.
     *
     * @private
     * @param {*} value The value to clone.
     * @returns {*} The cloned value.
     */
    var cloneDeep = _.partial(_.cloneDeepWith, _, function(value) {
      // Only clone primitives, arrays, and plain objects.
      return (_.isObject(value) && !_.isArray(value) && !_.isPlainObject(value))
        ? value
        : undefined;
    });

    /**
     * Creates a function from the given arguments string and body.
     *
     * @private
     * @param {string} args The comma separated function arguments.
     * @param {string} body The function body.
     * @returns {Function} The new function.
     */
    function createFunction() {
      // Lazy define.
      createFunction = function(args, body) {
        var result,
            anchor = freeDefine ? freeDefine.amd : Benchmark,
            prop = uid + 'createFunction';

        runScript((freeDefine ? 'define.amd.' : 'Benchmark.') + prop + '=function(' + args + '){' + body + '}');
        result = anchor[prop];
        delete anchor[prop];
        return result;
      };
      // Fix JaegerMonkey bug.
      // For more information see http://bugzil.la/639720.
      createFunction = support.browser && (createFunction('', 'return"' + uid + '"') || _.noop)() == uid ? createFunction : Function;
      return createFunction.apply(null, arguments);
    }

    /**
     * Delay the execution of a function based on the benchmark's `delay` property.
     *
     * @private
     * @param {Object} bench The benchmark instance.
     * @param {Object} fn The function to execute.
     */
    function delay(bench, fn) {
      bench._timerId = _.delay(fn, bench.delay * 1e3);
    }

    /**
     * Destroys the given element.
     *
     * @private
     * @param {Element} element The element to destroy.
     */
    function destroyElement(element) {
      trash.appendChild(element);
      trash.innerHTML = '';
    }

    /**
     * Gets the name of the first argument from a function's source.
     *
     * @private
     * @param {Function} fn The function.
     * @returns {string} The argument name.
     */
    function getFirstArgument(fn) {
      return (!_.has(fn, 'toString') &&
        (/^[\s(]*function[^(]*\(([^\s,)]+)/.exec(fn) || 0)[1]) || '';
    }

    /**
     * Computes the arithmetic mean of a sample.
     *
     * @private
     * @param {Array} sample The sample.
     * @returns {number} The mean.
     */
    function getMean(sample) {
      return (_.reduce(sample, function(sum, x) {
        return sum + x;
      }) / sample.length) || 0;
    }

    /**
     * Gets the source code of a function.
     *
     * @private
     * @param {Function} fn The function.
     * @returns {string} The function's source code.
     */
    function getSource(fn) {
      var result = '';
      if (isStringable(fn)) {
        result = String(fn);
      } else if (support.decompilation) {
        // Escape the `{` for Firefox 1.
        result = _.result(/^[^{]+\{([\s\S]*)\}\s*$/.exec(fn), 1);
      }
      // Trim string.
      result = (result || '').replace(/^\s+|\s+$/g, '');

      // Detect strings containing only the "use strict" directive.
      return /^(?:\/\*+[\w\W]*?\*\/|\/\/.*?[\n\r\u2028\u2029]|\s)*(["'])use strict\1;?$/.test(result)
        ? ''
        : result;
    }

    /**
     * Checks if an object is of the specified class.
     *
     * @private
     * @param {*} value The value to check.
     * @param {string} name The name of the class.
     * @returns {boolean} Returns `true` if the value is of the specified class, else `false`.
     */
    function isClassOf(value, name) {
      return value != null && toString.call(value) == '[object ' + name + ']';
    }

    /**
     * Host objects can return type values that are different from their actual
     * data type. The objects we are concerned with usually return non-primitive
     * types of "object", "function", or "unknown".
     *
     * @private
     * @param {*} object The owner of the property.
     * @param {string} property The property to check.
     * @returns {boolean} Returns `true` if the property value is a non-primitive, else `false`.
     */
    function isHostType(object, property) {
      if (object == null) {
        return false;
      }
      var type = typeof object[property];
      return !rePrimitive.test(type) && (type != 'object' || !!object[property]);
    }

    /**
     * Checks if a value can be safely coerced to a string.
     *
     * @private
     * @param {*} value The value to check.
     * @returns {boolean} Returns `true` if the value can be coerced, else `false`.
     */
    function isStringable(value) {
      return _.isString(value) || (_.has(value, 'toString') && _.isFunction(value.toString));
    }

    /**
     * A wrapper around `require` to suppress `module missing` errors.
     *
     * @private
     * @param {string} id The module id.
     * @returns {*} The exported module or `null`.
     */
    function require(id) {
      try {
        var result = freeExports && freeRequire(id);
      } catch(e) {}
      return result || null;
    }

    /**
     * Runs a snippet of JavaScript via script injection.
     *
     * @private
     * @param {string} code The code to run.
     */
    function runScript(code) {
      var anchor = freeDefine ? define.amd : Benchmark,
          script = doc.createElement('script'),
          sibling = doc.getElementsByTagName('script')[0],
          parent = sibling.parentNode,
          prop = uid + 'runScript',
          prefix = '(' + (freeDefine ? 'define.amd.' : 'Benchmark.') + prop + '||function(){})();';

      // Firefox 2.0.0.2 cannot use script injection as intended because it executes
      // asynchronously, but that's OK because script injection is only used to avoid
      // the previously commented JaegerMonkey bug.
      try {
        // Remove the inserted script *before* running the code to avoid differences
        // in the expected script element count/order of the document.
        script.appendChild(doc.createTextNode(prefix + code));
        anchor[prop] = function() { destroyElement(script); };
      } catch(e) {
        parent = parent.cloneNode(false);
        sibling = null;
        script.text = code;
      }
      parent.insertBefore(script, sibling);
      delete anchor[prop];
    }

    /**
     * A helper function for setting options/event handlers.
     *
     * @private
     * @param {Object} object The benchmark or suite instance.
     * @param {Object} [options={}] Options object.
     */
    function setOptions(object, options) {
      options = object.options = _.assign({}, cloneDeep(object.constructor.options), cloneDeep(options));

      _.forOwn(options, function(value, key) {
        if (value != null) {
          // Add event listeners.
          if (/^on[A-Z]/.test(key)) {
            _.each(key.split(' '), function(key) {
              object.on(key.slice(2).toLowerCase(), value);
            });
          } else if (!_.has(object, key)) {
            object[key] = cloneDeep(value);
          }
        }
      });
    }

    /*------------------------------------------------------------------------*/

    /**
     * Handles cycling/completing the deferred benchmark.
     *
     * @memberOf Benchmark.Deferred
     */
    function resolve() {
      var deferred = this,
          clone = deferred.benchmark,
          bench = clone._original;

      if (bench.aborted) {
        // cycle() -> clone cycle/complete event -> compute()'s invoked bench.run() cycle/complete.
        deferred.teardown();
        clone.running = false;
        cycle(deferred);
      }
      else if (++deferred.cycles < clone.count) {
        clone.compiled.call(deferred, context, timer);
      }
      else {
        timer.stop(deferred);
        deferred.teardown();
        delay(clone, function() { cycle(deferred); });
      }
    }

    /*------------------------------------------------------------------------*/

    /**
     * A generic `Array#filter` like method.
     *
     * @static
     * @memberOf Benchmark
     * @param {Array} array The array to iterate over.
     * @param {Function|string} callback The function/alias called per iteration.
     * @returns {Array} A new array of values that passed callback filter.
     * @example
     *
     * // get odd numbers
     * Benchmark.filter([1, 2, 3, 4, 5], function(n) {
     *   return n % 2;
     * }); // -> [1, 3, 5];
     *
     * // get fastest benchmarks
     * Benchmark.filter(benches, 'fastest');
     *
     * // get slowest benchmarks
     * Benchmark.filter(benches, 'slowest');
     *
     * // get benchmarks that completed without erroring
     * Benchmark.filter(benches, 'successful');
     */
    function filter(array, callback) {
      if (callback === 'successful') {
        // Callback to exclude those that are errored, unrun, or have hz of Infinity.
        callback = function(bench) {
          return bench.cycles && _.isFinite(bench.hz) && !bench.error;
        };
      }
      else if (callback === 'fastest' || callback === 'slowest') {
        // Get successful, sort by period + margin of error, and filter fastest/slowest.
        var result = filter(array, 'successful').sort(function(a, b) {
          a = a.stats; b = b.stats;
          return (a.mean + a.moe > b.mean + b.moe ? 1 : -1) * (callback === 'fastest' ? 1 : -1);
        });

        return _.filter(result, function(bench) {
          return result[0].compare(bench) == 0;
        });
      }
      return _.filter(array, callback);
    }

    /**
     * Converts a number to a more readable comma-separated string representation.
     *
     * @static
     * @memberOf Benchmark
     * @param {number} number The number to convert.
     * @returns {string} The more readable string representation.
     */
    function formatNumber(number) {
      number = String(number).split('.');
      return number[0].replace(/(?=(?:\d{3})+$)(?!\b)/g, ',') +
        (number[1] ? '.' + number[1] : '');
    }

    /**
     * Invokes a method on all items in an array.
     *
     * @static
     * @memberOf Benchmark
     * @param {Array} benches Array of benchmarks to iterate over.
     * @param {Object|string} name The name of the method to invoke OR options object.
     * @param {...*} [args] Arguments to invoke the method with.
     * @returns {Array} A new array of values returned from each method invoked.
     * @example
     *
     * // invoke `reset` on all benchmarks
     * Benchmark.invoke(benches, 'reset');
     *
     * // invoke `emit` with arguments
     * Benchmark.invoke(benches, 'emit', 'complete', listener);
     *
     * // invoke `run(true)`, treat benchmarks as a queue, and register invoke callbacks
     * Benchmark.invoke(benches, {
     *
     *   // invoke the `run` method
     *   'name': 'run',
     *
     *   // pass a single argument
     *   'args': true,
     *
     *   // treat as queue, removing benchmarks from front of `benches` until empty
     *   'queued': true,
     *
     *   // called before any benchmarks have been invoked.
     *   'onStart': onStart,
     *
     *   // called between invoking benchmarks
     *   'onCycle': onCycle,
     *
     *   // called after all benchmarks have been invoked.
     *   'onComplete': onComplete
     * });
     */
    function invoke(benches, name) {
      var args,
          bench,
          queued,
          index = -1,
          eventProps = { 'currentTarget': benches },
          options = { 'onStart': _.noop, 'onCycle': _.noop, 'onComplete': _.noop },
          result = _.toArray(benches);

      /**
       * Invokes the method of the current object and if synchronous, fetches the next.
       */
      function execute() {
        var listeners,
            async = isAsync(bench);

        if (async) {
          // Use `getNext` as the first listener.
          bench.on('complete', getNext);
          listeners = bench.events.complete;
          listeners.splice(0, 0, listeners.pop());
        }
        // Execute method.
        result[index] = _.isFunction(bench && bench[name]) ? bench[name].apply(bench, args) : undefined;
        // If synchronous return `true` until finished.
        return !async && getNext();
      }

      /**
       * Fetches the next bench or executes `onComplete` callback.
       */
      function getNext(event) {
        var cycleEvent,
            last = bench,
            async = isAsync(last);

        if (async) {
          last.off('complete', getNext);
          last.emit('complete');
        }
        // Emit "cycle" event.
        eventProps.type = 'cycle';
        eventProps.target = last;
        cycleEvent = Event(eventProps);
        options.onCycle.call(benches, cycleEvent);

        // Choose next benchmark if not exiting early.
        if (!cycleEvent.aborted && raiseIndex() !== false) {
          bench = queued ? benches[0] : result[index];
          if (isAsync(bench)) {
            delay(bench, execute);
          }
          else if (async) {
            // Resume execution if previously asynchronous but now synchronous.
            while (execute()) {}
          }
          else {
            // Continue synchronous execution.
            return true;
          }
        } else {
          // Emit "complete" event.
          eventProps.type = 'complete';
          options.onComplete.call(benches, Event(eventProps));
        }
        // When used as a listener `event.aborted = true` will cancel the rest of
        // the "complete" listeners because they were already called above and when
        // used as part of `getNext` the `return false` will exit the execution while-loop.
        if (event) {
          event.aborted = true;
        } else {
          return false;
        }
      }

      /**
       * Checks if invoking `Benchmark#run` with asynchronous cycles.
       */
      function isAsync(object) {
        // Avoid using `instanceof` here because of IE memory leak issues with host objects.
        var async = args[0] && args[0].async;
        return name == 'run' && (object instanceof Benchmark) &&
          ((async == null ? object.options.async : async) && support.timeout || object.defer);
      }

      /**
       * Raises `index` to the next defined index or returns `false`.
       */
      function raiseIndex() {
        index++;

        // If queued remove the previous bench.
        if (queued && index > 0) {
          shift.call(benches);
        }
        // If we reached the last index then return `false`.
        return (queued ? benches.length : index < result.length)
          ? index
          : (index = false);
      }
      // Juggle arguments.
      if (_.isString(name)) {
        // 2 arguments (array, name).
        args = slice.call(arguments, 2);
      } else {
        // 2 arguments (array, options).
        options = _.assign(options, name);
        name = options.name;
        args = _.isArray(args = 'args' in options ? options.args : []) ? args : [args];
        queued = options.queued;
      }
      // Start iterating over the array.
      if (raiseIndex() !== false) {
        // Emit "start" event.
        bench = result[index];
        eventProps.type = 'start';
        eventProps.target = bench;
        options.onStart.call(benches, Event(eventProps));

        // End early if the suite was aborted in an "onStart" listener.
        if (name == 'run' && (benches instanceof Suite) && benches.aborted) {
          // Emit "cycle" event.
          eventProps.type = 'cycle';
          options.onCycle.call(benches, Event(eventProps));
          // Emit "complete" event.
          eventProps.type = 'complete';
          options.onComplete.call(benches, Event(eventProps));
        }
        // Start method execution.
        else {
          if (isAsync(bench)) {
            delay(bench, execute);
          } else {
            while (execute()) {}
          }
        }
      }
      return result;
    }

    /**
     * Creates a string of joined array values or object key-value pairs.
     *
     * @static
     * @memberOf Benchmark
     * @param {Array|Object} object The object to operate on.
     * @param {string} [separator1=','] The separator used between key-value pairs.
     * @param {string} [separator2=': '] The separator used between keys and values.
     * @returns {string} The joined result.
     */
    function join(object, separator1, separator2) {
      var result = [],
          length = (object = Object(object)).length,
          arrayLike = length === length >>> 0;

      separator2 || (separator2 = ': ');
      _.each(object, function(value, key) {
        result.push(arrayLike ? value : key + separator2 + value);
      });
      return result.join(separator1 || ',');
    }

    /*------------------------------------------------------------------------*/

    /**
     * Aborts all benchmarks in the suite.
     *
     * @name abort
     * @memberOf Benchmark.Suite
     * @returns {Object} The suite instance.
     */
    function abortSuite() {
      var event,
          suite = this,
          resetting = calledBy.resetSuite;

      if (suite.running) {
        event = Event('abort');
        suite.emit(event);
        if (!event.cancelled || resetting) {
          // Avoid infinite recursion.
          calledBy.abortSuite = true;
          suite.reset();
          delete calledBy.abortSuite;

          if (!resetting) {
            suite.aborted = true;
            invoke(suite, 'abort');
          }
        }
      }
      return suite;
    }

    /**
     * Adds a test to the benchmark suite.
     *
     * @memberOf Benchmark.Suite
     * @param {string} name A name to identify the benchmark.
     * @param {Function|string} fn The test to benchmark.
     * @param {Object} [options={}] Options object.
     * @returns {Object} The suite instance.
     * @example
     *
     * // basic usage
     * suite.add(fn);
     *
     * // or using a name first
     * suite.add('foo', fn);
     *
     * // or with options
     * suite.add('foo', fn, {
     *   'onCycle': onCycle,
     *   'onComplete': onComplete
     * });
     *
     * // or name and options
     * suite.add('foo', {
     *   'fn': fn,
     *   'onCycle': onCycle,
     *   'onComplete': onComplete
     * });
     *
     * // or options only
     * suite.add({
     *   'name': 'foo',
     *   'fn': fn,
     *   'onCycle': onCycle,
     *   'onComplete': onComplete
     * });
     */
    function add(name, fn, options) {
      var suite = this,
          bench = new Benchmark(name, fn, options),
          event = Event({ 'type': 'add', 'target': bench });

      if (suite.emit(event), !event.cancelled) {
        suite.push(bench);
      }
      return suite;
    }

    /**
     * Creates a new suite with cloned benchmarks.
     *
     * @name clone
     * @memberOf Benchmark.Suite
     * @param {Object} options Options object to overwrite cloned options.
     * @returns {Object} The new suite instance.
     */
    function cloneSuite(options) {
      var suite = this,
          result = new suite.constructor(_.assign({}, suite.options, options));

      // Copy own properties.
      _.forOwn(suite, function(value, key) {
        if (!_.has(result, key)) {
          result[key] = value && _.isFunction(value.clone)
            ? value.clone()
            : cloneDeep(value);
        }
      });
      return result;
    }

    /**
     * An `Array#filter` like method.
     *
     * @name filter
     * @memberOf Benchmark.Suite
     * @param {Function|string} callback The function/alias called per iteration.
     * @returns {Object} A new suite of benchmarks that passed callback filter.
     */
    function filterSuite(callback) {
      var suite = this,
          result = new suite.constructor(suite.options);

      result.push.apply(result, filter(suite, callback));
      return result;
    }

    /**
     * Resets all benchmarks in the suite.
     *
     * @name reset
     * @memberOf Benchmark.Suite
     * @returns {Object} The suite instance.
     */
    function resetSuite() {
      var event,
          suite = this,
          aborting = calledBy.abortSuite;

      if (suite.running && !aborting) {
        // No worries, `resetSuite()` is called within `abortSuite()`.
        calledBy.resetSuite = true;
        suite.abort();
        delete calledBy.resetSuite;
      }
      // Reset if the state has changed.
      else if ((suite.aborted || suite.running) &&
          (suite.emit(event = Event('reset')), !event.cancelled)) {
        suite.aborted = suite.running = false;
        if (!aborting) {
          invoke(suite, 'reset');
        }
      }
      return suite;
    }

    /**
     * Runs the suite.
     *
     * @name run
     * @memberOf Benchmark.Suite
     * @param {Object} [options={}] Options object.
     * @returns {Object} The suite instance.
     * @example
     *
     * // basic usage
     * suite.run();
     *
     * // or with options
     * suite.run({ 'async': true, 'queued': true });
     */
    function runSuite(options) {
      var suite = this;

      suite.reset();
      suite.running = true;
      options || (options = {});

      invoke(suite, {
        'name': 'run',
        'args': options,
        'queued': options.queued,
        'onStart': function(event) {
          suite.emit(event);
        },
        'onCycle': function(event) {
          var bench = event.target;
          if (bench.error) {
            suite.emit({ 'type': 'error', 'target': bench });
          }
          suite.emit(event);
          event.aborted = suite.aborted;
        },
        'onComplete': function(event) {
          suite.running = false;
          suite.emit(event);
        }
      });
      return suite;
    }

    /*------------------------------------------------------------------------*/

    /**
     * Executes all registered listeners of the specified event type.
     *
     * @memberOf Benchmark, Benchmark.Suite
     * @param {Object|string} type The event type or object.
     * @param {...*} [args] Arguments to invoke the listener with.
     * @returns {*} Returns the return value of the last listener executed.
     */
    function emit(type) {
      var listeners,
          object = this,
          event = Event(type),
          events = object.events,
          args = (arguments[0] = event, arguments);

      event.currentTarget || (event.currentTarget = object);
      event.target || (event.target = object);
      delete event.result;

      if (events && (listeners = _.has(events, event.type) && events[event.type])) {
        _.each(listeners.slice(), function(listener) {
          if ((event.result = listener.apply(object, args)) === false) {
            event.cancelled = true;
          }
          return !event.aborted;
        });
      }
      return event.result;
    }

    /**
     * Returns an array of event listeners for a given type that can be manipulated
     * to add or remove listeners.
     *
     * @memberOf Benchmark, Benchmark.Suite
     * @param {string} type The event type.
     * @returns {Array} The listeners array.
     */
    function listeners(type) {
      var object = this,
          events = object.events || (object.events = {});

      return _.has(events, type) ? events[type] : (events[type] = []);
    }

    /**
     * Unregisters a listener for the specified event type(s),
     * or unregisters all listeners for the specified event type(s),
     * or unregisters all listeners for all event types.
     *
     * @memberOf Benchmark, Benchmark.Suite
     * @param {string} [type] The event type.
     * @param {Function} [listener] The function to unregister.
     * @returns {Object} The current instance.
     * @example
     *
     * // unregister a listener for an event type
     * bench.off('cycle', listener);
     *
     * // unregister a listener for multiple event types
     * bench.off('start cycle', listener);
     *
     * // unregister all listeners for an event type
     * bench.off('cycle');
     *
     * // unregister all listeners for multiple event types
     * bench.off('start cycle complete');
     *
     * // unregister all listeners for all event types
     * bench.off();
     */
    function off(type, listener) {
      var object = this,
          events = object.events;

      if (!events) {
        return object;
      }
      _.each(type ? type.split(' ') : events, function(listeners, type) {
        var index;
        if (typeof listeners == 'string') {
          type = listeners;
          listeners = _.has(events, type) && events[type];
        }
        if (listeners) {
          if (listener) {
            index = _.indexOf(listeners, listener);
            if (index > -1) {
              listeners.splice(index, 1);
            }
          } else {
            listeners.length = 0;
          }
        }
      });
      return object;
    }

    /**
     * Registers a listener for the specified event type(s).
     *
     * @memberOf Benchmark, Benchmark.Suite
     * @param {string} type The event type.
     * @param {Function} listener The function to register.
     * @returns {Object} The current instance.
     * @example
     *
     * // register a listener for an event type
     * bench.on('cycle', listener);
     *
     * // register a listener for multiple event types
     * bench.on('start cycle', listener);
     */
    function on(type, listener) {
      var object = this,
          events = object.events || (object.events = {});

      _.each(type.split(' '), function(type) {
        (_.has(events, type)
          ? events[type]
          : (events[type] = [])
        ).push(listener);
      });
      return object;
    }

    /*------------------------------------------------------------------------*/

    /**
     * Aborts the benchmark without recording times.
     *
     * @memberOf Benchmark
     * @returns {Object} The benchmark instance.
     */
    function abort() {
      var event,
          bench = this,
          resetting = calledBy.reset;

      if (bench.running) {
        event = Event('abort');
        bench.emit(event);
        if (!event.cancelled || resetting) {
          // Avoid infinite recursion.
          calledBy.abort = true;
          bench.reset();
          delete calledBy.abort;

          if (support.timeout) {
            clearTimeout(bench._timerId);
            delete bench._timerId;
          }
          if (!resetting) {
            bench.aborted = true;
            bench.running = false;
          }
        }
      }
      return bench;
    }

    /**
     * Creates a new benchmark using the same test and options.
     *
     * @memberOf Benchmark
     * @param {Object} options Options object to overwrite cloned options.
     * @returns {Object} The new benchmark instance.
     * @example
     *
     * var bizarro = bench.clone({
     *   'name': 'doppelganger'
     * });
     */
    function clone(options) {
      var bench = this,
          result = new bench.constructor(_.assign({}, bench, options));

      // Correct the `options` object.
      result.options = _.assign({}, cloneDeep(bench.options), cloneDeep(options));

      // Copy own custom properties.
      _.forOwn(bench, function(value, key) {
        if (!_.has(result, key)) {
          result[key] = cloneDeep(value);
        }
      });

      return result;
    }

    /**
     * Determines if a benchmark is faster than another.
     *
     * @memberOf Benchmark
     * @param {Object} other The benchmark to compare.
     * @returns {number} Returns `-1` if slower, `1` if faster, and `0` if indeterminate.
     */
    function compare(other) {
      var bench = this;

      // Exit early if comparing the same benchmark.
      if (bench == other) {
        return 0;
      }
      var critical,
          zStat,
          sample1 = bench.stats.sample,
          sample2 = other.stats.sample,
          size1 = sample1.length,
          size2 = sample2.length,
          maxSize = max(size1, size2),
          minSize = min(size1, size2),
          u1 = getU(sample1, sample2),
          u2 = getU(sample2, sample1),
          u = min(u1, u2);

      function getScore(xA, sampleB) {
        return _.reduce(sampleB, function(total, xB) {
          return total + (xB > xA ? 0 : xB < xA ? 1 : 0.5);
        }, 0);
      }

      function getU(sampleA, sampleB) {
        return _.reduce(sampleA, function(total, xA) {
          return total + getScore(xA, sampleB);
        }, 0);
      }

      function getZ(u) {
        return (u - ((size1 * size2) / 2)) / sqrt((size1 * size2 * (size1 + size2 + 1)) / 12);
      }
      // Reject the null hypothesis the two samples come from the
      // same population (i.e. have the same median) if...
      if (size1 + size2 > 30) {
        // ...the z-stat is greater than 1.96 or less than -1.96
        // http://www.statisticslectures.com/topics/mannwhitneyu/
        zStat = getZ(u);
        return abs(zStat) > 1.96 ? (u == u1 ? 1 : -1) : 0;
      }
      // ...the U value is less than or equal the critical U value.
      critical = maxSize < 5 || minSize < 3 ? 0 : uTable[maxSize][minSize - 3];
      return u <= critical ? (u == u1 ? 1 : -1) : 0;
    }

    /**
     * Reset properties and abort if running.
     *
     * @memberOf Benchmark
     * @returns {Object} The benchmark instance.
     */
    function reset() {
      var bench = this;
      if (bench.running && !calledBy.abort) {
        // No worries, `reset()` is called within `abort()`.
        calledBy.reset = true;
        bench.abort();
        delete calledBy.reset;
        return bench;
      }
      var event,
          index = 0,
          changes = [],
          queue = [];

      // A non-recursive solution to check if properties have changed.
      // For more information see http://www.jslab.dk/articles/non.recursive.preorder.traversal.part4.
      var data = {
        'destination': bench,
        'source': _.assign({}, cloneDeep(bench.constructor.prototype), cloneDeep(bench.options))
      };

      do {
        _.forOwn(data.source, function(value, key) {
          var changed,
              destination = data.destination,
              currValue = destination[key];

          // Skip pseudo private properties like `_timerId` which could be a
          // Java object in environments like RingoJS.
          if (key.charAt(0) == '_') {
            return;
          }
          if (value && typeof value == 'object') {
            if (_.isArray(value)) {
              // Check if an array value has changed to a non-array value.
              if (!_.isArray(currValue)) {
                changed = currValue = [];
              }
              // Check if an array has changed its length.
              if (currValue.length != value.length) {
                changed = currValue = currValue.slice(0, value.length);
                currValue.length = value.length;
              }
            }
            // Check if an object has changed to a non-object value.
            else if (!currValue || typeof currValue != 'object') {
              changed = currValue = {};
            }
            // Register a changed object.
            if (changed) {
              changes.push({ 'destination': destination, 'key': key, 'value': currValue });
            }
            queue.push({ 'destination': currValue, 'source': value });
          }
          // Register a changed primitive.
          else if (value !== currValue && !(value == null || _.isFunction(value))) {
            changes.push({ 'destination': destination, 'key': key, 'value': value });
          }
        });
      }
      while ((data = queue[index++]));

      // If changed emit the `reset` event and if it isn't cancelled reset the benchmark.
      if (changes.length && (bench.emit(event = Event('reset')), !event.cancelled)) {
        _.each(changes, function(data) {
          data.destination[data.key] = data.value;
        });
      }
      return bench;
    }

    /**
     * Displays relevant benchmark information when coerced to a string.
     *
     * @name toString
     * @memberOf Benchmark
     * @returns {string} A string representation of the benchmark instance.
     */
    function toStringBench() {
      var bench = this,
          error = bench.error,
          hz = bench.hz,
          id = bench.id,
          stats = bench.stats,
          size = stats.sample.length,
          pm = '\xb1',
          result = bench.name || (_.isNaN(id) ? id : '<Test #' + id + '>');

      if (error) {
        result += ': ' + join(error);
      } else {
        result += ' x ' + formatNumber(hz.toFixed(hz < 100 ? 2 : 0)) + ' ops/sec ' + pm +
          stats.rme.toFixed(2) + '% (' + size + ' run' + (size == 1 ? '' : 's') + ' sampled)';
      }
      return result;
    }

    /*------------------------------------------------------------------------*/

    /**
     * Clocks the time taken to execute a test per cycle (secs).
     *
     * @private
     * @param {Object} bench The benchmark instance.
     * @returns {number} The time taken.
     */
    function clock() {
      var options = Benchmark.options,
          templateData = {},
          timers = [{ 'ns': timer.ns, 'res': max(0.0015, getRes('ms')), 'unit': 'ms' }];

      // Lazy define for hi-res timers.
      clock = function(clone) {
        var deferred;

        if (clone instanceof Deferred) {
          deferred = clone;
          clone = deferred.benchmark;
        }
        var bench = clone._original,
            stringable = isStringable(bench.fn),
            count = bench.count = clone.count,
            decompilable = stringable || (support.decompilation && (clone.setup !== _.noop || clone.teardown !== _.noop)),
            id = bench.id,
            name = bench.name || (typeof id == 'number' ? '<Test #' + id + '>' : id),
            result = 0;

        // Init `minTime` if needed.
        clone.minTime = bench.minTime || (bench.minTime = bench.options.minTime = options.minTime);

        // Compile in setup/teardown functions and the test loop.
        // Create a new compiled test, instead of using the cached `bench.compiled`,
        // to avoid potential engine optimizations enabled over the life of the test.
        var funcBody = deferred
          ? 'var d#=this,${fnArg}=d#,m#=d#.benchmark._original,f#=m#.fn,su#=m#.setup,td#=m#.teardown;' +
            // When `deferred.cycles` is `0` then...
            'if(!d#.cycles){' +
            // set `deferred.fn`,
            'd#.fn=function(){var ${fnArg}=d#;if(typeof f#=="function"){try{${fn}\n}catch(e#){f#(d#)}}else{${fn}\n}};' +
            // set `deferred.teardown`,
            'd#.teardown=function(){d#.cycles=0;if(typeof td#=="function"){try{${teardown}\n}catch(e#){td#()}}else{${teardown}\n}};' +
            // execute the benchmark's `setup`,
            'if(typeof su#=="function"){try{${setup}\n}catch(e#){su#()}}else{${setup}\n};' +
            // start timer,
            't#.start(d#);' +
            // and then execute `deferred.fn` and return a dummy object.
            '}d#.fn();return{uid:"${uid}"}'

          : 'var r#,s#,m#=this,f#=m#.fn,i#=m#.count,n#=t#.ns;${setup}\n${begin};' +
            'while(i#--){${fn}\n}${end};${teardown}\nreturn{elapsed:r#,uid:"${uid}"}';

        var compiled = bench.compiled = clone.compiled = createCompiled(bench, decompilable, deferred, funcBody),
            isEmpty = !(templateData.fn || stringable);

        try {
          if (isEmpty) {
            // Firefox may remove dead code from `Function#toString` results.
            // For more information see http://bugzil.la/536085.
            throw new Error('The test "' + name + '" is empty. This may be the result of dead code removal.');
          }
          else if (!deferred) {
            // Pretest to determine if compiled code exits early, usually by a
            // rogue `return` statement, by checking for a return object with the uid.
            bench.count = 1;
            compiled = decompilable && (compiled.call(bench, context, timer) || {}).uid == templateData.uid && compiled;
            bench.count = count;
          }
        } catch(e) {
          compiled = null;
          clone.error = e || new Error(String(e));
          bench.count = count;
        }
        // Fallback when a test exits early or errors during pretest.
        if (!compiled && !deferred && !isEmpty) {
          funcBody = (
            stringable || (decompilable && !clone.error)
              ? 'function f#(){${fn}\n}var r#,s#,m#=this,i#=m#.count'
              : 'var r#,s#,m#=this,f#=m#.fn,i#=m#.count'
            ) +
            ',n#=t#.ns;${setup}\n${begin};m#.f#=f#;while(i#--){m#.f#()}${end};' +
            'delete m#.f#;${teardown}\nreturn{elapsed:r#}';

          compiled = createCompiled(bench, decompilable, deferred, funcBody);

          try {
            // Pretest one more time to check for errors.
            bench.count = 1;
            compiled.call(bench, context, timer);
            bench.count = count;
            delete clone.error;
          }
          catch(e) {
            bench.count = count;
            if (!clone.error) {
              clone.error = e || new Error(String(e));
            }
          }
        }
        // If no errors run the full test loop.
        if (!clone.error) {
          compiled = bench.compiled = clone.compiled = createCompiled(bench, decompilable, deferred, funcBody);
          result = compiled.call(deferred || bench, context, timer).elapsed;
        }
        return result;
      };

      /*----------------------------------------------------------------------*/

      /**
       * Creates a compiled function from the given function `body`.
       */
      function createCompiled(bench, decompilable, deferred, body) {
        var fn = bench.fn,
            fnArg = deferred ? getFirstArgument(fn) || 'deferred' : '';

        templateData.uid = uid + uidCounter++;

        _.assign(templateData, {
          'setup': decompilable ? getSource(bench.setup) : interpolate('m#.setup()'),
          'fn': decompilable ? getSource(fn) : interpolate('m#.fn(' + fnArg + ')'),
          'fnArg': fnArg,
          'teardown': decompilable ? getSource(bench.teardown) : interpolate('m#.teardown()')
        });

        // Use API of chosen timer.
        if (timer.unit == 'ns') {
          _.assign(templateData, {
            'begin': interpolate('s#=n#()'),
            'end': interpolate('r#=n#(s#);r#=r#[0]+(r#[1]/1e9)')
          });
        }
        else if (timer.unit == 'us') {
          if (timer.ns.stop) {
            _.assign(templateData, {
              'begin': interpolate('s#=n#.start()'),
              'end': interpolate('r#=n#.microseconds()/1e6')
            });
          } else {
            _.assign(templateData, {
              'begin': interpolate('s#=n#()'),
              'end': interpolate('r#=(n#()-s#)/1e6')
            });
          }
        }
        else if (timer.ns.now) {
          _.assign(templateData, {
            'begin': interpolate('s#=n#.now()'),
            'end': interpolate('r#=(n#.now()-s#)/1e3')
          });
        }
        else {
          _.assign(templateData, {
            'begin': interpolate('s#=new n#().getTime()'),
            'end': interpolate('r#=(new n#().getTime()-s#)/1e3')
          });
        }
        // Define `timer` methods.
        timer.start = createFunction(
          interpolate('o#'),
          interpolate('var n#=this.ns,${begin};o#.elapsed=0;o#.timeStamp=s#')
        );

        timer.stop = createFunction(
          interpolate('o#'),
          interpolate('var n#=this.ns,s#=o#.timeStamp,${end};o#.elapsed=r#')
        );

        // Create compiled test.
        return createFunction(
          interpolate('window,t#'),
          'var global = window, clearTimeout = global.clearTimeout, setTimeout = global.setTimeout;\n' +
          interpolate(body)
        );
      }

      /**
       * Gets the current timer's minimum resolution (secs).
       */
      function getRes(unit) {
        var measured,
            begin,
            count = 30,
            divisor = 1e3,
            ns = timer.ns,
            sample = [];

        // Get average smallest measurable time.
        while (count--) {
          if (unit == 'us') {
            divisor = 1e6;
            if (ns.stop) {
              ns.start();
              while (!(measured = ns.microseconds())) {}
            } else {
              begin = ns();
              while (!(measured = ns() - begin)) {}
            }
          }
          else if (unit == 'ns') {
            divisor = 1e9;
            begin = (begin = ns())[0] + (begin[1] / divisor);
            while (!(measured = ((measured = ns())[0] + (measured[1] / divisor)) - begin)) {}
            divisor = 1;
          }
          else if (ns.now) {
            begin = ns.now();
            while (!(measured = ns.now() - begin)) {}
          }
          else {
            begin = new ns().getTime();
            while (!(measured = new ns().getTime() - begin)) {}
          }
          // Check for broken timers.
          if (measured > 0) {
            sample.push(measured);
          } else {
            sample.push(Infinity);
            break;
          }
        }
        // Convert to seconds.
        return getMean(sample) / divisor;
      }

      /**
       * Interpolates a given template string.
       */
      function interpolate(string) {
        // Replaces all occurrences of `#` with a unique number and template tokens with content.
        return _.template(string.replace(/\#/g, /\d+/.exec(templateData.uid)))(templateData);
      }

      /*----------------------------------------------------------------------*/

      // Detect Chrome's microsecond timer:
      // enable benchmarking via the --enable-benchmarking command
      // line switch in at least Chrome 7 to use chrome.Interval
      try {
        if ((timer.ns = new (context.chrome || context.chromium).Interval)) {
          timers.push({ 'ns': timer.ns, 'res': getRes('us'), 'unit': 'us' });
        }
      } catch(e) {}

      // Detect Node.js's nanosecond resolution timer available in Node.js >= 0.8.
      if (processObject && typeof (timer.ns = processObject.hrtime) == 'function') {
        timers.push({ 'ns': timer.ns, 'res': getRes('ns'), 'unit': 'ns' });
      }
      // Detect Wade Simmons' Node.js `microtime` module.
      if (microtimeObject && typeof (timer.ns = microtimeObject.now) == 'function') {
        timers.push({ 'ns': timer.ns,  'res': getRes('us'), 'unit': 'us' });
      }
      // Pick timer with highest resolution.
      timer = _.minBy(timers, 'res');

      // Error if there are no working timers.
      if (timer.res == Infinity) {
        throw new Error('Benchmark.js was unable to find a working timer.');
      }
      // Resolve time span required to achieve a percent uncertainty of at most 1%.
      // For more information see http://spiff.rit.edu/classes/phys273/uncert/uncert.html.
      options.minTime || (options.minTime = max(timer.res / 2 / 0.01, 0.05));
      return clock.apply(null, arguments);
    }

    /*------------------------------------------------------------------------*/

    /**
     * Computes stats on benchmark results.
     *
     * @private
     * @param {Object} bench The benchmark instance.
     * @param {Object} options The options object.
     */
    function compute(bench, options) {
      options || (options = {});

      var async = options.async,
          elapsed = 0,
          initCount = bench.initCount,
          minSamples = bench.minSamples,
          queue = [],
          sample = bench.stats.sample;

      /**
       * Adds a clone to the queue.
       */
      function enqueue() {
        queue.push(bench.clone({
          '_original': bench,
          'events': {
            'abort': [update],
            'cycle': [update],
            'error': [update],
            'start': [update]
          }
        }));
      }

      /**
       * Updates the clone/original benchmarks to keep their data in sync.
       */
      function update(event) {
        var clone = this,
            type = event.type;

        if (bench.running) {
          if (type == 'start') {
            // Note: `clone.minTime` prop is inited in `clock()`.
            clone.count = bench.initCount;
          }
          else {
            if (type == 'error') {
              bench.error = clone.error;
            }
            if (type == 'abort') {
              bench.abort();
              bench.emit('cycle');
            } else {
              event.currentTarget = event.target = bench;
              bench.emit(event);
            }
          }
        } else if (bench.aborted) {
          // Clear abort listeners to avoid triggering bench's abort/cycle again.
          clone.events.abort.length = 0;
          clone.abort();
        }
      }

      /**
       * Determines if more clones should be queued or if cycling should stop.
       */
      function evaluate(event) {
        var critical,
            df,
            mean,
            moe,
            rme,
            sd,
            sem,
            variance,
            clone = event.target,
            done = bench.aborted,
            now = _.now(),
            size = sample.push(clone.times.period),
            maxedOut = size >= minSamples && (elapsed += now - clone.times.timeStamp) / 1e3 > bench.maxTime,
            times = bench.times,
            varOf = function(sum, x) { return sum + pow(x - mean, 2); };

        // Exit early for aborted or unclockable tests.
        if (done || clone.hz == Infinity) {
          maxedOut = !(size = sample.length = queue.length = 0);
        }

        if (!done) {
          // Compute the sample mean (estimate of the population mean).
          mean = getMean(sample);
          // Compute the sample variance (estimate of the population variance).
          variance = _.reduce(sample, varOf, 0) / (size - 1) || 0;
          // Compute the sample standard deviation (estimate of the population standard deviation).
          sd = sqrt(variance);
          // Compute the standard error of the mean (a.k.a. the standard deviation of the sampling distribution of the sample mean).
          sem = sd / sqrt(size);
          // Compute the degrees of freedom.
          df = size - 1;
          // Compute the critical value.
          critical = tTable[Math.round(df) || 1] || tTable.infinity;
          // Compute the margin of error.
          moe = sem * critical;
          // Compute the relative margin of error.
          rme = (moe / mean) * 100 || 0;

          _.assign(bench.stats, {
            'deviation': sd,
            'mean': mean,
            'moe': moe,
            'rme': rme,
            'sem': sem,
            'variance': variance
          });

          // Abort the cycle loop when the minimum sample size has been collected
          // and the elapsed time exceeds the maximum time allowed per benchmark.
          // We don't count cycle delays toward the max time because delays may be
          // increased by browsers that clamp timeouts for inactive tabs. For more
          // information see https://developer.mozilla.org/en/window.setTimeout#Inactive_tabs.
          if (maxedOut) {
            // Reset the `initCount` in case the benchmark is rerun.
            bench.initCount = initCount;
            bench.running = false;
            done = true;
            times.elapsed = (now - times.timeStamp) / 1e3;
          }
          if (bench.hz != Infinity) {
            bench.hz = 1 / mean;
            times.cycle = mean * bench.count;
            times.period = mean;
          }
        }
        // If time permits, increase sample size to reduce the margin of error.
        if (queue.length < 2 && !maxedOut) {
          enqueue();
        }
        // Abort the `invoke` cycle when done.
        event.aborted = done;
      }

      // Init queue and begin.
      enqueue();
      invoke(queue, {
        'name': 'run',
        'args': { 'async': async },
        'queued': true,
        'onCycle': evaluate,
        'onComplete': function() { bench.emit('complete'); }
      });
    }

    /*------------------------------------------------------------------------*/

    /**
     * Cycles a benchmark until a run `count` can be established.
     *
     * @private
     * @param {Object} clone The cloned benchmark instance.
     * @param {Object} options The options object.
     */
    function cycle(clone, options) {
      options || (options = {});

      var deferred;
      if (clone instanceof Deferred) {
        deferred = clone;
        clone = clone.benchmark;
      }
      var clocked,
          cycles,
          divisor,
          event,
          minTime,
          period,
          async = options.async,
          bench = clone._original,
          count = clone.count,
          times = clone.times;

      // Continue, if not aborted between cycles.
      if (clone.running) {
        // `minTime` is set to `Benchmark.options.minTime` in `clock()`.
        cycles = ++clone.cycles;
        clocked = deferred ? deferred.elapsed : clock(clone);
        minTime = clone.minTime;

        if (cycles > bench.cycles) {
          bench.cycles = cycles;
        }
        if (clone.error) {
          event = Event('error');
          event.message = clone.error;
          clone.emit(event);
          if (!event.cancelled) {
            clone.abort();
          }
        }
      }
      // Continue, if not errored.
      if (clone.running) {
        // Compute the time taken to complete last test cycle.
        bench.times.cycle = times.cycle = clocked;
        // Compute the seconds per operation.
        period = bench.times.period = times.period = clocked / count;
        // Compute the ops per second.
        bench.hz = clone.hz = 1 / period;
        // Avoid working our way up to this next time.
        bench.initCount = clone.initCount = count;
        // Do we need to do another cycle?
        clone.running = clocked < minTime;

        if (clone.running) {
          // Tests may clock at `0` when `initCount` is a small number,
          // to avoid that we set its count to something a bit higher.
          if (!clocked && (divisor = divisors[clone.cycles]) != null) {
            count = floor(4e6 / divisor);
          }
          // Calculate how many more iterations it will take to achieve the `minTime`.
          if (count <= clone.count) {
            count += Math.ceil((minTime - clocked) / period);
          }
          clone.running = count != Infinity;
        }
      }
      // Should we exit early?
      event = Event('cycle');
      clone.emit(event);
      if (event.aborted) {
        clone.abort();
      }
      // Figure out what to do next.
      if (clone.running) {
        // Start a new cycle.
        clone.count = count;
        if (deferred) {
          clone.compiled.call(deferred, context, timer);
        } else if (async) {
          delay(clone, function() { cycle(clone, options); });
        } else {
          cycle(clone);
        }
      }
      else {
        // Fix TraceMonkey bug associated with clock fallbacks.
        // For more information see http://bugzil.la/509069.
        if (support.browser) {
          runScript(uid + '=1;delete ' + uid);
        }
        // We're done.
        clone.emit('complete');
      }
    }

    /*------------------------------------------------------------------------*/

    /**
     * Runs the benchmark.
     *
     * @memberOf Benchmark
     * @param {Object} [options={}] Options object.
     * @returns {Object} The benchmark instance.
     * @example
     *
     * // basic usage
     * bench.run();
     *
     * // or with options
     * bench.run({ 'async': true });
     */
    function run(options) {
      var bench = this,
          event = Event('start');

      // Set `running` to `false` so `reset()` won't call `abort()`.
      bench.running = false;
      bench.reset();
      bench.running = true;

      bench.count = bench.initCount;
      bench.times.timeStamp = _.now();
      bench.emit(event);

      if (!event.cancelled) {
        options = { 'async': ((options = options && options.async) == null ? bench.async : options) && support.timeout };

        // For clones created within `compute()`.
        if (bench._original) {
          if (bench.defer) {
            Deferred(bench);
          } else {
            cycle(bench, options);
          }
        }
        // For original benchmarks.
        else {
          compute(bench, options);
        }
      }
      return bench;
    }

    /*------------------------------------------------------------------------*/

    // Firefox 1 erroneously defines variable and argument names of functions on
    // the function itself as non-configurable properties with `undefined` values.
    // The bugginess continues as the `Benchmark` constructor has an argument
    // named `options` and Firefox 1 will not assign a value to `Benchmark.options`,
    // making it non-writable in the process, unless it is the first property
    // assigned by for-in loop of `_.assign()`.
    _.assign(Benchmark, {

      /**
       * The default options copied by benchmark instances.
       *
       * @static
       * @memberOf Benchmark
       * @type Object
       */
      'options': {

        /**
         * A flag to indicate that benchmark cycles will execute asynchronously
         * by default.
         *
         * @memberOf Benchmark.options
         * @type boolean
         */
        'async': false,

        /**
         * A flag to indicate that the benchmark clock is deferred.
         *
         * @memberOf Benchmark.options
         * @type boolean
         */
        'defer': false,

        /**
         * The delay between test cycles (secs).
         * @memberOf Benchmark.options
         * @type number
         */
        'delay': 0.005,

        /**
         * Displayed by `Benchmark#toString` when a `name` is not available
         * (auto-generated if absent).
         *
         * @memberOf Benchmark.options
         * @type string
         */
        'id': undefined,

        /**
         * The default number of times to execute a test on a benchmark's first cycle.
         *
         * @memberOf Benchmark.options
         * @type number
         */
        'initCount': 1,

        /**
         * The maximum time a benchmark is allowed to run before finishing (secs).
         *
         * Note: Cycle delays aren't counted toward the maximum time.
         *
         * @memberOf Benchmark.options
         * @type number
         */
        'maxTime': 5,

        /**
         * The minimum sample size required to perform statistical analysis.
         *
         * @memberOf Benchmark.options
         * @type number
         */
        'minSamples': 5,

        /**
         * The time needed to reduce the percent uncertainty of measurement to 1% (secs).
         *
         * @memberOf Benchmark.options
         * @type number
         */
        'minTime': 0,

        /**
         * The name of the benchmark.
         *
         * @memberOf Benchmark.options
         * @type string
         */
        'name': undefined,

        /**
         * An event listener called when the benchmark is aborted.
         *
         * @memberOf Benchmark.options
         * @type Function
         */
        'onAbort': undefined,

        /**
         * An event listener called when the benchmark completes running.
         *
         * @memberOf Benchmark.options
         * @type Function
         */
        'onComplete': undefined,

        /**
         * An event listener called after each run cycle.
         *
         * @memberOf Benchmark.options
         * @type Function
         */
        'onCycle': undefined,

        /**
         * An event listener called when a test errors.
         *
         * @memberOf Benchmark.options
         * @type Function
         */
        'onError': undefined,

        /**
         * An event listener called when the benchmark is reset.
         *
         * @memberOf Benchmark.options
         * @type Function
         */
        'onReset': undefined,

        /**
         * An event listener called when the benchmark starts running.
         *
         * @memberOf Benchmark.options
         * @type Function
         */
        'onStart': undefined
      },

      /**
       * Platform object with properties describing things like browser name,
       * version, and operating system. See [`platform.js`](https://mths.be/platform).
       *
       * @static
       * @memberOf Benchmark
       * @type Object
       */
      'platform': context.platform || require('platform') || ({
        'description': context.navigator && context.navigator.userAgent || null,
        'layout': null,
        'product': null,
        'name': null,
        'manufacturer': null,
        'os': null,
        'prerelease': null,
        'version': null,
        'toString': function() {
          return this.description || '';
        }
      }),

      /**
       * The semantic version number.
       *
       * @static
       * @memberOf Benchmark
       * @type string
       */
      'version': '2.1.1'
    });

    _.assign(Benchmark, {
      'filter': filter,
      'formatNumber': formatNumber,
      'invoke': invoke,
      'join': join,
      'runInContext': runInContext,
      'support': support
    });

    // Add lodash methods to Benchmark.
    _.each(['each', 'forEach', 'forOwn', 'has', 'indexOf', 'map', 'reduce'], function(methodName) {
      Benchmark[methodName] = _[methodName];
    });

    /*------------------------------------------------------------------------*/

    _.assign(Benchmark.prototype, {

      /**
       * The number of times a test was executed.
       *
       * @memberOf Benchmark
       * @type number
       */
      'count': 0,

      /**
       * The number of cycles performed while benchmarking.
       *
       * @memberOf Benchmark
       * @type number
       */
      'cycles': 0,

      /**
       * The number of executions per second.
       *
       * @memberOf Benchmark
       * @type number
       */
      'hz': 0,

      /**
       * The compiled test function.
       *
       * @memberOf Benchmark
       * @type {Function|string}
       */
      'compiled': undefined,

      /**
       * The error object if the test failed.
       *
       * @memberOf Benchmark
       * @type Object
       */
      'error': undefined,

      /**
       * The test to benchmark.
       *
       * @memberOf Benchmark
       * @type {Function|string}
       */
      'fn': undefined,

      /**
       * A flag to indicate if the benchmark is aborted.
       *
       * @memberOf Benchmark
       * @type boolean
       */
      'aborted': false,

      /**
       * A flag to indicate if the benchmark is running.
       *
       * @memberOf Benchmark
       * @type boolean
       */
      'running': false,

      /**
       * Compiled into the test and executed immediately **before** the test loop.
       *
       * @memberOf Benchmark
       * @type {Function|string}
       * @example
       *
       * // basic usage
       * var bench = Benchmark({
       *   'setup': function() {
       *     var c = this.count,
       *         element = document.getElementById('container');
       *     while (c--) {
       *       element.appendChild(document.createElement('div'));
       *     }
       *   },
       *   'fn': function() {
       *     element.removeChild(element.lastChild);
       *   }
       * });
       *
       * // compiles to something like:
       * var c = this.count,
       *     element = document.getElementById('container');
       * while (c--) {
       *   element.appendChild(document.createElement('div'));
       * }
       * var start = new Date;
       * while (count--) {
       *   element.removeChild(element.lastChild);
       * }
       * var end = new Date - start;
       *
       * // or using strings
       * var bench = Benchmark({
       *   'setup': '\
       *     var a = 0;\n\
       *     (function() {\n\
       *       (function() {\n\
       *         (function() {',
       *   'fn': 'a += 1;',
       *   'teardown': '\
       *          }())\n\
       *        }())\n\
       *      }())'
       * });
       *
       * // compiles to something like:
       * var a = 0;
       * (function() {
       *   (function() {
       *     (function() {
       *       var start = new Date;
       *       while (count--) {
       *         a += 1;
       *       }
       *       var end = new Date - start;
       *     }())
       *   }())
       * }())
       */
      'setup': _.noop,

      /**
       * Compiled into the test and executed immediately **after** the test loop.
       *
       * @memberOf Benchmark
       * @type {Function|string}
       */
      'teardown': _.noop,

      /**
       * An object of stats including mean, margin or error, and standard deviation.
       *
       * @memberOf Benchmark
       * @type Object
       */
      'stats': {

        /**
         * The margin of error.
         *
         * @memberOf Benchmark#stats
         * @type number
         */
        'moe': 0,

        /**
         * The relative margin of error (expressed as a percentage of the mean).
         *
         * @memberOf Benchmark#stats
         * @type number
         */
        'rme': 0,

        /**
         * The standard error of the mean.
         *
         * @memberOf Benchmark#stats
         * @type number
         */
        'sem': 0,

        /**
         * The sample standard deviation.
         *
         * @memberOf Benchmark#stats
         * @type number
         */
        'deviation': 0,

        /**
         * The sample arithmetic mean (secs).
         *
         * @memberOf Benchmark#stats
         * @type number
         */
        'mean': 0,

        /**
         * The array of sampled periods.
         *
         * @memberOf Benchmark#stats
         * @type Array
         */
        'sample': [],

        /**
         * The sample variance.
         *
         * @memberOf Benchmark#stats
         * @type number
         */
        'variance': 0
      },

      /**
       * An object of timing data including cycle, elapsed, period, start, and stop.
       *
       * @memberOf Benchmark
       * @type Object
       */
      'times': {

        /**
         * The time taken to complete the last cycle (secs).
         *
         * @memberOf Benchmark#times
         * @type number
         */
        'cycle': 0,

        /**
         * The time taken to complete the benchmark (secs).
         *
         * @memberOf Benchmark#times
         * @type number
         */
        'elapsed': 0,

        /**
         * The time taken to execute the test once (secs).
         *
         * @memberOf Benchmark#times
         * @type number
         */
        'period': 0,

        /**
         * A timestamp of when the benchmark started (ms).
         *
         * @memberOf Benchmark#times
         * @type number
         */
        'timeStamp': 0
      }
    });

    _.assign(Benchmark.prototype, {
      'abort': abort,
      'clone': clone,
      'compare': compare,
      'emit': emit,
      'listeners': listeners,
      'off': off,
      'on': on,
      'reset': reset,
      'run': run,
      'toString': toStringBench
    });

    /*------------------------------------------------------------------------*/

    _.assign(Deferred.prototype, {

      /**
       * The deferred benchmark instance.
       *
       * @memberOf Benchmark.Deferred
       * @type Object
       */
      'benchmark': null,

      /**
       * The number of deferred cycles performed while benchmarking.
       *
       * @memberOf Benchmark.Deferred
       * @type number
       */
      'cycles': 0,

      /**
       * The time taken to complete the deferred benchmark (secs).
       *
       * @memberOf Benchmark.Deferred
       * @type number
       */
      'elapsed': 0,

      /**
       * A timestamp of when the deferred benchmark started (ms).
       *
       * @memberOf Benchmark.Deferred
       * @type number
       */
      'timeStamp': 0
    });

    _.assign(Deferred.prototype, {
      'resolve': resolve
    });

    /*------------------------------------------------------------------------*/

    _.assign(Event.prototype, {

      /**
       * A flag to indicate if the emitters listener iteration is aborted.
       *
       * @memberOf Benchmark.Event
       * @type boolean
       */
      'aborted': false,

      /**
       * A flag to indicate if the default action is cancelled.
       *
       * @memberOf Benchmark.Event
       * @type boolean
       */
      'cancelled': false,

      /**
       * The object whose listeners are currently being processed.
       *
       * @memberOf Benchmark.Event
       * @type Object
       */
      'currentTarget': undefined,

      /**
       * The return value of the last executed listener.
       *
       * @memberOf Benchmark.Event
       * @type Mixed
       */
      'result': undefined,

      /**
       * The object to which the event was originally emitted.
       *
       * @memberOf Benchmark.Event
       * @type Object
       */
      'target': undefined,

      /**
       * A timestamp of when the event was created (ms).
       *
       * @memberOf Benchmark.Event
       * @type number
       */
      'timeStamp': 0,

      /**
       * The event type.
       *
       * @memberOf Benchmark.Event
       * @type string
       */
      'type': ''
    });

    /*------------------------------------------------------------------------*/

    /**
     * The default options copied by suite instances.
     *
     * @static
     * @memberOf Benchmark.Suite
     * @type Object
     */
    Suite.options = {

      /**
       * The name of the suite.
       *
       * @memberOf Benchmark.Suite.options
       * @type string
       */
      'name': undefined
    };

    /*------------------------------------------------------------------------*/

    _.assign(Suite.prototype, {

      /**
       * The number of benchmarks in the suite.
       *
       * @memberOf Benchmark.Suite
       * @type number
       */
      'length': 0,

      /**
       * A flag to indicate if the suite is aborted.
       *
       * @memberOf Benchmark.Suite
       * @type boolean
       */
      'aborted': false,

      /**
       * A flag to indicate if the suite is running.
       *
       * @memberOf Benchmark.Suite
       * @type boolean
       */
      'running': false
    });

    _.assign(Suite.prototype, {
      'abort': abortSuite,
      'add': add,
      'clone': cloneSuite,
      'emit': emit,
      'filter': filterSuite,
      'join': arrayRef.join,
      'listeners': listeners,
      'off': off,
      'on': on,
      'pop': arrayRef.pop,
      'push': push,
      'reset': resetSuite,
      'run': runSuite,
      'reverse': arrayRef.reverse,
      'shift': shift,
      'slice': slice,
      'sort': arrayRef.sort,
      'splice': arrayRef.splice,
      'unshift': unshift
    });

    /*------------------------------------------------------------------------*/

    // Expose Deferred, Event, and Suite.
    _.assign(Benchmark, {
      'Deferred': Deferred,
      'Event': Event,
      'Suite': Suite
    });

    /*------------------------------------------------------------------------*/

    // Add lodash methods as Suite methods.
    _.each(['each', 'forEach', 'indexOf', 'map', 'reduce'], function(methodName) {
      var func = _[methodName];
      Suite.prototype[methodName] = function() {
        var args = [this];
        push.apply(args, arguments);
        return func.apply(_, args);
      };
    });

    // Avoid array-like object bugs with `Array#shift` and `Array#splice`
    // in Firefox < 10 and IE < 9.
    _.each(['pop', 'shift', 'splice'], function(methodName) {
      var func = arrayRef[methodName];

      Suite.prototype[methodName] = function() {
        var value = this,
            result = func.apply(value, arguments);

        if (value.length === 0) {
          delete value[0];
        }
        return result;
      };
    });

    // Avoid buggy `Array#unshift` in IE < 8 which doesn't return the new
    // length of the array.
    Suite.prototype.unshift = function() {
      var value = this;
      unshift.apply(value, arguments);
      return value.length;
    };

    return Benchmark;
  }

  /*--------------------------------------------------------------------------*/

  // Export Benchmark.
  // Some AMD build optimizers, like r.js, check for condition patterns like the following:
  if (typeof define == 'function' && typeof define.amd == 'object' && define.amd) {
    // Define as an anonymous module so, through path mapping, it can be aliased.
    define(['lodash', 'platform'], function(_, platform) {
      return runInContext({
        '_': _,
        'platform': platform
      });
    });
  }
  else {
    var Benchmark = runInContext();

    // Check for `exports` after `define` in case a build optimizer adds an `exports` object.
    if (freeExports && freeModule) {
      // Export for Node.js.
      if (moduleExports) {
        (freeModule.exports = Benchmark).Benchmark = Benchmark;
      }
      // Export for CommonJS support.
      freeExports.Benchmark = Benchmark;
    }
    else {
      // Export to the global object.
      root.Benchmark = Benchmark;
    }
  }
}.call(this));
