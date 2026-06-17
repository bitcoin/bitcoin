import type { Octokit } from "@octokit/core";
/**
 * @param octokit Octokit instance
 * @param options Options passed to Octokit constructor
 */
export declare function requestLog(octokit: Octokit): void;
export declare namespace requestLog {
    var VERSION: string;
}
