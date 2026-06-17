import { Octokit as Core } from "@octokit/core";
export type { RestEndpointMethodTypes } from "@octokit/plugin-rest-endpoint-methods";
export declare const Octokit: typeof Core & import("@octokit/core/dist-types/types.js").Constructor<{
    paginate: import("@octokit/plugin-paginate-rest").PaginateInterface;
} & import("@octokit/plugin-rest-endpoint-methods/dist-types/generated/method-types.js").RestEndpointMethods & import("@octokit/plugin-rest-endpoint-methods").Api>;
export type Octokit = InstanceType<typeof Octokit>;
