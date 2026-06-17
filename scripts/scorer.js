import { Octokit } from "@octokit/rest";
import { generateFeedback } from "./feedback.js";

// Scoring thresholds
const THRESHOLD_HIGH = 65; // ready-for-review
const THRESHOLD_MED  = 40; // needs-attention

// Comment marker so we can upsert instead of spam
const BOT_COMMENT_MARKER = "<!-- core-gate-comment -->";

// Labels — names match Bitcoin Core's existing labels where possible
const LABELS = {
  READY:     { name: "ready-for-review", color: "0e8a16", description: "Meets baseline quality gate" },
  ATTENTION: { name: "needs-attention",  color: "e4e669", description: "Close but needs minor fixes" },
  WORK:      { name: "needs-work",       color: "d93f0b", description: "Needs contributor fixes before review" },
  HIGH_RISK: { name: "high-risk",        color: "b60205", description: "Touches consensus or crypto code" },
  GUI:       { name: "gui-change",       color: "c5def5", description: "Touches Qt/GUI code" },
};

// Exact prefixes from Bitcoin Core CONTRIBUTING.md
const COMPONENT_PREFIXES = [
  "consensus", "doc", "qt", "gui", "log", "mining",
  "net", "p2p", "refactor", "rpc", "rest", "zmq",
  "contrib", "cli", "test", "qa", "ci", "util", "lib",
  "wallet", "build", "guix", "kernel", "mempool", "fees",
  "fuzz", "bench", "depends", "validation", "index",
  "addrman", "banman", "init", "node", "script", "crypto",
  "interfaces", "txmempool", "flatpak", "snap", "cmake",
];

// Risk path mapping
const RISK_PATHS = {
  high:   ["src/consensus/", "src/crypto/", "src/secp256k1/"],
  medium: ["src/wallet/", "src/rpc/", "src/validation/", "src/net/", "src/txmempool/"],
  gui:    ["src/qt/", "src/gui/"],
  low:    ["doc/", "contrib/", "test/", "src/test/"],
};

// AI slop buzzwords — high density = likely AI-generated
const BUZZWORDS = [
  "improves performance", "enhances security", "fixes various",
  "general improvements", "refactors code", "better code",
  "cleaner code", "improved readability", "minor fixes",
  "small fixes", "various fixes", "made it better",
  "optimized code", "code cleanup", "misc fixes", "miscellaneous",
  "overall improvements", "several improvements",
];

// ─── Entry point ────────────────────────────────────────────────────────────

async function main() {
  console.log("Core-Gate PR quality gate starting...");

  // Local mock mode for testing without a real PR
  if (process.env.CORE_GATE_MOCK) {
    await runMock(process.env.CORE_GATE_MOCK);
    return;
  }

  const context = readContext();
  if (!context) return;

  const octokit = new Octokit({ auth: context.token });

  // Fetch PR data and Bitcoin Core labels in parallel
  const [prData, btcLabels] = await Promise.all([
    fetchPullRequestData(octokit, context),
    fetchBitcoinCoreLabels(),
  ]);

  const risk    = getRiskProfile(prData.files);
  const result  = scorePullRequest(prData, risk);
  const feedback = result.passed
    ? null
    : await generateFeedback({
        result,
        risk,
        pr: prData.pr,
        files: prData.files,
        commits: prData.commits,
        authorData: prData.authorData,
        apiKey: process.env.KIMI_API_KEY || process.env.ANTHROPIC_API_KEY,
      });

  const body = buildComment(result, risk, feedback);

  console.log(`PR #${context.prNumber}: ${prData.pr.title}`);
  console.log(`Author: ${prData.pr.user?.login ?? "unknown"} | Prior merges: ${prData.authorPriorMerges}`);
  console.log(`Commits: ${prData.commits.length} | Files: ${prData.files.length}`);
  console.log(`Risk: ${risk.level} | Score: ${result.score}/100`);

  await ensureAllLabels(octokit, context, btcLabels);
  await applyLabels(octokit, context, result, risk);
  await upsertComment(octokit, context, body);

  console.log("Core-Gate complete.");
}

// ─── Mock mode ──────────────────────────────────────────────────────────────

async function runMock(kind) {
  const prData = buildMockData(kind);
  const risk   = getRiskProfile(prData.files);
  const result = scorePullRequest(prData, risk);
  const feedback = result.passed
    ? null
    : await generateFeedback({
        result, risk,
        pr: prData.pr,
        files: prData.files,
        commits: prData.commits,
        authorData: prData.authorData,
        apiKey: "",
      });

  console.log(`Mock: ${kind} | Score: ${result.score}/100 | Risk: ${risk.level}`);
  console.log(buildComment(result, risk, feedback));
}

function buildMockData(kind) {
  if (kind === "good") {
    const files = [
      mockFile("src/wallet/wallet.cpp", 28, 7),
      mockFile("src/test/wallet_tests.cpp", 38, 2),
    ];
    return {
      pr: {
        title: "wallet: fix fee rounding edge case",
        body: "Fixes a fee rounding edge case in wallet transaction creation.\n\nThe previous behavior could round a small value inconsistently. This patch keeps the calculation explicit and adds a regression test.\n\nFixes #12345",
        user: { login: "demo-contributor" },
      },
      commits: [mockCommit("wallet: fix fee rounding")],
      files,
      totals: calcTotals(files),
      authorData: { login: "demo-contributor", created_at: "2020-01-01T00:00:00Z" },
      authorPriorMerges: 3,
    };
  }

  const files = [mockFile("src/bitcoin-cli.cpp", 2, 0)];
  return {
    pr: { title: "fixed stuff", body: "", user: { login: "newuser" } },
    commits: [mockCommit("fixed stuff")],
    files,
    totals: calcTotals(files),
    authorData: { login: "newuser", created_at: new Date().toISOString() },
    authorPriorMerges: 0,
  };
}

function mockCommit(subject) { return { commit: { message: subject } }; }
function mockFile(filename, additions, deletions) {
  return { filename, additions, deletions, changes: additions + deletions };
}
function calcTotals(files) {
  return files.reduce(
    (s, f) => ({ additions: s.additions + f.additions, deletions: s.deletions + f.deletions, changes: s.changes + f.changes }),
    { additions: 0, deletions: 0, changes: 0 }
  );
}

// ─── Context ─────────────────────────────────────────────────────────────────

function readContext() {
  const required = ["GITHUB_TOKEN", "REPO", "PR_NUMBER"];
  const missing  = required.filter(n => !process.env[n]);

  if (missing.length > 0) {
    console.log(`Missing env vars: ${missing.join(", ")}`);
    if (process.env.GITHUB_ACTIONS === "true") process.exitCode = 1;
    return null;
  }

  const [owner, repo] = process.env.REPO.split("/");
  const prNumber = Number(process.env.PR_NUMBER);

  if (!owner || !repo || Number.isNaN(prNumber)) {
    throw new Error("Invalid REPO or PR_NUMBER.");
  }

  return { token: process.env.GITHUB_TOKEN, owner, repo, prNumber };
}

// ─── Data fetching ───────────────────────────────────────────────────────────

async function fetchPullRequestData(octokit, context) {
  const { owner, repo, prNumber } = context;

  const [prResponse, commits, files] = await Promise.all([
    octokit.pulls.get({ owner, repo, pull_number: prNumber }),
    octokit.paginate(octokit.pulls.listCommits, { owner, repo, pull_number: prNumber, per_page: 100 }),
    octokit.paginate(octokit.pulls.listFiles,   { owner, repo, pull_number: prNumber, per_page: 100 }),
  ]);

  const totals = calcTotals(files);
  const pr     = prResponse.data;
  const author = pr.user?.login ?? "";

  // Fetch author data and prior merged PRs
  let authorData       = { login: author, created_at: new Date().toISOString() };
  let authorPriorMerges = 0;

  try {
    const [userData, searchData] = await Promise.all([
      octokit.users.getByUsername({ username: author }),
      octokit.search.issuesAndPullRequests({
        q: `repo:${owner}/${repo} is:pr is:merged author:${author}`
      }),
    ]);
    authorData        = userData.data;
    authorPriorMerges = searchData.data.total_count;
  } catch (_) {}

  return { pr, commits, files, totals, authorData, authorPriorMerges };
}

// Fetch Bitcoin Core's actual labels to reuse their names and colors
async function fetchBitcoinCoreLabels() {
  try {
    const octokit = new Octokit(); // no auth needed for public repo
    const labels  = await octokit.paginate(octokit.issues.listLabelsForRepo, {
      owner: "bitcoin",
      repo:  "bitcoin",
      per_page: 100,
    });
    return labels; // [{ name, color, description }]
  } catch (_) {
    console.log("Could not fetch Bitcoin Core labels — using defaults.");
    return [];
  }
}

// ─── Risk profiling ──────────────────────────────────────────────────────────

function getRiskProfile(files) {
  const names = files.map(f => f.filename);

  if (RISK_PATHS.high.some(p => names.some(f => f.startsWith(p)))) {
    return {
      level: "HIGH",
      label: LABELS.HIGH_RISK.name,
      btcLabel: "Consensus",
      message: "⚠️ **High Risk** — touches consensus-critical or cryptography code. Requires senior maintainer review.",
      requiresBIP: true,
    };
  }

  if (RISK_PATHS.gui.some(p => names.some(f => f.startsWith(p)))) {
    // Check if ONLY gui files — if so, should target bitcoin-core/gui repo
    const nonGuiFiles = names.filter(f =>
      !RISK_PATHS.gui.some(p => f.startsWith(p))
    );
    return {
      level: "GUI",
      label: LABELS.GUI.name,
      btcLabel: "GUI",
      message: nonGuiFiles.length === 0
        ? "🖥️ **GUI Only** — this PR only touches `src/qt/`. Consider targeting the [bitcoin-core/gui](https://github.com/bitcoin-core/gui) repository instead."
        : "🖥️ **GUI Change** — touches Qt/GUI code. GUI PRs attract high comment volume; ensure you have concept ACKs before requesting review.",
      requiresBIP: false,
    };
  }

  if (RISK_PATHS.medium.some(p => names.some(f => f.startsWith(p)))) {
    return {
      level: "MEDIUM",
      label: null,
      btcLabel: names.some(f => f.startsWith("src/wallet/")) ? "Wallet" : null,
      message: "🔶 **Medium Risk** — touches wallet, RPC, or validation code. Functional tests required.",
      requiresBIP: false,
    };
  }

  return {
    level: "LOW",
    label: null,
    btcLabel: names.some(f => f.startsWith("src/test/") || f.includes("/test/")) ? "Tests" : null,
    message: "🟢 **Low Risk** — touches documentation, tests, or non-consensus code.",
    requiresBIP: false,
  };
}

// ─── Scoring ─────────────────────────────────────────────────────────────────

function scorePullRequest({ pr, commits, files, totals, authorData, authorPriorMerges }, risk) {
  const checks = [
    checkTitlePrefix(pr.title),
    checkDescription(pr.body ?? "", pr.title, risk, authorPriorMerges),
    checkCommitMessages(commits),
    checkDiffSize(files, totals),
    checkTestCoverage(files, risk),
  ];

  // Informational checks — no score impact
  const info = [
    checkContributorHistory(authorData, authorPriorMerges),
  ];

  const score = checks.reduce((sum, c) => sum + c.score, 0);

  return {
    score,
    thresholdHigh: THRESHOLD_HIGH,
    thresholdMed:  THRESHOLD_MED,
    passed:        score >= THRESHOLD_HIGH,
    lane:          score >= THRESHOLD_HIGH ? "ready" : score >= THRESHOLD_MED ? "attention" : "work",
    checks,
    info,
  };
}

// Check 1 — Component prefix (20pts)
function checkTitlePrefix(title) {
  const lower  = title.toLowerCase();
  const prefix = COMPONENT_PREFIXES.find(p => lower.startsWith(`${p}:`));

  if (prefix) {
    return pass("title-prefix", "Component prefix", `Found valid prefix: \`${prefix}:\``);
  }

  const examples = ["wallet", "rpc", "test", "doc", "consensus", "net"]
    .map(p => `\`${p}:\``).join(", ");

  return fail("title-prefix", "Component prefix",
    `Title \`"${title}"\` is missing a component prefix. Valid prefixes from CONTRIBUTING.md: ${examples} and more.`
  );
}

// Check 2 — Description quality (20pts)
// Scored on quality signals, not just length.
// A short but technically specific description outscores a long buzzword-filled one.
//
// Signal breakdown (20pts total):
//   6pts — explains the problem (what is broken/missing)
//   6pts — technical specificity (file names, function names, Bitcoin terms)
//   4pts — issue reference (Fixes #XXXX)
//   2pts — no @mentions
//   2pts — no buzzword language
//
// Penalties:
//   -5pts — missing issue link (if description exists)
//   -5pts — consensus change without BIP reference
//   -5pts — @mentions present
//   -5pts — buzzword density >= 2
//   WIP   — capped at 10pts regardless of signals
function checkDescription(body, title, risk, authorPriorMerges) {
  const cleaned    = body.replace(/<!--[\s\S]*?-->/g, "").trim();
  const lower      = cleaned.toLowerCase();
  const hasMention = /(^|\s)@\w+/.test(cleaned);
  const isWIP      = /\[wip\]/i.test(title) || /\b(wip|work in progress)\b/i.test(cleaned);
  const hasIssueRef = /(fixes|closes|resolves|refs?)\s+#\d+/i.test(cleaned) || /#\d+/.test(cleaned);
  const buzzwordsFound = BUZZWORDS.filter(w => lower.includes(w));
  const hasBuzzwords   = buzzwordsFound.length >= 2;

  // Consensus changes need a BIP reference
  const needsBIP = risk.requiresBIP;
  const hasBIP   = /BIP[-\s]?\d+/i.test(cleaned);

  // Signal 1 — problem statement (6pts)
  // Does the description explain what is broken, wrong, or missing?
  const hasProblemStatement = /\b(fix(es|ed)?|bug|issue|problem|broken|fail(s|ed)?|error|wrong|incorrect|crash(es|ed)?|regression|edge case|undefined|unexpected|missing|lack)\b/i.test(cleaned);
  const problemPts = hasProblemStatement ? 6 : 0;

  // Signal 2 — technical specificity (6pts)
  // Bitcoin Core-specific terms, file paths, function names, or concrete values
  // A description mentioning src/wallet/ or CWallet:: is almost certainly written by a human who read the code
  const hasTechnicalDetail = (
    /src\/[a-z]/.test(cleaned) ||                          // file path like src/wallet/
    /\b[A-Z][a-z]+::[A-Z][a-zA-Z]+/.test(cleaned) ||      // C++ method like CWallet::CreateTransaction
    /\b(sat(oshi)?s?|btc|utxo|mempool|scriptpubkey|witness|segwit|taproot|schnorr|secp256k1|bip\d+)\b/i.test(cleaned) || // Bitcoin terms
    /\b(assert|nullptr|overflow|underflow|integer|race condition|deadlock|mutex|lock|thread)\b/i.test(cleaned) || // technical CS terms
    /#\d{4,}/.test(cleaned) ||                             // issue/PR reference with 4+ digit number
    /0x[0-9a-fA-F]+/.test(cleaned)                        // hex value
  );
  const technicalPts = hasTechnicalDetail ? 6 : 0;

  // Signal 3 — issue reference (4pts)
  const issuePts = hasIssueRef ? 4 : 0;

  // Signal 4 — no @mentions (2pts)
  const mentionPts = hasMention ? 0 : 2;

  // Signal 5 — no buzzwords (2pts)
  const buzzPts = hasBuzzwords ? 0 : 2;

  let score = problemPts + technicalPts + issuePts + mentionPts + buzzPts;
  const notes = [];

  // Empty description
  if (cleaned.length === 0) {
    score = 0;
    notes.push("No description — explain the problem, approach, and impact");
  } else if (isWIP) {
    score = Math.min(score, 10);
    notes.push("Marked [WIP] — partial credit");
  }

  // Signal feedback
  if (hasProblemStatement)  notes.push("✓ Problem statement found");
  else                      notes.push("Missing problem statement — what is broken or wrong?");

  if (hasTechnicalDetail)   notes.push("✓ Technical specificity found");
  else                      notes.push("Missing technical detail — mention file paths, function names, or specific values");

  if (hasIssueRef)          notes.push("✓ Issue reference found");
  else if (cleaned.length > 0) notes.push("No issue reference — add `Fixes #XXXX` or `Closes #XXXX`");

  // Penalties
  if (needsBIP && !hasBIP) { score -= 5; notes.push("Consensus change requires a BIP reference"); }
  if (hasMention)           { notes.push("Remove @mentions — they spam every fork's notification feed"); }
  if (hasBuzzwords)         { notes.push(`Buzzword language detected ("${buzzwordsFound.slice(0, 2).join('", "')}") — be specific`); }

  score = Math.max(0, Math.min(20, score));

  return {
    id: "description", name: "PR description",
    score, max: 20,
    passed: score >= 15,
    detail: notes.join(". "),
    meta: { hasIssueRef, hasBuzzwords, buzzwordsFound, hasBIP, needsBIP },
  };
}

// Check 3 — Commit messages (20pts)
function checkCommitMessages(commits) {
  if (commits.length === 0) {
    return fail("commit-messages", "Commit message format", "No commits found on this PR.");
  }

  const subjects = commits.map(c => c.commit.message.split("\n")[0].trim());

  const badSubjects = subjects.filter(s => !isGoodSubject(s));
  const hasFixups   = subjects.some(s => /^(fixup|squash)!/i.test(s));
  const hasBuzzwords = subjects.some(s =>
    BUZZWORDS.some(w => s.toLowerCase().includes(w))
  );

  const notes = [];
  let score = 20;

  if (badSubjects.length > 0) {
    score -= badSubjects.length >= commits.length ? 20 : 10;
    notes.push(`Vague or too-long subjects: ${badSubjects.slice(0, 2).map(s => `"${s}"`).join(", ")}`);
  }

  if (hasFixups) {
    score -= 5;
    notes.push("Fixup/squash commits found — squash these before requesting review (see CONTRIBUTING.md)");
  }

  if (hasBuzzwords) {
    score -= 5;
    notes.push("Buzzword language in commit messages — be specific about what changed");
  }

  score = Math.max(0, score);

  if (score === 20) {
    return pass("commit-messages", "Commit message format", "Commit subjects are specific and concise.");
  }

  return {
    id: "commit-messages", name: "Commit message format",
    score, max: 20,
    passed: score >= 15,
    detail: notes.join(". "),
  };
}

// Check 4 — Diff size (20pts)
function checkDiffSize(files, totals) {
  const changed = totals.additions + totals.deletions;

  if (changed <= 200 && files.length <= 10) {
    return pass("diff-size", "Diff size and focus",
      `Small focused diff: ${changed} changed lines across ${files.length} files.`);
  }

  if (changed <= 500 && files.length <= 20) {
    return partial("diff-size", "Diff size and focus", 12,
      `Medium diff: ${changed} changed lines across ${files.length} files. Split if unrelated changes exist.`);
  }

  return fail("diff-size", "Diff size and focus",
    `Large diff: ${changed} changed lines across ${files.length} files. Bitcoin Core prefers small reviewable PRs — split this up.`);
}

// Check 5 — Test coverage (20pts)
function checkTestCoverage(files, risk) {
  const srcFiles  = files.filter(f => isSourceFile(f.filename));
  const testFiles = files.filter(f => isTestFile(f.filename));

  if (srcFiles.length === 0) {
    return pass("test-coverage", "Test coverage", "No source files changed that require tests.");
  }

  if (testFiles.length > 0) {
    return pass("test-coverage", "Test coverage",
      `Test coverage included: ${testFiles.slice(0, 2).map(f => f.filename).join(", ")}`);
  }

  // Find which src files are untested
  const untested = srcFiles
    .slice(0, 3)
    .map(f => f.filename);

  const testDir = srcFiles.some(f => f.filename.startsWith("src/wallet/"))
    ? "src/test/wallet_tests.cpp or test/functional/wallet_*.py"
    : "src/test/ or test/functional/";

  return fail("test-coverage", "Test coverage",
    `No test files found. Files needing tests: ${untested.join(", ")}. Add tests in ${testDir}.`);
}

// Informational — contributor history (no score impact)
function checkContributorHistory(authorData, authorPriorMerges) {
  const ageDays = Math.floor(
    (Date.now() - new Date(authorData.created_at).getTime()) / (1000 * 60 * 60 * 24)
  );
  const isNew = ageDays < 30 || authorPriorMerges === 0;

  return {
    id: "contributor", name: "Contributor history",
    isNew, ageDays, authorPriorMerges,
    detail: isNew
      ? `New contributor — account ${ageDays} days old, ${authorPriorMerges} prior merged PRs. Note: CONTRIBUTING.md advises against refactoring PRs from new contributors.`
      : `${authorPriorMerges} prior merged PRs in this repo.`,
  };
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

function isGoodSubject(subject) {
  if (!subject || subject.length > 50) return false;
  const vague = [
    /^fix(ed|es)?[\s:!]?$/i, /^fixed stuff$/i,
    /^update[sd]?[\s:!]?$/i, /^change[sd]?[\s:!]?$/i,
    /^wip\b/i, /^work in progress\b/i, /^misc\b/i,
  ];
  return !vague.some(p => p.test(subject));
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
    filename.includes("/bench/") ||
    /_tests?\.(c|cc|cpp|h|hpp)$/i.test(filename) ||
    (filename.endsWith(".py") && filename.includes("functional"))
  );
}

function pass(id, name, detail)            { return { id, name, score: 20, max: 20, passed: true,  detail }; }
function partial(id, name, score, detail)  { return { id, name, score,     max: 20, passed: false, detail }; }
function fail(id, name, detail)            { return { id, name, score: 0,  max: 20, passed: false, detail }; }

// ─── Comment builder ─────────────────────────────────────────────────────────

function buildComment(result, risk, feedback) {
  const { score, lane, checks, info, thresholdHigh } = result;

  const header = {
    ready:     `## ✅ PR Quality Gate — Passed (${score}/100)`,
    attention: `## 🔶 PR Quality Gate — Needs Attention (${score}/100)`,
    work:      `## ⚠️ PR Quality Gate — Needs Work (${score}/100)`,
  }[lane];

  const routing = {
    ready:     "This PR meets Bitcoin Core's contribution guidelines and has been routed to maintainers for review.",
    attention: `This PR is close but needs a few fixes before entering the maintainer review queue (threshold: ${thresholdHigh}/100).`,
    work:      `This PR did not meet the baseline quality threshold (${thresholdHigh}/100). Please address the feedback below.`,
  }[lane];

  const rows = checks.map(c => {
    const icon = c.passed ? "✅" : c.score > 0 ? "⚠️" : "❌";
    return `| ${icon} | ${c.name} | ${c.score}/${c.max} | ${escapeCell(c.detail)} |`;
  }).join("\n");

  const scoreTable = `| | Check | Score | Detail |
|---|---|---:|---|
${rows}
| | **Total** | **${score}/100** | |`;

  const contributorInfo = info[0];
  const newContributorNote = contributorInfo?.isNew
    ? `\n> 👋 **First-time contributor** — welcome! Note that CONTRIBUTING.md advises against refactoring PRs from new contributors until you have more context on the codebase.\n`
    : "";

  const feedbackSection = feedback
    ? `\n---\n\n### What to Fix\n\n${feedback}\n\n---\n\n*Push a new commit to re-run this check automatically.*`
    : "";

  return `${BOT_COMMENT_MARKER}
${header}

${routing}

${risk.message}
${newContributorNote}
${scoreTable}
${feedbackSection}
*Scored against [Bitcoin Core's CONTRIBUTING.md](https://github.com/bitcoin/bitcoin/blob/master/CONTRIBUTING.md) · Powered by [Core-Gate](https://github.com/Zeegaths/core-gate)*`;
}

function escapeCell(value) {
  return String(value).replaceAll("|", "\\|").replace(/\n/g, " ");
}

// ─── Label management ────────────────────────────────────────────────────────

async function ensureAllLabels(octokit, context, btcLabels) {
  const btcLabelMap = Object.fromEntries(btcLabels.map(l => [l.name, l.color]));

  const allLabels = Object.values(LABELS);

  await Promise.all(allLabels.map(async label => {
    // Use Bitcoin Core's color if the label exists there
    const color = btcLabelMap[label.name] || label.color;
    try {
      await octokit.issues.getLabel({ owner: context.owner, repo: context.repo, name: label.name });
    } catch (err) {
      if (err.status !== 404) return;
      await octokit.issues.createLabel({
        owner: context.owner, repo: context.repo,
        name: label.name, color, description: label.description,
      });
    }
  }));
}

async function applyLabels(octokit, context, result, risk) {
  const { owner, repo, prNumber } = context;

  // Routing label based on lane
  const routingLabel = {
    ready:     LABELS.READY.name,
    attention: LABELS.ATTENTION.name,
    work:      LABELS.WORK.name,
  }[result.lane];

  const removeLabels = [LABELS.READY.name, LABELS.ATTENTION.name, LABELS.WORK.name]
    .filter(l => l !== routingLabel);

  // Add routing label
  await octokit.issues.addLabels({ owner, repo, issue_number: prNumber, labels: [routingLabel] });

  // Remove stale routing labels
  await Promise.all(removeLabels.map(name =>
    octokit.issues.removeLabel({ owner, repo, issue_number: prNumber, name }).catch(() => {})
  ));

  // Add risk label if applicable
  if (risk.label) {
    await octokit.issues.addLabels({ owner, repo, issue_number: prNumber, labels: [risk.label] }).catch(() => {});
  }

  // Add Bitcoin Core component label if applicable
  if (risk.btcLabel) {
    try {
      await octokit.issues.addLabels({ owner, repo, issue_number: prNumber, labels: [risk.btcLabel] });
    } catch (_) {}
  }
}

// ─── Comment upsert ──────────────────────────────────────────────────────────

async function upsertComment(octokit, context, body) {
  const { owner, repo, prNumber } = context;

  const comments = await octokit.paginate(octokit.issues.listComments, {
    owner, repo, issue_number: prNumber, per_page: 100,
  });

  const existing = comments.find(c => c.body?.includes(BOT_COMMENT_MARKER));

  if (existing) {
    await octokit.issues.updateComment({ owner, repo, comment_id: existing.id, body });
  } else {
    await octokit.issues.createComment({ owner, repo, issue_number: prNumber, body });
  }
}

// ─── Run ─────────────────────────────────────────────────────────────────────

main().catch(err => {
  console.error("Core-Gate failed:", err);
  process.exitCode = 1;
});