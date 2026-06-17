"use strict";
var __defProp = Object.defineProperty;
var __getOwnPropDesc = Object.getOwnPropertyDescriptor;
var __getOwnPropNames = Object.getOwnPropertyNames;
var __hasOwnProp = Object.prototype.hasOwnProperty;
var __export = (target, all) => {
  for (var name in all)
    __defProp(target, name, { get: all[name], enumerable: true });
};
var __copyProps = (to, from, except, desc) => {
  if (from && typeof from === "object" || typeof from === "function") {
    for (let key of __getOwnPropNames(from))
      if (!__hasOwnProp.call(to, key) && key !== except)
        __defProp(to, key, { get: () => from[key], enumerable: !(desc = __getOwnPropDesc(from, key)) || desc.enumerable });
  }
  return to;
};
var __toCommonJS = (mod) => __copyProps(__defProp({}, "__esModule", { value: true }), mod);

// pkg/dist-src/index.js
var index_exports = {};
__export(index_exports, {
  Octokit: () => Octokit
});
module.exports = __toCommonJS(index_exports);
var import_core = require("@octokit/core");
var import_plugin_request_log = require("@octokit/plugin-request-log");
var import_plugin_paginate_rest = require("@octokit/plugin-paginate-rest");
var import_plugin_rest_endpoint_methods = require("@octokit/plugin-rest-endpoint-methods");

// pkg/dist-src/version.js
var VERSION = "20.1.2";

// pkg/dist-src/index.js
var Octokit = import_core.Octokit.plugin(
  import_plugin_request_log.requestLog,
  import_plugin_rest_endpoint_methods.legacyRestEndpointMethods,
  import_plugin_paginate_rest.paginateRest
).defaults({
  userAgent: `octokit-rest.js/${VERSION}`
});
// Annotate the CommonJS export names for ESM import in node:
0 && (module.exports = {
  Octokit
});
