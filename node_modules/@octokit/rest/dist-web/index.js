// pkg/dist-src/index.js
import { Octokit as Core } from "@octokit/core";
import { requestLog } from "@octokit/plugin-request-log";
import { paginateRest } from "@octokit/plugin-paginate-rest";
import { legacyRestEndpointMethods } from "@octokit/plugin-rest-endpoint-methods";

// pkg/dist-src/version.js
var VERSION = "20.1.2";

// pkg/dist-src/index.js
var Octokit = Core.plugin(
  requestLog,
  legacyRestEndpointMethods,
  paginateRest
).defaults({
  userAgent: `octokit-rest.js/${VERSION}`
});
export {
  Octokit
};
