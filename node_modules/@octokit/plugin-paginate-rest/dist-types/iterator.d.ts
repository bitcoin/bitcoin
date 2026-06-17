import type { Octokit } from "@octokit/core";
import type { RequestInterface, RequestParameters, Route } from "./types.js";
export declare function iterator(octokit: Octokit, route: Route | RequestInterface, parameters?: RequestParameters): AsyncIterable<any>;
