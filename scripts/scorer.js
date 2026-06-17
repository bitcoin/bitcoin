import { Octokit } from "@octokit/rest";
import { generateFeedback } from "./feedback.js";

const THRESHOLD = 65;
const BOT_COMMENT_MARKER = "<!-- core-gate-comment -->";
const READY_LABEL = "ready-for-review";
const NEEDS_WORK_LABEL = "needs-work";

const COMPONENT_PREFIXES = [
  "wallet",
  "rpc",
  "test",
  "doc",
  "docs",
  "build",
  "ci",
  "net",
  "p2p",
  "consensus",
  "script",
  "crypto",
  "qt",
  "fuzz",
  "bench",
  "kernel",
  "mempool",
  "mining",
  "fees",
  "refactor",
  "validation",
  "index",
  "gui",
  "util",
  "depends",
  "i18n",
];

async function main() {
  console.log("Core-Gate PR quality gate starting...");

  if (process.env.CORE_GATE_MOCK) {
    await runMock(process.env.CORE_GATE_MOCK);
    return;
  }

  const context = readContext();
  if (!context) return;

  const octokit = new Octokit({ auth: context.token });
  const prData = await fetchPullRequestData(octokit, context);
  const result = scorePullRequest(prData);
  const feedback = await generateFeedback({
    result,
    pr: prData.pr,
    files: prData.files,
    anthropicApiKey: process.env.ANTHROPIC_API_KEY,
  });

  const body = buildComment(result, feedback);

  console.log(`PR #${context.prNumber}: ${prData.pr.title}`);
  console.log(`Author: ${prData.pr.user?.login ?? "unknown"}`);
  console.log(`Commits: ${prData.commits.length}`);
  console.log(`Files changed: ${prData.files.length}`);
  console.log(`Additions: ${prData.totals.additions}`);
  console.log(`Deletions: ${prData.totals.deletions}`);
  console.log(`Score: ${result.score}/100`);

  await ensureLabels(octokit, context);
  await applyRoutingLabel(octokit, context, result.passed);
  await upsertComment(octokit, context, body);

  console.log("Core-Gate PR quality gate complete.");
}

async function runMock(kind) {
  const prData = buildMockPullRequestData(kind);
  const result = scorePullRequest(prData);
  const feedback = await generateFeedback({
    result,
    pr: prData.pr,
    files: prData.files,
    anthropicApiKey: "",
  });

  console.log(`Mock PR: ${kind}`);
  console.log(`Score: ${result.score}/100`);
  console.log(buildComment(result, feedback));
}

function readContext() {
  const required = ["GITHUB_TOKEN", "REPO", "PR_NUMBER"];
  const missing = required.filter((name) => !process.env[name]);

  if (missing.length > 0) {
    console.log(`Missing required env vars: ${missing.join(", ")}`);
    console.log("Run this through GitHub Actions on a pull request to score real PR data.");

    if (process.env.GITHUB_ACTIONS === "true") {
      process.exitCode = 1;
    }

    return null;
  }

  const [owner, repo] = process.env.REPO.split("/");
  const prNumber = Number(process.env.PR_NUMBER);

  if (!owner || !repo || Number.isNaN(prNumber)) {
    throw new Error("Invalid REPO or PR_NUMBER environment variable.");
  }

  return {
    token: process.env.GITHUB_TOKEN,
    owner,
    repo,
    prNumber,
  };
}

async function fetchPullRequestData(octokit, context) {
  const [prResponse, commits, files] = await Promise.all([
    octokit.pulls.get({
      owner: context.owner,
      repo: context.repo,
      pull_number: context.prNumber,
    }),
    octokit.paginate(octokit.pulls.listCommits, {
      owner: context.owner,
      repo: context.repo,
      pull_number: context.prNumber,
      per_page: 100,
    }),
    octokit.paginate(octokit.pulls.listFiles, {
      owner: context.owner,
      repo: context.repo,
      pull_number: context.prNumber,
      per_page: 100,
    }),
  ]);

  const totals = files.reduce(
    (sum, file) => ({
      additions: sum.additions + file.additions,
      deletions: sum.deletions + file.deletions,
      changes: sum.changes + file.changes,
    }),
    { additions: 0, deletions: 0, changes: 0 },
  );

  return {
    pr: prResponse.data,
    commits,
    files,
    totals,
  };
}

function scorePullRequest({ pr, commits, files, totals }) {
  const checks = [
    checkTitlePrefix(pr.title),
    checkDescription(pr.body ?? ""),
    checkCommitMessages(commits),
    checkDiffSize(files, totals),
    checkTestCoverage(files),
  ];

  const score = checks.reduce((sum, check) => sum + check.score, 0);

  return {
    score,
    threshold: THRESHOLD,
    passed: score >= THRESHOLD,
    checks,
  };
}

function buildMockPullRequestData(kind) {
  if (kind === "good") {
    const files = [
      mockFile("src/wallet/wallet.cpp", 28, 7),
      mockFile("src/wallet/test/wallet_tests.cpp", 38, 2),
    ];

    return {
      pr: {
        title: "wallet: fix fee rounding edge case",
        body:
          "This change fixes a fee rounding edge case in wallet transaction creation. The previous behavior could round a small value inconsistently, so this patch keeps the calculation explicit and adds a regression test for the affected path.",
        user: { login: "demo-contributor" },
      },
      commits: [mockCommit("wallet: fix fee rounding")],
      files,
      totals: totalFiles(files),
    };
  }

  const files = [mockFile("src/bitcoin-cli.cpp", 2, 0)];

  return {
    pr: {
      title: "fixed stuff",
      body: "",
      user: { login: "demo-contributor" },
    },
    commits: [mockCommit("fixed stuff")],
    files,
    totals: totalFiles(files),
  };
}

function mockCommit(subject) {
  return {
    commit: {
      message: subject,
    },
  };
}

function mockFile(filename, additions, deletions) {
  return {
    filename,
    additions,
    deletions,
    changes: additions + deletions,
  };
}

function totalFiles(files) {
  return files.reduce(
    (sum, file) => ({
      additions: sum.additions + file.additions,
      deletions: sum.deletions + file.deletions,
      changes: sum.changes + file.changes,
    }),
    { additions: 0, deletions: 0, changes: 0 },
  );
}

function checkTitlePrefix(title) {
  const lowerTitle = title.toLowerCase();
  const prefix = COMPONENT_PREFIXES.find((item) => lowerTitle.startsWith(`${item}:`));

  if (prefix) {
    return pass("title-prefix", "Component prefix", `Found valid prefix: ${prefix}:`);
  }

  return fail(
    "title-prefix",
    "Component prefix",
    `Title is missing a component prefix like ${COMPONENT_PREFIXES.slice(0, 6)
      .map((item) => `${item}:`)
      .join(", ")}.`,
  );
}

function checkDescription(body) {
  const normalized = body.replace(/<!--[\s\S]*?-->/g, "").trim();
  const hasMention = /(^|\s)@\w+/.test(normalized);
  const isWip = /\b(wip|work in progress)\b/i.test(normalized);

  if (normalized.length >= 100 && !hasMention && !isWip) {
    return pass("description", "PR description", "Description explains the change with enough detail.");
  }

  if (normalized.length >= 50 && !isWip) {
    return partial(
      "description",
      "PR description",
      10,
      "Description has some detail, but should better explain why the change exists.",
    );
  }

  return fail(
    "description",
    "PR description",
    "Description is too short or marked WIP. Explain the problem, approach, and impact.",
  );
}

function checkCommitMessages(commits) {
  if (commits.length === 0) {
    return fail("commit-messages", "Commit message format", "No commits found on this PR.");
  }

  const badSubjects = commits
    .map((commit) => commit.commit.message.split("\n")[0].trim())
    .filter((subject) => !isGoodCommitSubject(subject));

  if (badSubjects.length === 0) {
    return pass("commit-messages", "Commit message format", "Commit subjects are specific and concise.");
  }

  if (badSubjects.length < commits.length) {
    return partial(
      "commit-messages",
      "Commit message format",
      10,
      `Some commit subjects need work: ${badSubjects.slice(0, 2).join("; ")}`,
    );
  }

  return fail(
    "commit-messages",
    "Commit message format",
    `Commit subjects are vague or too long: ${badSubjects.slice(0, 2).join("; ")}`,
  );
}

function checkDiffSize(files, totals) {
  const changedLines = totals.additions + totals.deletions;

  if (changedLines <= 200 && files.length <= 10) {
    return pass(
      "diff-size",
      "Diff size and focus",
      `Small focused diff: ${changedLines} changed lines across ${files.length} files.`,
    );
  }

  if (changedLines <= 500 && files.length <= 20) {
    return partial(
      "diff-size",
      "Diff size and focus",
      12,
      `Medium diff: ${changedLines} changed lines across ${files.length} files.`,
    );
  }

  return fail(
    "diff-size",
    "Diff size and focus",
    `Large diff: ${changedLines} changed lines across ${files.length} files. Split if possible.`,
  );
}

function checkTestCoverage(files) {
  const sourceFiles = files.filter((file) => isSourceFile(file.filename));
  const testFiles = files.filter((file) => isTestFile(file.filename));

  if (sourceFiles.length === 0) {
    return pass("test-coverage", "Test coverage", "No source files changed that require tests.");
  }

  if (testFiles.length > 0) {
    return pass(
      "test-coverage",
      "Test coverage",
      `Found test coverage changes: ${testFiles.map((file) => file.filename).slice(0, 2).join(", ")}`,
    );
  }

  return fail(
    "test-coverage",
    "Test coverage",
    `Source files changed without test updates: ${sourceFiles
      .map((file) => file.filename)
      .slice(0, 2)
      .join(", ")}`,
  );
}

function isGoodCommitSubject(subject) {
  if (!subject || subject.length > 50) return false;

  const vaguePatterns = [
    /^fix$/i,
    /^fixes$/i,
    /^fixed$/i,
    /^fixed stuff$/i,
    /^update$/i,
    /^updates$/i,
    /^changes$/i,
    /^wip\b/i,
    /^work in progress\b/i,
    /^misc\b/i,
  ];

  return !vaguePatterns.some((pattern) => pattern.test(subject));
}

function isSourceFile(filename) {
  if (isTestFile(filename)) return false;
  if (!filename.startsWith("src/")) return false;

  return /\.(c|cc|cpp|h|hpp)$/i.test(filename);
}

function isTestFile(filename) {
  return (
    filename.includes("/test/") ||
    filename.includes("/tests/") ||
    filename.includes("/fuzz/") ||
    /(^|[/_-])test(s)?[._/-]/i.test(filename) ||
    /_tests?\.(c|cc|cpp|h|hpp|js|ts)$/i.test(filename)
  );
}

function pass(id, name, detail) {
  return {
    id,
    name,
    score: 20,
    max: 20,
    passed: true,
    detail,
  };
}

function partial(id, name, score, detail) {
  return {
    id,
    name,
    score,
    max: 20,
    passed: false,
    detail,
  };
}

function fail(id, name, detail) {
  return {
    id,
    name,
    score: 0,
    max: 20,
    passed: false,
    detail,
  };
}

function buildComment(result, feedback) {
  const statusIcon = result.passed ? "✅" : "⚠️";
  const statusText = result.passed ? "Passed" : "Needs Work";
  const routingText = result.passed
    ? "This PR meets the baseline contribution quality checks and has been routed for review."
    : "This PR did not meet the baseline quality threshold. Please address the feedback below.";

  const rows = result.checks
    .map((check) => {
      const icon = check.passed ? "✅" : check.score > 0 ? "⚠️" : "❌";
      return `| ${icon} | ${check.name} | ${check.score}/${check.max} | ${escapeTableCell(check.detail)} |`;
    })
    .join("\n");

  return `${BOT_COMMENT_MARKER}
## ${statusIcon} PR Quality Gate - ${statusText} (${result.score}/100)

${routingText}

Threshold: **${result.threshold}/100**

|  | Check | Score | Detail |
|---|---|---:|---|
${rows}

${feedback}

---
Core-Gate runs automatically when this PR is opened or updated.`;
}

function escapeTableCell(value) {
  return String(value).replaceAll("|", "\\|").replace(/\n/g, " ");
}

async function ensureLabels(octokit, context) {
  await Promise.all([
    ensureLabel(octokit, context, READY_LABEL, "0e8a16", "PR meets the baseline quality gate."),
    ensureLabel(octokit, context, NEEDS_WORK_LABEL, "d93f0b", "PR needs contributor fixes before review."),
  ]);
}

async function ensureLabel(octokit, context, name, color, description) {
  try {
    await octokit.issues.getLabel({
      owner: context.owner,
      repo: context.repo,
      name,
    });
  } catch (error) {
    if (error.status !== 404) throw error;

    await octokit.issues.createLabel({
      owner: context.owner,
      repo: context.repo,
      name,
      color,
      description,
    });
  }
}

async function applyRoutingLabel(octokit, context, passed) {
  const addLabel = passed ? READY_LABEL : NEEDS_WORK_LABEL;
  const removeLabel = passed ? NEEDS_WORK_LABEL : READY_LABEL;

  await octokit.issues.addLabels({
    owner: context.owner,
    repo: context.repo,
    issue_number: context.prNumber,
    labels: [addLabel],
  });

  try {
    await octokit.issues.removeLabel({
      owner: context.owner,
      repo: context.repo,
      issue_number: context.prNumber,
      name: removeLabel,
    });
  } catch (error) {
    if (error.status !== 404) throw error;
  }
}

async function upsertComment(octokit, context, body) {
  const comments = await octokit.paginate(octokit.issues.listComments, {
    owner: context.owner,
    repo: context.repo,
    issue_number: context.prNumber,
    per_page: 100,
  });

  const existingComment = comments.find((comment) => comment.body?.includes(BOT_COMMENT_MARKER));

  if (existingComment) {
    await octokit.issues.updateComment({
      owner: context.owner,
      repo: context.repo,
      comment_id: existingComment.id,
      body,
    });
    return;
  }

  await octokit.issues.createComment({
    owner: context.owner,
    repo: context.repo,
    issue_number: context.prNumber,
    body,
  });
}

main().catch((error) => {
  console.error("Core-Gate failed:");
  console.error(error);
  process.exitCode = 1;
});
